#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "DecentSampler/DecentSamplerParser.h"

juce::String DSKSFzProcessor::getPresetMemoryKey(const juce::File& file) const
{
    auto path = file.getFullPathName();

   #if JUCE_WINDOWS
    path = path.toLowerCase();
   #endif

    return "lastPreset_"
        + juce::String::toHexString(path.hashCode64());
}

int DSKSFzProcessor::getRememberedPreset(const juce::File& file)
{
    if (auto* settings = appProperties.getUserSettings())
        return settings->getIntValue(getPresetMemoryKey(file), 0);

    return 0;
}

void DSKSFzProcessor::rememberPreset(const juce::File& file, int index)
{
    if (!file.existsAsFile())
        return;

    if (auto* settings = appProperties.getUserSettings())
    {
        settings->setValue(getPresetMemoryKey(file), index);
        settings->saveIfNeeded();
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout DSKSFzProcessor::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto makeFloat = [](const juce::String& id, const juce::String& name,
                        float lo, float hi, float def)
    {
        return std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(id, 1), name,
            juce::NormalisableRange<float>(lo, hi), def);
    };

    auto makeInt = [](const juce::String& id, const juce::String& name,
                      int lo, int hi, int def)
    {
        return std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID(id, 1), name, lo, hi, def);
    };

    auto makeBool = [](const juce::String& id, const juce::String& name, bool def)
    {
        return std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID(id, 1), name, def);
    };

    params.push_back(makeFloat("masterVolume",  "Master Volume",   -60.0f,  6.0f,   0.0f));
    params.push_back(makeFloat("masterPan",     "Master Pan",     -100.0f, 100.0f,  0.0f));

    params.push_back(makeFloat("ampAttack",  "Amp Attack",  0.001f, 10.0f, 0.003f));
    params.push_back(makeFloat("ampDecay",   "Amp Decay",   0.001f, 10.0f, 0.5f));
    params.push_back(makeFloat("ampSustain", "Amp Sustain", 0.0f,  100.0f, 70.0f));
    params.push_back(makeFloat("ampRelease", "Amp Release", 0.001f, 10.0f, 0.5f));

    params.push_back(makeInt  ("filterType",    "Filter Type",    0,   3,        0));
    params.push_back(makeFloat("filterCutoff",  "Cutoff",        20.0f, 20000.0f, 20000.0f));
    params.push_back(makeFloat("filterRes",     "Resonance",      0.1f,   20.0f,   0.707f));
    params.push_back(makeFloat("filterEnvAmt",  "Filter Env",    -1.0f,    1.0f,   0.0f));
    params.push_back(makeFloat("filterAttack",  "Flt Attack",    0.001f, 10.0f,   0.001f));
    params.push_back(makeFloat("filterDecay",   "Flt Decay",     0.001f, 10.0f,   0.1f));
    params.push_back(makeFloat("filterSustain", "Flt Sustain",   0.0f,  100.0f,  100.0f));
    params.push_back(makeFloat("filterRelease", "Flt Release",   0.001f, 10.0f,   0.1f));

    params.push_back(makeFloat("transpose", "Transpose", -24.0f,  24.0f,  0.0f));
    params.push_back(makeFloat("fineTune",  "Fine Tune", -100.0f, 100.0f, 0.0f));

    params.push_back(makeFloat("lfo1Rate",   "LFO1 Rate",   0.1f, 20.0f, 1.0f));
    params.push_back(makeFloat("lfo1Amount", "LFO1 Depth",  0.0f,  1.0f, 0.0f));
    params.push_back(makeInt  ("lfo1Shape",  "LFO1 Shape",  0,     4,    0));
    params.push_back(makeInt  ("lfo1Target", "LFO1 Target", 0,     3,    0));

    params.push_back(makeFloat("lfo2Rate",   "LFO2 Rate",   0.1f, 20.0f, 1.0f));
    params.push_back(makeFloat("lfo2Amount", "LFO2 Depth",  0.0f,  1.0f, 0.0f));
    params.push_back(makeInt  ("lfo2Shape",  "LFO2 Shape",  0,     4,    0));
    params.push_back(makeInt  ("lfo2Target", "LFO2 Target", 0,     3,    0));

    // FX enable buttons: ON = effect active, OFF (default) = bypassed
    params.push_back(makeBool ("driveOn",    "Drive Enable",  false));
    params.push_back(makeFloat("driveAmount","Drive",         0.0f, 1.0f, 0.5f));
    params.push_back(makeFloat("driveMix",   "Drive Mix",     0.0f, 1.0f, 0.35f));

    params.push_back(makeBool ("chorusOn",   "Chorus Enable", false));
    params.push_back(makeFloat("chorusRate", "Chorus Rate",   0.1f, 10.0f, 0.5f));
    params.push_back(makeFloat("chorusDepth","Chorus Depth",  0.0f,  1.0f, 0.02f));
    params.push_back(makeFloat("chorusMix",  "Chorus Mix",    0.0f,  1.0f, 0.4f));

    params.push_back(makeBool ("delayOn",      "Delay Enable",    false));
    params.push_back(makeFloat("delayTime",    "Delay Time (ms)", 1.0f, 2000.0f, 250.0f));
    params.push_back(makeFloat("delayFeedback","Delay Feedback",  0.0f, 0.95f,   0.35f));
    params.push_back(makeFloat("delayMix",     "Delay Mix",       0.0f, 1.0f,    0.3f));

    params.push_back(makeBool ("reverbOn",    "Reverb Enable",  false));
    params.push_back(makeFloat("reverbSize",  "Reverb Size",    0.0f, 1.0f, 0.6f));
    params.push_back(makeFloat("reverbDamping","Reverb Damping",0.0f, 1.0f, 0.4f));
    params.push_back(makeFloat("reverbMix",   "Reverb Mix",     0.0f, 1.0f, 0.3f));

    params.push_back(makeBool ("eqOn",    "EQ Enable",  false));
    params.push_back(makeFloat("eqLow",   "EQ Low",    -12.0f, 12.0f, 0.0f));
    params.push_back(makeFloat("eqMid",   "EQ Mid",    -12.0f, 12.0f, 0.0f));
    params.push_back(makeFloat("eqHigh",  "EQ High",   -12.0f, 12.0f, 0.0f));

    params.push_back(makeInt  ("maxVoices", "Polyphony", 1, 64, 32));


    // Append new parameters to preserve existing host parameter ordering.
    params.push_back(makeBool("phaserOn", "Phaser Enable", false));
    params.push_back(makeFloat("phaserRate", "Phaser Rate", 0.1f, 10.0f, 0.5f));
    params.push_back(makeFloat("phaserDepth", "Phaser Depth", 0.0f, 1.0f, 0.7f));
    params.push_back(makeFloat("phaserMix", "Phaser Mix", 0.0f, 1.0f, 0.5f));
    params.push_back(makeBool("flangerOn", "Flanger Enable", false));
    params.push_back(makeFloat("flangerRate", "Flanger Rate", 0.1f, 10.0f, 0.25f));
    params.push_back(makeFloat("flangerDepth", "Flanger Depth", 0.0f, 1.0f, 0.7f));
    params.push_back(makeFloat("flangerMix", "Flanger Mix", 0.0f, 1.0f, 0.5f));

    params.push_back(makeFloat("glideTime", "Glide (ms)", 0.0f, 2000.0f, 0.0f));
    params.push_back(makeInt("modWheelTarget", "Mod Wheel Depth", 0, 3, 0));
    for (const auto& band : { juce::String("Low"), juce::String("Mid"), juce::String("High") })
    {
        const float frequency = band == "Low" ? 200.0f : band == "Mid" ? 1000.0f : 4000.0f;
        params.push_back(makeFloat("eq" + band + "Freq", "EQ " + band + " Frequency", 20.0f, 20000.0f, frequency));
        params.push_back(makeFloat("eq" + band + "Width", "EQ " + band + " Width", 0.25f, 4.0f, 1.0f));
    }
    return { params.begin(), params.end() };
}

