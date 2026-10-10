#include <JuceHeader.h>
#include "SF2Synth.h"

#define TSF_IMPLEMENTATION
#include <tsf.h>

// ══════════════════════════════════════════════════════════════════════════════
// SF2Synth
// ══════════════════════════════════════════════════════════════════════════════
SF2Synth::SF2Synth() {}

SF2Synth::~SF2Synth()
{
    unload();
}

bool SF2Synth::loadSF2(const juce::File& sf2File)
{
    unload();

    juce::MemoryBlock data;
    if (!sf2File.loadFileAsData(data))
    {
        lastError = "Could not read the SF2 file: " + sf2File.getFullPathName();
        return false;
    }

    font = tsf_load_memory(data.getData(), (int)data.getSize());
    if (font == nullptr)
    {
        lastError = "Could not parse the SF2 file: " + sf2File.getFullPathName();
        return false;
    }

    const int count = tsf_get_presetcount(font);
    if (count <= 0)
    {
        lastError = "The SF2 file does not contain any preset.";
        tsf_close(font);
        font = nullptr;
        return false;
    }

    // Enumerar (banco, número de programa, nombre) de cada preset.
    // tsf_channel_get_preset_bank/number leen el estado del CANAL MIDI, no del
    // preset por índice (tsf_channel_set_presetindex nunca actualiza c->bank),
    // así que siempre devolvían banco 0. font->presets[i] expone bank/preset
    // directamente (struct tsf_preset es pública en tsf.h) y sí es correcto.
    presets.clear();
    presets.reserve((size_t)count);
    for (int i = 0; i < count; ++i)
    {
        Preset p;
        p.index        = i;
        p.bank         = font->presets[i].bank;
        p.presetNumber = font->presets[i].preset;
        p.name         = juce::String::fromUTF8(tsf_get_presetname(font, i));
        presets.push_back(p);
    }

    tsf_set_output(font, TSF_STEREO_UNWEAVED, (int)sampleRate, 0.0f);
    // Reserve once off the audio thread; applyGlobals changes only the active range.
    if (!tsf_set_max_voices(font, 64))
    {
        unload();
        lastError = "Unable to allocate SoundFont voices.";
        return false;
    }
    applyGlobals();

    loaded = true;
    lastError = {};
    selectPreset(0);
    return true;
}

void SF2Synth::unload()
{
    lastGlideVoice = lastGlideNote = -1;
    voiceGlides = {};
    if (font != nullptr)
    {
        tsf_close(font);
        font = nullptr;
    }
    presets.clear();
    currentPresetIndex = -1;
    loaded = false;
    lastError = {};

    filterL.reset();
    filterR.reset();
}

void SF2Synth::prepare(double sr, int blockSize)
{
    sampleRate = sr;
    renderScratch.assign((size_t)blockSize * 2, 0.0f);
    if (font != nullptr)
        tsf_set_output(font, TSF_STEREO_UNWEAVED, (int)sampleRate, 0.0f);
    allNotesOff();

    lfo1.reset();
    lfo2.reset();
    filterEnv.setup(fltAttack, 0.0f, fltDecay, fltSustain, fltRelease, sampleRate);
}

bool SF2Synth::selectPreset(int presetIndex)
{
    if (font == nullptr || presetIndex < 0 || presetIndex >= (int)presets.size())
        return false;

    tsf_channel_set_presetindex(font, 0, presetIndex);
    tsf_channel_set_pitchrange(font, 0, 2.0f); // ±2 semitonos, igual que SFZSynth::pitchBend
    currentPresetIndex = presetIndex;
    return true;
}

void SF2Synth::applyGlobals()
{
    if (font == nullptr) return;
    tsf_set_volume(font, juce::Decibels::decibelsToGain(masterVolume));
    const int limit = juce::jlimit(1, 64, maxVoices);
    for (int i = limit; i < font->voiceNum; ++i) tsf_voice_kill(&font->voices[i]);
    // TinySoundFont's allocator only grows its limit. Keep the reserved memory,
    // but update the range used for both note allocation and rendering.
    font->voiceNum = font->maxVoiceNum = limit;
}

