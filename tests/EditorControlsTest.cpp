#include "PluginEditor.h"
#include "GifFixture.h"
#include <complex>
#include <iostream>
#include <stdexcept>

static void require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

int main()
{
    juce::ScopedJuceInitialiser_GUI gui;
    EQDisplay display;
    for (double rate : { 44100.0, 48000.0, 96000.0 })
    {
        EffectsChain fx;
        fx.driveBypass = fx.chorusBypass = fx.phaserBypass = fx.flangerBypass = fx.delayBypass = fx.reverbBypass = true;
        fx.eqBypass = false;
        fx.eqLowGain = 9; fx.eqMidGain = -6; fx.eqHighGain = 4;
        fx.prepare(rate, 16384);
        juce::AudioBuffer<float> impulse(2, 16384);
        impulse.clear(); impulse.setSample(0, 0, 1.0f); impulse.setSample(1, 0, 1.0f);
        fx.process(impulse);
        display.setResponse(rate, 9, -6, 4, true);
        for (double hz : { 40.0, 200.0, 1000.0, 4000.0, 16000.0 })
        {
            std::complex<double> amplitude{};
            for (int i = 0; i < impulse.getNumSamples(); ++i)
                amplitude += (double) impulse.getSample(0, i) * std::polar(1.0, -juce::MathConstants<double>::twoPi * hz * i / rate);
            const auto actualDb = juce::Decibels::gainToDecibels(std::abs(amplitude));
            require(std::abs(actualDb - display.responseDb(hz)) < 0.02, "EQ graph differs from the audio filter response");
        }
        for (const auto& bands : { EQBandSettings { { 600, 2200, 7500 }, { 0.5f, 2.0f, 1.5f } },
                                  EQBandSettings { { 300, 1800, 9000 }, { 2.0f, 0.5f, 0.75f } } })
        {
            fx.eqLowFreq = bands.frequencies[0]; fx.eqMidFreq = bands.frequencies[1]; fx.eqHighFreq = bands.frequencies[2];
            fx.eqWidths = bands.widths;
            impulse.clear(); impulse.setSample(0, 0, 1); impulse.setSample(1, 0, 1);
            fx.process(impulse);
            display.setResponse(rate, 9, -6, 4, true, bands);
            for (double hz : { 100.0, 600.0, 2200.0, 7500.0, 14000.0 })
            {
                std::complex<double> amplitude{};
                for (int i = 0; i < impulse.getNumSamples(); ++i)
                    amplitude += (double) impulse.getSample(0, i) * std::polar(1.0, -juce::MathConstants<double>::twoPi * hz * i / rate);
                require(std::abs(juce::Decibels::gainToDecibels(std::abs(amplitude)) - display.responseDb(hz)) < 0.03,
                    "Changing band frequency and width must update the audio and EQ graph together");
            }
        }
        display.setResponse(rate, 9, -6, 4, false);
        require(display.responseDb(1000) == 0.0, "Bypassed EQ graph must show a flat response");
    }

    require(NumericKnob::displayNumber(0.010000001) == "0.01" && NumericKnob::displayNumber(120.0) == "120" && NumericKnob::displayNumber(-0.000001) == "0", "Numeric controls must display at most two decimals without floating point noise");
    for (double sampleRate : { 22050.0, 44100.0, 96000.0 })
    {
        EQBandSettings crossed { { 20000, 20, 20 }, { 0.01f, 100, 1 } };
        crossed.constrain(sampleRate);
        require(crossed.frequencies[0] < crossed.frequencies[1] && crossed.frequencies[1] < crossed.frequencies[2]
            && crossed.frequencies[2] < sampleRate * 0.5 && crossed.widths[0] >= 0.25f && crossed.widths[1] <= 4,
            "Automation must never cross EQ bands or exceed safe frequency/width bounds");
    }
    EQDisplay spectrumTest;
    spectrumTest.setResponse(48000, 0, 0, 0, false);
    for (int i = 0; i < 2048; ++i)
        spectrumTest.pushSample(std::sin(juce::MathConstants<float>::twoPi * 32.0f * i / 2048.0f));
    require(spectrumTest.spectrumMagnitude(32) > 0.9f && spectrumTest.spectrumMagnitude(100) < 0.01f, "Live spectrum must identify the actual audio frequency even when EQ is bypassed");
    const auto root = juce::File::getCurrentWorkingDirectory().getNonexistentChildFile("editor-fixtures", "", false);
    struct Cleanup { juce::File directory; ~Cleanup() { directory.deleteRecursively(); } } cleanup { root };
    require(root.createDirectory().wasOk(), "Cannot create fixtures");
    juce::PropertiesFile::Options settings;
    settings.applicationName = "EditorTest";
    settings.filenameSuffix = ".settings";
    settings.folderName = root.getFullPathName();
    settings.osxLibrarySubFolder = "Application Support";
    require(settings.getDefaultFile().isAChildOf(root), "Test settings must stay in the fixture folder");
    DSKSFzProcessor processor;
    processor.getAppProperties().setStorageParameters(settings);
    {
        auto output = root.getChildFile("sample.wav").createOutputStream();
        juce::WavAudioFormat wav;
        std::unique_ptr<juce::AudioFormatWriter> writer(wav.createWriterFor(output.release(), 44100, 1, 16, {}, 0));
        require(writer != nullptr, "Cannot write sample");
        juce::AudioBuffer<float> silence(1, 128); silence.clear();
        writer->writeFromAudioSampleBuffer(silence, 0, 128);
    }
    root.getChildFile("Test Piano.sfz").replaceWithText("<region> sample=sample.wav key=60");
    require(processor.loadInstrument(root.getChildFile("Test Piano.sfz")), "Cannot load test instrument");
    auto& synth = processor.getSynth();
    synth.prepare(44100, 16);
    synth.maxVoices = 4;
    for (int i = 0; i < 4; ++i) synth.noteOn(1, 60, 100);
    require(synth.getActiveVoiceCount() == 4, "Polyphony fixture must start four voices");
    synth.maxVoices = 1;
    juce::AudioBuffer<float> voiceBlock(2, 16);
    juce::MidiBuffer noNotes;
    synth.processBlock(voiceBlock, noNotes);
    require(synth.getActiveVoiceCount() <= 1, "Lowering voices to one must affect already playing notes");
    for (int i = 0; i < 5; ++i) synth.noteOn(1, 60, 100);
    require(synth.getActiveVoiceCount() == 1, "New notes must respect a voice limit of one even with spare voice slots");
    synth.maxVoices = 4;
    for (int i = 0; i < 3; ++i) synth.noteOn(1, 60, 100);
    require(synth.getActiveVoiceCount() == 4, "Increasing the limit must restore polyphony");
    synth.maxVoices = 1;
    synth.noteOn(1, 60, 100);
    require(synth.getActiveVoiceCount() == 1, "Repeated reductions to one must remain reliable");
    synth.allNotesOff();
    const auto checkStartupTheme = [](DSKSFzEditor& editor, int index)
    {
        const auto& expected = DSKLookAndFeel::themePreset(index);
        auto* theme = dynamic_cast<DSKLookAndFeel*>(&editor.getLookAndFeel());
        require(theme && theme->getThemeIndex() == index, "Saved theme must restore immediately");
        LibraryPanel* library = nullptr;
        for (auto* child : editor.getChildren()) if (auto* panel = dynamic_cast<LibraryPanel*>(child)) library = panel;
        require(library, "Missing library panel");
        for (auto* child : library->getChildren())
        {
            if (auto* search = dynamic_cast<juce::TextEditor*>(child))
                require(search->findColour(juce::TextEditor::backgroundColourId) == expected.libTreeBg
                    && search->findColour(juce::TextEditor::textColourId) == expected.libText,
                    "Search must match saved theme before any manual theme change");
            if (auto* filter = dynamic_cast<juce::ComboBox*>(child))
                require(filter->findColour(juce::ComboBox::backgroundColourId) == expected.libTreeBg,
                    "Library filters must match the restored theme");
            if (auto* tree = dynamic_cast<juce::TreeView*>(child))
                require(tree->findColour(juce::TreeView::backgroundColourId) == expected.libTreeBg,
                    "Library tree must match the restored theme");
        }
    };
    processor.getAppProperties().getUserSettings()->setValue("uiTheme", 5);
    int savedWidth = 0;
    {
        DSKSFzEditor editor(processor);
        checkStartupTheme(editor, 5);
        editor.timerCallback();
        auto* editorTheme = dynamic_cast<DSKLookAndFeel*>(&editor.getLookAndFeel());
        juce::MidiKeyboardComponent* piano = nullptr;
        juce::ComboBox* wheelSelector = nullptr;
        juce::Slider* glideControl = nullptr;
        for (auto* child : editor.getChildren())
        {
            if (auto* keys = dynamic_cast<juce::MidiKeyboardComponent*>(child)) piano = keys;
            if (auto* combo = dynamic_cast<juce::ComboBox*>(child))
                if (combo->getNumItems() == 4 && combo->getItemText(0) == "Wheel: Off") wheelSelector = combo;
            if (auto* slider = dynamic_cast<juce::Slider*>(child))
                if (slider->getName() == "Glide (ms)") glideControl = slider;
        }
        require(editorTheme && piano && wheelSelector && glideControl, "Missing performance controls");
        editorTheme->setTheme(0); editor.sendLookAndFeelChange();
        require(piano->findColour(juce::MidiKeyboardComponent::whiteNoteColourId).getPerceivedBrightness() < 0.5f,
            "Dark themes must use dark piano keys");
        editorTheme->setTheme(4); editor.sendLookAndFeelChange();
        require(piano->findColour(juce::MidiKeyboardComponent::whiteNoteColourId).getPerceivedBrightness() > 0.8f,
            "Light themes must retain light piano keys");
        editorTheme->setTheme(0); editor.sendLookAndFeelChange();
        glideControl->setValue(250, juce::sendNotificationSync);
        wheelSelector->setSelectedId(4, juce::sendNotificationSync);
        require(processor.params.getRawParameterValue("glideTime")->load() == 250
            && processor.params.getRawParameterValue("modWheelTarget")->load() == 3,
            "Performance controls must update automatable parameters");
        glideControl->setValue(0, juce::sendNotificationSync);
        wheelSelector->setSelectedId(1, juce::sendNotificationSync);
        juce::Slider* voiceControl = nullptr;
        for (auto* child : editor.getChildren())
            if (auto* slider = dynamic_cast<juce::Slider*>(child))
                if (slider->getName() == "Polyphony") voiceControl = slider;
        require(voiceControl, "Voices control must exist");
        for (int value : { 1, 32, 1, 64, 1 })
        {
            voiceControl->showTextBox();
            juce::TextEditor* input = nullptr;
            for (auto* child : voiceControl->getChildren())
                if (auto* label = dynamic_cast<juce::Label*>(child)) input = label->getCurrentTextEditor();
            require(input, "Voices must allow direct text entry");
            input->setText(juce::String(value));
            editor.timerCallback();
            voiceControl->hideTextBox(false);
            require((int) voiceControl->getValue() == value
                && (int) processor.params.getRawParameterValue("maxVoices")->load() == value,
                "Typing voices must reliably commit to both display and audio parameter");
        }
        AudioVisualizer* visual = nullptr;
        EQDisplay* eq = nullptr;
        int stars = 0;
        StarButton* thirdStar = nullptr;
        LibraryPanel* library = nullptr;
        bool visualSizeButton = false, renameName = false;
        for (auto* child : editor.getChildren())
        {
            if (auto* v = dynamic_cast<AudioVisualizer*>(child)) visual = v;
            if (auto* star = dynamic_cast<StarButton*>(child))
            {
                if (star->getComponentID() != "bank-rating" && ++stars == 3) thirdStar = star;
            }
            if (auto* panel = dynamic_cast<LibraryPanel*>(child)) library = panel;
            if (auto* b = dynamic_cast<juce::TextButton*>(child))
                if (b->getButtonText() == "Visual") visualSizeButton = true;
            if (auto* label = dynamic_cast<juce::Label*>(child))
                if (label->getTooltip() == "Click to rename instrument")
                {
                    renameName = true;
                    require(!label->getText().contains("[+]"), "The old header preset selector must be removed");
                }
        }
        if (visual) for (auto* child : visual->getChildren())
            if (auto* e = dynamic_cast<EQDisplay*>(child)) eq = e;
        require(visual && eq && stars == 5 && !visualSizeButton && renameName,
                "Missing visualizer sizing, EQ graph, actual stars or header rename action");
        require(thirdStar && library, "Rating controls not found");
        const LibrarySearch::Result current { "Test Piano", {}, root.getChildFile("Test Piano.sfz") };
        thirdStar->onClick();
        require(library->instrumentInfo(current).rating == 3, "Vector stars must still set the selected rating");
        thirdStar->onClick();
        require(library->instrumentInfo(current).rating == 0, "Clicking the selected star must clear the rating");
        thirdStar->onClick();
        const int originalHeight = visual->getHeight();
        const int minimumHeight = editor.getHeight() - originalHeight;
        const int originalWidth = editor.getWidth();
        editor.setSize(editor.getWidth() + 160, editor.getHeight() + 100);
        require(visual->getHeight() == originalHeight, "Window resizing must not stretch the visual height");
        juce::TextButton* options = nullptr;
        for (auto* child : editor.getChildren())
            if (auto* button = dynamic_cast<juce::TextButton*>(child))
                if (button->getButtonText() == "Options") options = button;
        require(options && options->getBoundsInParent().getRight() >= editor.getWidth() - 12, "Options must track the widened right edge");
        const juce::File widePreview("/tmp/dsk-wide-editor.png");
        widePreview.deleteFile();
        if (auto stream = widePreview.createOutputStream())
            juce::PNGImageFormat().writeImageToStream(editor.createComponentSnapshot(editor.getLocalBounds()), *stream);
        editor.setSize(originalWidth, editor.getHeight());
        const auto outerSize = editor.getBounds();
        const auto dragStart = juce::Point<float>((float) library->getRight() + 2, 100.0f);
        auto event = [&](juce::Point<float> position, bool dragged)
        {
            return juce::MouseEvent(juce::Desktop::getInstance().getMainMouseSource(), position,
                juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier), 1, 0, 0, 0, 0, &editor, &editor,
                juce::Time::getCurrentTime(), dragStart, juce::Time::getCurrentTime(), 1, dragged);
        };
        editor.mouseDown(event(dragStart, false));
        editor.mouseDrag(event(dragStart.translated(80, 0), true));
        editor.mouseUp(event(dragStart.translated(80, 0), true));
        require(editor.getBounds() == outerSize && library->getWidth() == (int) dragStart.x - 2 + 80,
                "Dragging the left divider must only adjust the split, not resize the window");
        for (auto* child : editor.getChildren())
            if (dynamic_cast<juce::Slider*>(child) || dynamic_cast<juce::TextButton*>(child))
                require(child->getBoundsInParent().getRight() <= editor.getWidth(), "Resizing the browser must not push controls offscreen");
        const int compactHeight = minimumHeight;
        editor.setSize(editor.getWidth(), compactHeight + 32);
        require(visual->getHeight() <= 32 && visual->isVisible(), "Visualizer must support a tiny height");
        editor.setSize(editor.getWidth(), compactHeight);
        require(!visual->isVisible(), "The bottom visual must fit when the window shrinks to its minimum height");
        auto set = [&](const char* id, float value)
        {
            auto* parameter = processor.params.getParameter(id);
            parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
        };
        set("eqOn", 1); set("eqLow", 9); set("eqMid", -6); set("eqHigh", 4);
        visual->setStyle((int) AudioVisualizer::Style::EQ);
        editor.timerCallback();
        require(eq->isVisible() && eq->getParentComponent() == visual, "EQ must be an option inside the bottom visual");
        require(eq->responseDb(40) > 8 && eq->responseDb(1000) < -4, "EQ graph must follow parameter changes");
        editor.setSize(editor.getWidth(), compactHeight + originalHeight);
        require(eq->getHeight() > 0, "EQ drag test requires a visible graph");
        const auto lowStart = eq->bandPoint(0);
        const auto eqEvent = [&](juce::Point<float> point)
        {
            return juce::MouseEvent(juce::Desktop::getInstance().getMainMouseSource(), point,
                juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier), 1, 0, 0, 0, 0, eq, eq,
                juce::Time::getCurrentTime(), lowStart, juce::Time::getCurrentTime(), 1, true);
        };
        eq->mouseDown(eqEvent(lowStart));
        eq->mouseDrag(eqEvent({ (float) eq->getWidth(), lowStart.y }));
        eq->mouseUp(eqEvent(lowStart));
        require(eq->getBands().frequencies[0] > 900.0f && processor.params.getRawParameterValue("eqLowFreq")->load() > 900.0f, "Dragging Low must move its frequency in both graph and parameter");
        require(eq->getBands().frequencies[0] < eq->getBands().frequencies[1], "Dragging Low past Mid must stop before crossing");
        const auto midStart = eq->bandPoint(1);
        eq->mouseDown(eqEvent(midStart));
        eq->mouseDrag(eqEvent({ 0, midStart.y }));
        eq->mouseUp(eqEvent(midStart));
        require(eq->getBands().frequencies[1] > eq->getBands().frequencies[0], "Dragging Mid below Low must stop before crossing");
        eq->editBand(2, 20, 4, 2.5f);
        editor.timerCallback();
        require(eq->getBands().frequencies[2] > eq->getBands().frequencies[1]
            && std::abs(processor.params.getRawParameterValue("eqHighWidth")->load() - 2.5f) < 0.001f,
            "Graph edits must persist band width and enforce High above Mid");
        juce::MouseWheelDetails scroll; scroll.deltaY = 0.1f;
        const auto previousWidth = eq->getBands().widths[2];
        eq->mouseWheelMove(eqEvent(eq->bandPoint(2)), scroll);
        require(eq->getBands().widths[2] > previousWidth, "Scrolling over a band must broaden its width");

        editor.setSize(editor.getWidth(), compactHeight + originalHeight);
        const juce::File preview("/tmp/dsk-editor-controls.png");
        preview.deleteFile();
        if (auto stream = preview.createOutputStream())
            juce::PNGImageFormat().writeImageToStream(editor.createComponentSnapshot(editor.getLocalBounds()), *stream);
        const auto gifFile = root.getChildFile("Visual.gif");
        writeGifFixture(gifFile);
        require(editor.isInterestedInFileDrag({ gifFile.getFullPathName() }), "GIF drag and drop must be accepted");
        editor.filesDropped({ gifFile.getFullPathName() }, visual->getX(), visual->getY());
        for (int i = 0; i < 100 && visual->getGifFrameCount() == 0; ++i)
            juce::MessageManager::getInstance()->runDispatchLoopUntil(20);
        require(visual->getGifFrameCount() == 5 && processor.getAppProperties().getUserSettings()->getValue("visualGif") == gifFile.getFullPathName(),
                "Added GIFs must be saved in editor settings");
        visual->setStyle((int) AudioVisualizer::Style::GIF);
        savedWidth = editor.getWidth();
    }
    {
        processor.getAppProperties().getUserSettings()->setValue("uiTheme", 2);
        DSKSFzEditor restored(processor);
        checkStartupTheme(restored, 2);
        AudioVisualizer* visual = nullptr;
        for (auto* child : restored.getChildren()) if (auto* v = dynamic_cast<AudioVisualizer*>(child)) visual = v;
        require(visual && restored.getWidth() == savedWidth, "Window width must restore independently of the browser split");
        for (int i = 0; i < 100 && visual->getGifFrameCount() == 0; ++i)
            juce::MessageManager::getInstance()->runDispatchLoopUntil(20);
        require(visual->getStyle() == (int) AudioVisualizer::Style::GIF && visual->getGifFrameCount() == 5,
                "Editor reopening must restore the selected animated GIF");
    }
    processor.getAppProperties().closeFiles();
    std::cout << "EQ curve/audio agreement, bypass, automation, compact visualizer, stars and rename header checks passed.\n";
}