DSKSFzProcessor::DSKSFzProcessor()
    : AudioProcessor(BusesProperties()
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      params(*this, nullptr, "DSKSFzParams", createParameters())
{
    juce::PropertiesFile::Options opts;
    opts.applicationName     = "DSKSFzPlayer";
    opts.filenameSuffix      = ".settings";
    opts.folderName          = "DSK SFz player";
    opts.osxLibrarySubFolder = "Application Support";
    appProperties.setStorageParameters(opts);
}

DSKSFzProcessor::~DSKSFzProcessor() {}

void DSKSFzProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    synth.prepare(sampleRate, samplesPerBlock);
    sf2synth.prepare(sampleRate, samplesPerBlock);
    fx.prepare(sampleRate, samplesPerBlock);
    recorder.prepare(sampleRate, getTotalNumOutputChannels());
    keyboardState.reset();
}

void DSKSFzProcessor::releaseResources()
{
    allNotesOff();
    recorder.stopRecording(); // el dispositivo de audio va a cambiar/cerrarse
}

void DSKSFzProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                    juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Silence output while loading to avoid audio thread / load thread race
    if (isLoadingInstrument.load(std::memory_order_acquire))
    {
        buffer.clear();
        midiMessages.clear();
        return;
    }

    // Process on-screen keyboard MIDI events (adds to the incoming buffer)
    keyboardState.processNextMidiBuffer(midiMessages, 0, buffer.getNumSamples(), true);

    syncParametersToEngine();

    if (currentFormat == InstrumentFormat::SF2)
        sf2synth.processBlock(buffer, midiMessages);
    else
        synth.processBlock(buffer, midiMessages);
    fx.process(buffer);

    if (buffer.getNumChannels() > 0)
    {
        int start1, size1, start2, size2;
        spectrumFifo.prepareToWrite(buffer.getNumSamples(), start1, size1, start2, size2);
        for (int i = 0; i < size1 + size2; ++i)
        {
            float sample = 0.0f;
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch) sample += buffer.getSample(ch, i);
            spectrumSamples[(size_t) (i < size1 ? start1 + i : start2 + i - size1)] = sample / buffer.getNumChannels();
        }
        spectrumFifo.finishedWrite(size1 + size2);
    }
    recorder.processBlock(buffer);
}

