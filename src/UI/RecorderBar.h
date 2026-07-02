#pragma once
#include <JuceHeader.h>

class DSKSFzProcessor;
struct DSKTheme;

// ══════════════════════════════════════════════════════════════════════════════
// RecorderBar — barra de transporte de grabación (Grabar/Reproducir/Detener
// + contador de tiempo + selector de carpeta destino, MP3). Solo se crea y
// se muestra en modo Standalone (ver DSKSFzEditor); en VST3 no existe.
// ══════════════════════════════════════════════════════════════════════════════
class RecorderBar : public juce::Component, private juce::Timer
{
public:
    explicit RecorderBar(DSKSFzProcessor& processor);
    ~RecorderBar() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void applyTheme(const DSKTheme& t);

private:
    // Botón con icono dibujado a mano (sin depender de glifos de fuente).
    class IconButton : public juce::Button
    {
    public:
        enum class Icon { Record, Play, Stop, Save, Folder };
        explicit IconButton(Icon i) : juce::Button({}), icon(i) {}

        void setActive(bool a) { active = a; repaint(); }
        void setTheme(const DSKTheme* t) { theme = t; repaint(); }

    private:
        void paintButton(juce::Graphics& g, bool highlighted, bool down) override;
        Icon icon;
        bool active { false };
        const DSKTheme* theme { nullptr };
    };

    void timerCallback() override;
    void updateStates();
    void recordClicked();
    void beginNewRecording();
    void playClicked();
    void stopClicked();
    void saveClicked();
    void chooseFolderClicked();
    static juce::String formatTime(double seconds);

    DSKSFzProcessor& proc;

    IconButton recordBtn { IconButton::Icon::Record };
    IconButton playBtn   { IconButton::Icon::Play   };
    IconButton stopBtn   { IconButton::Icon::Stop   };
    IconButton saveBtn   { IconButton::Icon::Save   };
    IconButton folderBtn { IconButton::Icon::Folder };
    juce::Label timeLabel;

    juce::File destinationFolder;
    std::unique_ptr<juce::FileChooser> fileChooser;

    bool wasRecording { false };
    bool wasPlaying   { false };
    float pulsePhase  { 0.0f };
    const DSKTheme* theme { nullptr };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RecorderBar)
};
