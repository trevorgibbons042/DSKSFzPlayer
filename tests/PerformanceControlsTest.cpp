#include "SFZ/SFZSynth.h"
#include "SF2/SF2Synth.h"
#include <iostream>
#include <stdexcept>

static void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
static void chunk(juce::MemoryOutputStream& out, const char* id, const juce::MemoryOutputStream& data)
{
    out.write(id, 4); out.writeInt((int) data.getDataSize()); out.write(data.getData(), data.getDataSize());
}
static void name(juce::MemoryOutputStream& out, const char* text)
{ char bytes[20] {}; std::strncpy(bytes, text, 20); out.write(bytes, 20); }
static void writeSoundFont(const juce::File& file)
{
    juce::MemoryOutputStream pdta, phdr, pbag, pmod, pgen, inst, ibag, imod, igen, shdr, samples, sdta, body, riff;
    for (int i = 0; i < 2; ++i)
    {
        name(phdr, i ? "EOP" : "Sine"); phdr.writeShort(0); phdr.writeShort(0); phdr.writeShort((short) i);
        phdr.writeInt(0); phdr.writeInt(0); phdr.writeInt(0);
        pbag.writeShort((short) i); pbag.writeShort(0);
        name(inst, i ? "EOI" : "Sine"); inst.writeShort((short) i);
        ibag.writeShort((short) (i * 2)); ibag.writeShort(0);
    }
    for (int i = 0; i < 10; ++i) { pmod.writeByte(0); imod.writeByte(0); }
    pgen.writeShort(41); pgen.writeShort(0); pgen.writeInt(0);
    igen.writeShort(54); igen.writeShort(1); // continuous sample loop
    igen.writeShort(53); igen.writeShort(0); igen.writeInt(0);
    for (int i = 0; i < 44100 + 46; ++i)
        samples.writeShort((short) (i < 44100 ? 16000 * std::sin(juce::MathConstants<double>::twoPi * 220 * i / 44100) : 0));
    for (int i = 0; i < 2; ++i)
    {
        name(shdr, i ? "EOS" : "Sine");
        shdr.writeInt(0); shdr.writeInt(i ? 0 : 44100); shdr.writeInt(0); shdr.writeInt(i ? 0 : 44100);
        shdr.writeInt(44100); shdr.writeByte(60); shdr.writeByte(0); shdr.writeShort(0); shdr.writeShort(1);
    }
    pdta.write("pdta", 4);
    chunk(pdta, "phdr", phdr); chunk(pdta, "pbag", pbag); chunk(pdta, "pmod", pmod); chunk(pdta, "pgen", pgen);
    chunk(pdta, "inst", inst); chunk(pdta, "ibag", ibag); chunk(pdta, "imod", imod); chunk(pdta, "igen", igen); chunk(pdta, "shdr", shdr);
    sdta.write("sdta", 4); chunk(sdta, "smpl", samples);
    body.write("sfbk", 4); chunk(body, "LIST", sdta); chunk(body, "LIST", pdta);
    chunk(riff, "RIFF", body);
    require(file.replaceWithData(riff.getData(), riff.getDataSize()), "Cannot write SoundFont fixture");
}
static double frequency(const std::vector<float>& audio, int start, int count)
{
    int crossings = 0;
    for (int i = start + 1; i < start + count; ++i) if (audio[(size_t) i - 1] <= 0 && audio[(size_t) i] > 0) ++crossings;
    return crossings * 44100.0 / count;
}
template<class Synth> static void checkAudio(Synth& synth, int blockSize)
{
    synth.prepare(44100, blockSize);
    synth.maxVoices = 1; synth.glideTime = 200;
    synth.ampAttack = synth.ampDecay = synth.ampRelease = 0; synth.ampSustain = 100;
    synth.lfo1.amount = synth.lfo2.amount = 0;
    juce::AudioBuffer<float> warmup(2, 256);
    juce::MidiBuffer midi; midi.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8) 100), 0);
    synth.processBlock(warmup, midi);
    std::vector<float> audio;
    for (int offset = 0; offset < 16000; offset += blockSize)
    {
        juce::AudioBuffer<float> block(2, juce::jmin(blockSize, 16000 - offset));
        juce::MidiBuffer events;
        if (offset == 0) events.addEvent(juce::MidiMessage::noteOn(1, 72, (juce::uint8) 100), 0);
        synth.processBlock(block, events);
        audio.insert(audio.end(), block.getReadPointer(0), block.getReadPointer(0) + block.getNumSamples());
    }
    const auto early = frequency(audio, 100, 1800), late = frequency(audio, 11000, 4000);
    std::cout << "Glide " << blockSize << ": " << early << " -> " << late << " Hz\n";
    require(early > 190 && early < 300 && late > 420 && late < 460, "Glide must audibly slide from the previous pitch and reach the target");
    synth.glideTime = 0;
    synth.lfo1.target = LFO::Target::Volume; synth.lfo1.amount = 0.8f; synth.lfo1.rate = 0.1f;
    synth.modWheel.target = 1;
    const auto measure = [&](int wheel)
    {
        synth.allNotesOff(); synth.lfo1.phase = 0.25;
        juce::AudioBuffer<float> block(2, 4096);
        juce::MidiBuffer events;
        events.addEvent(juce::MidiMessage::controllerEvent(1, 1, wheel), 0);
        events.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8) 100), 0);
        synth.processBlock(block, events);
        return block.getRMSLevel(0, 512, 3000);
    };
    const auto zero = measure(0), full = measure(127);
    require(zero > 0.001f && full > zero * 1.3f && full < zero * 1.5f, "MIDI CC1 must control actual audio modulation depth");
    synth.modWheel.target = 0;
    const auto offZero = measure(0), offFull = measure(127);
    require(std::abs(offZero - offFull) < offZero * 0.02f, "Disabling wheel mapping must restore knob-only modulation");
}
int main()
{
    juce::ScopedJuceInitialiser_GUI gui;
    for (double rate : { 44100.0, 48000.0, 96000.0 })
    {
        NoteGlide ramp; ramp.start(-12, 100, rate);
        ramp.advance((int) (rate / 20)); require(std::abs(ramp.value() + 6) < 0.01, "Glide halfway pitch must be sample-rate independent");
        ramp.advance((int) rate); require(ramp.value() == 0, "Glide must stop exactly at target");
        ramp.start(-12, 0, rate); require(ramp.value() == 0, "Zero glide must be instant");
    }
    ModWheelDepth wheel; wheel.target = 1; wheel.controller(1, 64);
    require(wheel.scale(1) > 0.5 && wheel.scale(1) < 0.51 && wheel.scale(2) == 1, "Wheel routing must leave the other LFO unchanged");
    wheel.target = 3; wheel.controller(121, 0);
    require(wheel.scale(1) == 0 && wheel.scale(2) == 0, "Reset controllers must reset modulation depth");
    const auto root = juce::File::getCurrentWorkingDirectory().getNonexistentChildFile("performance-fixtures", "", false);
    struct Cleanup { juce::File root; ~Cleanup() { root.deleteRecursively(); } } cleanup { root };
    require(root.createDirectory().wasOk(), "Cannot create fixtures");
    {
        juce::WavAudioFormat wav;
        auto out = root.getChildFile("sine.wav").createOutputStream();
        std::unique_ptr<juce::AudioFormatWriter> writer(wav.createWriterFor(out.release(), 44100, 1, 16, {}, 0));
        juce::AudioBuffer<float> samples(1, 44100);
        for (int i = 0; i < 44100; ++i) samples.setSample(0, i, (float) (0.5 * std::sin(juce::MathConstants<double>::twoPi * 220 * i / 44100)));
        require(writer && writer->writeFromAudioSampleBuffer(samples, 0, 44100), "Cannot write sine fixture");
    }
    const auto sfzFile = root.getChildFile("sine.sfz"), sf2File = root.getChildFile("sine.sf2");
    sfzFile.replaceWithText("<region> sample=sine.wav pitch_keycenter=60 loop_mode=loop_continuous loop_start=0 loop_end=44099");
    writeSoundFont(sf2File);
    for (int block : { 32, 127, 512 })
    {
        SFZSynth sfz; require(sfz.loadSFZ(sfzFile), "Cannot load SFZ fixture"); checkAudio(sfz, block);
        SF2Synth sf2; require(sf2.loadSF2(sf2File), "Cannot load SF2 fixture"); checkAudio(sf2, block);
    }
    std::cout << "Glide timing, SFZ/SF2 audio pitch, wheel depth and bypass passed.\n";
}