void DSKSFzProcessor::allNotesOff()
{
    synth.allNotesOff();
    sf2synth.allNotesOff();
}

void DSKSFzProcessor::resetRoundRobin()
{
    synth.resetRoundRobin();
    sf2synth.resetRoundRobin();
}

bool DSKSFzProcessor::selectSF2Preset(int presetIndex)
{
    std::lock_guard<std::mutex> lock(loadMutex);
    isLoadingInstrument.store(true, std::memory_order_release);
    juce::Thread::sleep(30);

    sf2synth.allNotesOff();

    const bool ok = sf2synth.selectPreset(presetIndex);

    if (ok)
        rememberPreset(currentSFZFile, presetIndex);

    isLoadingInstrument.store(false, std::memory_order_release);
    return ok;
}

void DSKSFzProcessor::syncParametersToEngine()
{
    auto getF = [this](const char* id) -> float
    {
        auto* p = params.getRawParameterValue(id);
        return p ? p->load() : 0.0f;
    };
    auto getI = [this](const char* id) -> int
    {
        auto* p = params.getRawParameterValue(id);
        return p ? (int)p->load() : 0;
    };
    auto getB = [this](const char* id) -> bool
    {
        auto* p = params.getRawParameterValue(id);
        return p ? p->load() > 0.5f : false;
    };

    synth.masterVolume    = getF("masterVolume");
    synth.masterPan       = getF("masterPan");
    synth.ampAttack       = getF("ampAttack");
    synth.ampDecay        = getF("ampDecay");
    synth.ampSustain      = getF("ampSustain");
    synth.ampRelease      = getF("ampRelease");
    synth.filterCutoff    = getF("filterCutoff");
    synth.filterResonance = getF("filterRes");
    synth.filterTypeIdx   = getI("filterType");
    synth.filterEnvAmt    = getF("filterEnvAmt");
    synth.fltAttack       = getF("filterAttack");
    synth.fltDecay        = getF("filterDecay");
    synth.fltSustain      = getF("filterSustain");
    synth.fltRelease      = getF("filterRelease");
    synth.globalTranspose = getF("transpose");
    synth.globalTune      = getF("fineTune");
    synth.maxVoices       = getI("maxVoices");
    synth.glideTime = sf2synth.glideTime = getF("glideTime");
    synth.modWheel.target = sf2synth.modWheel.target = getI("modWheelTarget");

    // El motor SF2 comparte los mismos parámetros globales que SFZSynth (ver
    // SF2Synth.h): vol/pan/transpose/tune/polifonía, ADSR de amplitud, filtro
    // (aplicado en bus) y LFOs — se mantienen sincronizados con los mismos knobs.
    sf2synth.masterVolume    = synth.masterVolume;
    sf2synth.masterPan       = synth.masterPan;
    sf2synth.globalTranspose = synth.globalTranspose;
    sf2synth.globalTune      = synth.globalTune;
    sf2synth.maxVoices       = synth.maxVoices;
    sf2synth.ampAttack       = synth.ampAttack;
    sf2synth.ampDecay        = synth.ampDecay;
    sf2synth.ampSustain      = synth.ampSustain;
    sf2synth.ampRelease      = synth.ampRelease;
    sf2synth.filterCutoff    = synth.filterCutoff;
    sf2synth.filterResonance = synth.filterResonance;
    sf2synth.filterTypeIdx   = synth.filterTypeIdx;
    sf2synth.filterEnvAmt    = synth.filterEnvAmt;
    sf2synth.fltAttack       = synth.fltAttack;
    sf2synth.fltDecay        = synth.fltDecay;
    sf2synth.fltSustain      = synth.fltSustain;
    sf2synth.fltRelease      = synth.fltRelease;

    synth.lfo1.rate   = getF("lfo1Rate");
    synth.lfo1.amount = getF("lfo1Amount");
    synth.lfo1.shape  = static_cast<LFO::Shape>(getI("lfo1Shape"));
    synth.lfo1.target = static_cast<LFO::Target>(getI("lfo1Target"));
    synth.lfo2.rate   = getF("lfo2Rate");
    synth.lfo2.amount = getF("lfo2Amount");
    synth.lfo2.shape  = static_cast<LFO::Shape>(getI("lfo2Shape"));
    synth.lfo2.target = static_cast<LFO::Target>(getI("lfo2Target"));

    // Solo se copian los ajustes (no la fase en curso: cada motor corre su
    // propio LFO de forma independiente, ya que solo uno de los dos procesa
    // audio a la vez según currentFormat).
    sf2synth.lfo1.rate   = synth.lfo1.rate;
    sf2synth.lfo1.amount = synth.lfo1.amount;
    sf2synth.lfo1.shape  = synth.lfo1.shape;
    sf2synth.lfo1.target = synth.lfo1.target;
    sf2synth.lfo2.rate   = synth.lfo2.rate;
    sf2synth.lfo2.amount = synth.lfo2.amount;
    sf2synth.lfo2.shape  = synth.lfo2.shape;
    sf2synth.lfo2.target = synth.lfo2.target;

    // Enable buttons: ON = effect active (bypass = NOT enabled)
    fx.driveBypass  = !getB("driveOn");
    fx.driveAmount  = getF("driveAmount");
    fx.driveMix     = getF("driveMix");

    fx.chorusBypass = !getB("chorusOn");
    fx.chorusRate   = getF("chorusRate");
    fx.chorusDepth  = getF("chorusDepth");
    fx.chorusMix    = getF("chorusMix");

    fx.phaserBypass = !getB("phaserOn");
    fx.phaserRate = getF("phaserRate");
    fx.phaserDepth = getF("phaserDepth");
    fx.phaserMix = getF("phaserMix");

    fx.flangerBypass = !getB("flangerOn");
    fx.flangerRate = getF("flangerRate");
    fx.flangerDepth = getF("flangerDepth");
    fx.flangerMix = getF("flangerMix");

    fx.delayBypass   = !getB("delayOn");
    fx.delayTimeMs   = getF("delayTime");
    fx.delayFeedback = getF("delayFeedback");
    fx.delayMix      = getF("delayMix");

    fx.reverbBypass  = !getB("reverbOn");
    fx.reverbSize    = getF("reverbSize");
    fx.reverbDamping = getF("reverbDamping");
    fx.reverbMix     = getF("reverbMix");

    fx.eqBypass   = !getB("eqOn");
    EQBandSettings bands { { getF("eqLowFreq"), getF("eqMidFreq"), getF("eqHighFreq") },
        { getF("eqLowWidth"), getF("eqMidWidth"), getF("eqHighWidth") } };
    bands.constrain(getSampleRate());
    fx.eqLowFreq = bands.frequencies[0]; fx.eqMidFreq = bands.frequencies[1]; fx.eqHighFreq = bands.frequencies[2];
    fx.eqWidths = bands.widths;
    fx.eqLowGain  = getF("eqLow");
    fx.eqMidGain  = getF("eqMid");
    fx.eqHighGain = getF("eqHigh");
}

