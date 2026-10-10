#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI/DSKLookAndFeel.h"
#include "UI/NumericKnob.h"
#include "UI/LibraryPanel.h"
#include "UI/RecorderBar.h"
#include "UI/AudioVisualizer.h"
#include "UI/StarButton.h"
#include "UI/EQDisplay.h"
#include "UI/InstrumentEditorWindow.h"

// ══════════════════════════════════════════════════════════════════════════════
// ADSRDisplay — live ADSR envelope visualiser
// ══════════════════════════════════════════════════════════════════════════════
class ADSRDisplay : public juce::Component
{
public:
    float attack = 0.001f;
    float decay = 0.1f;
    float sustain = 100.0f;
    float release = 0.05f;
    void paint(juce::Graphics& g) override;
};

// ══════════════════════════════════════════════════════════════════════════════
// DSKSFzEditor
// ══════════════════════════════════════════════════════════════════════════════
class DSKSFzEditor : public juce::AudioProcessorEditor,
    public LibraryPanel::Listener,
    public juce::FileDragAndDropTarget,
    public juce::Timer
{
public:
    std::function<void(int)> standaloneOption;
    explicit DSKSFzEditor(DSKSFzProcessor& p);
    ~DSKSFzEditor() override;

    void paint(juce::Graphics& g) override;
    void paintOverChildren(juce::Graphics& g) override;
    void resized()                   override;
    void setScaleFactor(float newScale) override;
    void timerCallback()              override;
    void lookAndFeelChanged()         override;

    void mouseMove(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    // PC keyboard → MIDI note forwarding
    bool keyPressed(const juce::KeyPress& key) override;
    bool keyStateChanged(bool isKeyDown)            override;

    // LibraryPanel::Listener
    void libraryFileRequested(const juce::File& f) override;
    void libraryZipRequested(const juce::File& f) override;

    // Options > Edit Instrument... — used by CustomStandaloneApp to gate the
    // window close button on unsaved instrument-editor changes.
    bool hasUnsavedInstrumentEdits() const { return instrumentEditorWindow != nullptr && instrumentEditorDirty; }
    void requestCloseInstrumentEditor(std::function<void(bool)> onResolved);

    // FileDragAndDropTarget
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void fileDragEnter(const juce::StringArray& files, int x, int y) override;
    void fileDragExit(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

private:
    DSKSFzProcessor& proc;
    DSKLookAndFeel   lf;

    std::atomic<int> loadGeneration{ 0 };
    juce::ThreadPool libraryLoadPool { 1 };
    void loadLibraryInstrument(const juce::File&, int bank = -1, int program = -1);

    bool isDragOver{ false };

    // Dimensiones y estados de arrastre
    int  libWidth{ 220 };
    int  pluginWidth{ 1040 };
    int  pluginHeight{ 388 }; // Altura por defecto (se calcula dinámicamente en el cpp)
    bool isDraggingDivider{ false };
    bool isDraggingHDivider{ false };
    int  libWidthAtDragStart{ 220 };
    int  visualHeightAtDragStart{ 90 };
    int  dragStartX{ 0 };
    int  dragStartY{ 0 };
    int  previewCountdown{ 0 };

    // ── Library ───────────────────────────────────────────────────────────────
    LibraryPanel libraryPanel;

    // ── Header ────────────────────────────────────────────────────────────────
    juce::HyperlinkButton titleLink;
    juce::HyperlinkButton siteLink;

    // Click the loaded instrument name to rename it.
    class ClickableLabel : public juce::Label {
    public:
        std::function<void()> onClick;
        void mouseUp(const juce::MouseEvent& e) override {
            juce::Label::mouseUp(e);
            if (isEnabled() && contains(e.getPosition()) && onClick) onClick();
        }
    };
    ClickableLabel        instrNameLabel, bankNameLabel;
    juce::Label           statusLabel;

    std::array<StarButton, 5> ratingButtons, bankRatingButtons;
    juce::TextButton categoryButton { "Category" };
    LibrarySearch::Result currentLibraryInstrument() const;

    juce::TextButton      openBtn{ "Open SFZ/SF2/DS/ZIP" };
    juce::TextButton      optionsBtn{ "Options" };

    // ── AMP section ───────────────────────────────────────────────────────────
    NumericKnob masterVolKnob, masterPanKnob;
    NumericKnob pitchTransKnob, pitchFineKnob, glideKnob;
    NumericKnob ampAKnob, ampDKnob, ampSKnob, ampRKnob;
    ADSRDisplay  adsrDisplay;

    // ── FILTER section ────────────────────────────────────────────────────────
    juce::ComboBox filtTypeBox;
    NumericKnob   filtCutKnob, filtResKnob, filtEnvKnob;
    NumericKnob   filtAKnob, filtDKnob, filtSKnob, filtRKnob;

    // ── MOD section ───────────────────────────────────────────────────────────
    juce::ComboBox modWheelBox;
    juce::ComboBox lfo1ShapeBox, lfo1TgtBox, lfo2ShapeBox, lfo2TgtBox;
    NumericKnob   lfo1RateKnob, lfo1AmtKnob, lfo2RateKnob, lfo2AmtKnob;

    // ── FX section ────────────────────────────────────────────────────────────
    juce::TextButton driveOnBtn, chorusOnBtn, phaserOnBtn, flangerOnBtn, delayOnBtn, reverbOnBtn, eqOnBtn;
    NumericKnob driveAmtKnob, driveMixKnob;
    NumericKnob choRateKnob, choDepthKnob, choMixKnob;
    NumericKnob phaRateKnob, phaDepthKnob, phaMixKnob;
    NumericKnob flgRateKnob, flgDepthKnob, flgMixKnob;
    NumericKnob delTimeKnob, delFbkKnob, delMixKnob;
    NumericKnob revSizeKnob, revDampKnob, revMixKnob;
    NumericKnob eqLowKnob, eqMidKnob, eqHighKnob;

    // ── Footer ────────────────────────────────────────────────────────────────
    NumericKnob      voicesKnob;
    int visualHeight = 90;
    int expandedVisualHeight = 90;
    void toggleCompactVoicesArea();
    void setVoicesVisualHeight(int height);
    juce::TextButton rrResetBtn{ "RR Reset" };

    // Grabador MP3 — solo se crea en modo Standalone (oculto por completo en VST3)
    std::unique_ptr<RecorderBar> recorderBar;

    // ── Visualizer ────────────────────────────────────────────────────────────
    // Ocupa el espacio libre entre la fila de Voices/Grabador y el teclado
    AudioVisualizer visualizer;

    // ── Piano keyboard ────────────────────────────────────────────────────────
    juce::MidiKeyboardComponent keyboard;

    // ── Meters ────────────────────────────────────────────────────────────────
    std::atomic<float> meterL{ 0.0f }, meterR{ 0.0f };

    // ── File chooser ──────────────────────────────────────────────────────────
    std::unique_ptr<juce::FileChooser> fileChooser;

    // ── Knob labels ───────────────────────────────────────────────────────────
    struct KnobLabel { juce::Slider* s; std::unique_ptr<juce::Label> lbl; };
    std::vector<KnobLabel> kLabels;

    // ── APVTS attachments ─────────────────────────────────────────────────────
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::vector<std::unique_ptr<SA>> sAttach;
    std::vector<std::unique_ptr<BA>> bAttach;
    std::vector<std::unique_ptr<CA>> cAttach;

    // ── Build helpers ─────────────────────────────────────────────────────────
    void buildUI();
    void addKL(juce::Slider& s, const juce::String& text);
    void mk(juce::Slider& s);
    void mkFXBtn(juce::TextButton& b, const juce::String& name);
    void sa(juce::Slider& s, const juce::String& id);
    void ba(juce::TextButton& b, const juce::String& id);
    void ca(juce::ComboBox& c, const juce::String& id);
    void applyWindowConstraints();
    void setGuiScale(float scale);
    float maximumGuiScale() const;
    float guiScale = 1.0f;
    float hostScale = 1.0f;
    void triggerPreviewNote();
    void loadInstrumentForPreview(const juce::File& f);

    void onInstrumentLoaded();

    // ── Instrument text editor (Options > Edit Instrument...) ───────────────
    bool canEditCurrentInstrument() const;
    void openInstrumentEditor();
    bool saveInstrumentEditorContents(); // returns false (and leaves window open) if the write failed
    std::unique_ptr<InstrumentEditorWindow> instrumentEditorWindow;
    juce::File instrumentEditorFile;
    bool instrumentEditorDirty { false };

    // Funciones de detección de tiradores
    bool isNearDivider(int x) const;
    bool isNearHDivider(int y) const;

    // ── Paint helpers ─────────────────────────────────────────────────────────
    void secBg(juce::Graphics& g, juce::Rectangle<int> r, const juce::String& t);
    void drawMeter(juce::Graphics& g, int x, int y, int w, int h, float lvl, const char* ch);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DSKSFzEditor)
};