void SF2Synth::applyPresetOverrides()
{
    if (font == nullptr || currentPresetIndex < 0 || currentPresetIndex >= font->presetNum)
        return;

    auto& preset = font->presets[currentPresetIndex];
    const float sustainGain = juce::jlimit(0.0f, 1.0f, ampSustain / 100.0f);

    for (int r = 0; r < preset.regionNum; ++r)
    {
        auto& region = preset.regions[r];

        // ADSR de amplitud: sustituye la envolvente propia del preset SF2 por los
        // knobs del plugin (mismo criterio "siempre aplicar el ADSR global" que
        // SFZSynth ya usa para SFZ).
        region.ampenv.delay         = 0.0f;
        region.ampenv.attack        = ampAttack;
        region.ampenv.hold          = 0.0f;
        region.ampenv.decay         = ampDecay;
        region.ampenv.sustain       = sustainGain;
        region.ampenv.release       = ampRelease;
        region.ampenv.keynumToHold  = 0.0f;
        region.ampenv.keynumToDecay = 0.0f;

        // Desactiva el filtro paso-bajo interno de tsf por voz: el filtro global
        // de bus (filterL/filterR, más abajo) es el que controla el sonido para
        // SF2, igual que el ADSR de arriba sustituye al de la región.
        region.initialFilterFc = 20000; // > 13500 => tsf lo trata como "sin filtro"
    }
}

void SF2Synth::renderSegment(juce::AudioBuffer<float>& output, int startSample, int numSamples)
{
    // Short render slices keep per-voice pitch slides smooth at any host block size.
    if (numSamples > 16 && glideTime > 0.0f)
    {
        for (int offset = 0; offset < numSamples; offset += 16)
            renderSegment(output, startSample + offset, juce::jmin(16, numSamples - offset));
        return;
    }

    if (numSamples <= 0) return;
    if ((size_t)(numSamples * 2) > renderScratch.size())
        renderScratch.assign((size_t)numSamples * 2, 0.0f);

    // LFOs y envolvente de filtro (global, en bus — mismo esquema que SFZSynth)
    double lfo1Val = lfo1.process(sampleRate, numSamples) * modWheel.scale(1);
    double lfo2Val = lfo2.process(sampleRate, numSamples) * modWheel.scale(2);

    double fltEnvLevel = filterEnv.process();
    float  cutoffMod   = (float)fltEnvLevel
                       * juce::jlimit(-1.0f, 1.0f, filterEnvAmt)
                       * filterCutoff;
    if (lfo1.target == LFO::Target::Cutoff) cutoffMod += (float)lfo1Val * filterCutoff;
    if (lfo2.target == LFO::Target::Cutoff) cutoffMod += (float)lfo2Val * filterCutoff;

    float ampMod = 0.0f;
    if (lfo1.target == LFO::Target::Volume) ampMod += (float)lfo1Val * 0.5f;
    if (lfo2.target == LFO::Target::Volume) ampMod += (float)lfo2Val * 0.5f;

    float pitchMod = 0.0f;
    if (lfo1.target == LFO::Target::Pitch) pitchMod += (float)lfo1Val * 12.0f;
    if (lfo2.target == LFO::Target::Pitch) pitchMod += (float)lfo2Val * 12.0f;

    tsf_channel_set_tuning(font, 0, globalTranspose + globalTune / 100.0f + pitchMod);

    const float cutoffModded = juce::jlimit(20.0f, 20000.0f, filterCutoff + cutoffMod);
    const auto  filtType     = static_cast<SFZFilterType>(filterTypeIdx);
    filterL.setType(filtType, cutoffModded, filterResonance, sampleRate);
    filterR.setType(filtType, cutoffModded, filterResonance, sampleRate);

    const auto* channel = font->channels ? &font->channels->channels[0] : nullptr;
    const float bend = channel && channel->pitchWheel != 8192
        ? channel->pitchWheel / 16383.0f * channel->pitchRange * 2.0f - channel->pitchRange : 0.0f;
    for (int i = 0; i < font->voiceNum; ++i)
        if (font->voices[i].playingPreset != -1)
            tsf_voice_calcpitchratio(&font->voices[i], globalTranspose + globalTune / 100.0f + pitchMod + bend
                + (float) voiceGlides[(size_t) i].advance(numSamples), (float) sampleRate);
    tsf_render_float(font, renderScratch.data(), numSamples, 0);

    // TSF_STEREO_UNWEAVED: primera mitad = canal L, segunda mitad = canal R
    const float* srcL = renderScratch.data();
    const float* srcR = renderScratch.data() + numSamples;

    // Pan/volumen master como post-proceso sobre el buffer ya renderizado
    // (misma ley seno usada en SFZVoice para el pan)
    double panRad = juce::jlimit(-1.0, 1.0, (double)masterPan / 100.0)
                  * juce::MathConstants<double>::pi / 4.0;
    float panL = (float)std::cos(panRad);
    float panR = (float)std::sin(panRad + juce::MathConstants<double>::pi / 2.0);

    const float gainMod = 1.0f + ampMod;

    float* outL = output.getWritePointer(0, startSample);
    float* outR = output.getWritePointer(output.getNumChannels() > 1 ? 1 : 0, startSample);
    for (int i = 0; i < numSamples; ++i)
    {
        outL[i] = (float)filterL.process(srcL[i] * panL * gainMod);
        if (outR != outL)
            outR[i] = (float)filterR.process(srcR[i] * panR * gainMod);
    }
}