bool DSKSFzProcessor::loadInstrument(const juce::File& file)
{
    std::lock_guard<std::mutex> lock(loadMutex);

    // Signal audio thread to output silence during loading
    isLoadingInstrument.store(true, std::memory_order_release);

    // Wait long enough for the audio thread to complete its current block
    juce::Thread::sleep(30);

    allNotesOff();

    bool ok = false;
    if (file.hasFileExtension(".sf2"))
    {
        ok = sf2synth.loadSF2(file);
        if (ok) currentFormat = InstrumentFormat::SF2;
    }
    else if (file.hasFileExtension(".dslibrary"))
    {
        ok = loadDSLibrary(file);
    }
    else if (file.hasFileExtension(".dspreset"))
    {
        dsPresetList.clear();
        dsPresetList.push_back(file);
        ok = loadDecentSamplerPreset(file);
        if (ok) dsCurrentPresetIndex = 0;
    }
    else
    {
        ok = synth.loadSFZ(file);
        if (ok) currentFormat = InstrumentFormat::SFZ;
    }
    if (ok)
    {
        currentSFZFile = file;

        const int remembered = getRememberedPreset(file);

        if (currentFormat == InstrumentFormat::SF2)
        {
            const int count = (int) sf2synth.getPresets().size();

            if (count > 0)
            {
                const int index = juce::jlimit(0, count - 1, remembered);
                sf2synth.selectPreset(index);
            }
        }
        else if (currentFormat == InstrumentFormat::DecentSampler
                 && dsPresetList.size() > 1)
        {
            const int index = juce::jlimit(
                0,
                (int) dsPresetList.size() - 1,
                remembered);

            if (index != dsCurrentPresetIndex
                && loadDecentSamplerPreset(dsPresetList[(size_t) index]))
            {
                dsCurrentPresetIndex = index;
            }
        }
    }

    isLoadingInstrument.store(false, std::memory_order_release);
    return ok;
}

bool DSKSFzProcessor::loadDecentSamplerPreset(const juce::File& dspresetFile)
{
    std::vector<SFZRegion> regions;
    DecentSamplerParser parser;
    if (!parser.parse(dspresetFile, regions))
        return false;

    bool ok = synth.loadRegions(std::move(regions), dspresetFile.getParentDirectory(),
                                 dspresetFile.getFileNameWithoutExtension());
    if (ok) currentFormat = InstrumentFormat::DecentSampler;
    return ok;
}

bool DSKSFzProcessor::loadDSLibrary(const juce::File& dslibraryFile)
{
    // Extrae el .dslibrary (es un zip) a una carpeta cache propia de la app,
    // igual que .sf2/.sfz se cargan directamente desde disco — sin pedir al
    // usuario dónde extraer (a diferencia del flujo genérico de .zip).
    auto cacheRoot = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                          .getChildFile("DSK SFz player")
                          .getChildFile("DSLibraryCache");

    const juce::String key = dslibraryFile.getFileNameWithoutExtension()
                              + "_" + juce::String((juce::int64)dslibraryFile.getLastModificationTime()
                                                        .toMilliseconds());
    auto destDir = cacheRoot.getChildFile(key);

    if (!destDir.isDirectory() || destDir.getNumberOfChildFiles(juce::File::findFilesAndDirectories) == 0)
    {
        destDir.createDirectory();
        juce::ZipFile zip(dslibraryFile);
        if (zip.getNumEntries() == 0)
            return false;
        auto result = zip.uncompressTo(destDir);
        if (result.failed())
            return false;
    }

    juce::Array<juce::File> found;
    destDir.findChildFiles(found, juce::File::findFiles, true, "*.dspreset");
    if (found.isEmpty())
        return false;

    found.sort();
    dsPresetList.assign(found.begin(), found.end());

    if (!loadDecentSamplerPreset(dsPresetList[0]))
        return false;

    dsCurrentPresetIndex = 0;
    return true;
}