void SF2Synth::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    buffer.clear();
    if (!loaded || font == nullptr) return;

    applyGlobals();
    applyPresetOverrides();

    const int numSamples = buffer.getNumSamples();
    int samplePos = 0;

    for (const auto meta : midiMessages)
    {
        int eventSample = juce::jlimit(0, numSamples - 1, meta.samplePosition);
        if (eventSample > samplePos)
            renderSegment(buffer, samplePos, eventSample - samplePos);

        auto msg = meta.getMessage();
        if (msg.isNoteOn(true))
        {
            // Re-dispara la envolvente de filtro global en cada nota (igual que
            // SFZSynth::noteOn), estilo "mono": una única envolvente compartida
            // por todas las voces ya que SF2 se renderiza mezclado en bus.
            filterEnv.setup(fltAttack, 0.0f, fltDecay, fltSustain, fltRelease, sampleRate);
            filterEnv.noteOn();
            const double fromPitch = lastGlideVoice >= 0 && lastGlideVoice < font->voiceNum
                && font->voices[lastGlideVoice].playingPreset != -1
                ? lastGlideNote + voiceGlides[(size_t) lastGlideVoice].value() : (double) lastGlideNote;
            const double glideOffset = lastGlideNote >= 0 ? fromPitch - msg.getNoteNumber() : 0.0;
            const auto playIndex = font->voicePlayIndex;
            bool freeVoice = false;
            tsf_voice* oldest = nullptr;
            for (int i = 0; i < font->voiceNum; ++i)
            {
                auto& voice = font->voices[i];
                if (voice.playingPreset == -1) { freeVoice = true; break; }
                if (!oldest || voice.playIndex < oldest->playIndex) oldest = &voice;
            }
            if (!freeVoice && oldest) tsf_voice_kill(oldest);
            tsf_channel_note_on(font, 0, msg.getNoteNumber(), (float)msg.getVelocity() / 127.0f);
            for (int i = 0; i < font->voiceNum; ++i)
                if (font->voices[i].playingPreset != -1 && font->voices[i].playIndex == playIndex)
                {
                    voiceGlides[(size_t) i].start(glideOffset, glideTime, sampleRate);
                    lastGlideVoice = i; lastGlideNote = msg.getNoteNumber();
                }
        }
        else if (msg.isNoteOff(true))
            tsf_channel_note_off(font, 0, msg.getNoteNumber());
        else if (msg.isPitchWheel())
            tsf_channel_set_pitchwheel(font, 0, msg.getPitchWheelValue());
        else if (msg.isController())
        {
            modWheel.controller(msg.getControllerNumber(), msg.getControllerValue());
            tsf_channel_midi_control(font, 0, msg.getControllerNumber(), msg.getControllerValue());
        }

        samplePos = eventSample;
    }
    if (samplePos < numSamples)
        renderSegment(buffer, samplePos, numSamples - samplePos);

    midiMessages.clear();

    if (buffer.getNumChannels() > 0)
        meterLevelL.store(buffer.getMagnitude(0, 0, numSamples));
    if (buffer.getNumChannels() > 1)
        meterLevelR.store(buffer.getMagnitude(1, 0, numSamples));
}

void SF2Synth::allNotesOff()
{
    lastGlideVoice = lastGlideNote = -1;
    if (font != nullptr)
        tsf_note_off_all(font);
}