bool DSKSFzProcessor::selectDSPreset(int presetIndex)
{
    std::lock_guard<std::mutex> lock(loadMutex);
    if (presetIndex < 0 || presetIndex >= (int) dsPresetList.size())
        return false;

    isLoadingInstrument.store(true, std::memory_order_release);
    juce::Thread::sleep(30);
    allNotesOff();

    const bool ok =
        loadDecentSamplerPreset(dsPresetList[(size_t) presetIndex]);

    if (ok)
    {
        dsCurrentPresetIndex = presetIndex;
        rememberPreset(currentSFZFile, presetIndex);
    }

    isLoadingInstrument.store(false, std::memory_order_release);
    return ok;
}
void DSKSFzProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    auto state = params.copyState();
    state.setProperty("sfzPath", currentSFZFile.getFullPathName(), nullptr);
    if (currentFormat == InstrumentFormat::SF2)
        state.setProperty("sf2PresetIndex", sf2synth.getCurrentPresetIndex(), nullptr);
    if (currentFormat == InstrumentFormat::DecentSampler)
        state.setProperty("dsPresetIndex", dsCurrentPresetIndex, nullptr);

    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, dest);
}

void DSKSFzProcessor::setStateInformation(const void* data, int size)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, size));
    if (!xml) return;

    auto state = juce::ValueTree::fromXml(*xml);
    if (state.isValid()) params.replaceState(state);

    juce::String sfzPath = state.getProperty("sfzPath", "").toString();
    if (sfzPath.isNotEmpty())
    {
        juce::File sfzFile(sfzPath);
        if (sfzFile.existsAsFile() && loadInstrument(sfzFile))
        {
            int presetIndex = state.getProperty("sf2PresetIndex", -1);
            if (currentFormat == InstrumentFormat::SF2 && presetIndex >= 0)
                selectSF2Preset(presetIndex);

            int dsPresetIndex = state.getProperty("dsPresetIndex", -1);
            if (currentFormat == InstrumentFormat::DecentSampler && dsPresetIndex >= 0)
                selectDSPreset(dsPresetIndex);
        }
    }
}

juce::AudioProcessorEditor* DSKSFzProcessor::createEditor()
{
    return new DSKSFzEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DSKSFzProcessor();
}
