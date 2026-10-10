#pragma once
#include <JuceHeader.h>
#include "AnimatedGif.h"
#include "EQDisplay.h"

// ══════════════════════════════════════════════════════════════════════════════
// AudioVisualizer
// Panel decorativo que reacciona al nivel de audio del plugin. Se coloca en el
// espacio libre entre la fila de Voices/Grabador y el teclado. Incluye un botón
// "Customize" en la esquina inferior derecha para elegir entre varios estilos de
// animación; el estilo elegido se persiste (ver PluginEditor load/saveState).
//
// Todas las posiciones internas se guardan en coordenadas normalizadas (0..1)
// para que el redimensionado del panel (arrastrar el divisor, maximizar la
// ventana standalone…) no requiera reiniciar ninguna animación.
// ══════════════════════════════════════════════════════════════════════════════
class AudioVisualizer : public juce::Component, private juce::Timer
{
public:
    enum class Style
    {
        Starfield = 0,
        Bubbles,
        Spectrum,
        Rings,
        Plasma,
        Particles,
        Orbit,
        Waveform,
        Matrix,
        EQ,
        GIF,
        NumStyles
    };

    AudioVisualizer();
    ~AudioVisualizer() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Llamado por el editor (~12Hz) con el nivel de pico suavizado L/R (0..~1.2)
    void pushLevel(float levelL, float levelR);

    // Colores del tema actual — llamado desde lookAndFeelChanged()
    void setThemeColours(bool isDark, juce::Colour bg, juce::Colour accent, juce::Colour accent2, juce::Colour sectionBg);

    void setStyle(int styleIndex);
    int  getStyle() const { return (int)style; }
    static juce::String getStyleName(int styleIndex);

    // Notifica al editor que el usuario cambió de estilo (para persistirlo)
    std::function<void(int)> onStyleChanged;
    std::function<void()> onGifLibraryChanged;
    void pushSpectrumSample(float sample) { eqDisplay.pushSample(sample); }
    EQDisplay& getEQDisplay() { return eqDisplay; }
    void setEQResponse(double rate, float low, float mid, float high, bool on, EQBandSettings bands = {})
    { eqDisplay.setResponse(rate, low, mid, high, on, bands); }
    void loadGif(const juce::File&, bool showErrors = true);
    void restoreGifs(const juce::StringArray& paths, const juce::String& selected);
    const juce::StringArray& getGifPaths() const { return gifPaths; }
    juce::String getGifPath() const { return gifPath; }
    size_t getGifFrameCount() const { return gif.frames.size(); }
    size_t getGifFrameIndex() const { return gifFrame; }


private:
    void timerCallback() override;
    void showStyleMenu();
    void ensureSeeded();
    void chooseGif();
    EQDisplay eqDisplay;
    AnimatedGif gif;
    juce::StringArray gifPaths;
    juce::String gifPath;
    size_t gifFrame = 0;
    double gifElapsedMs = 0, lastGifTick = 0;
    juce::ThreadPool gifPool { 1 };
    std::shared_ptr<std::atomic<int>> gifGeneration { std::make_shared<std::atomic<int>>(0) };
    std::unique_ptr<juce::FileChooser> gifChooser;


    void drawStarfield (juce::Graphics&, juce::Rectangle<float>);
    void drawBubbles   (juce::Graphics&, juce::Rectangle<float>);
    void drawSpectrum  (juce::Graphics&, juce::Rectangle<float>);
    void drawRings     (juce::Graphics&, juce::Rectangle<float>);
    void drawPlasma    (juce::Graphics&, juce::Rectangle<float>);
    void drawParticles (juce::Graphics&, juce::Rectangle<float>);
    void drawOrbit      (juce::Graphics&, juce::Rectangle<float>);
    void drawWaveform  (juce::Graphics&, juce::Rectangle<float>);
    void drawMatrix    (juce::Graphics&, juce::Rectangle<float>);
    void rebuildMatrixColumns();
    void resetMatrixColumn(int index);

    Style style { Style::Spectrum };
    bool  seeded { false };

    bool         darkTheme { true };
    juce::Colour bgColour    { 0xFF141928 };
    juce::Colour accentA     { 0xFF00D4FF };
    juce::Colour accentB     { 0xFFE94560 };

    float targetLevel { 0.0f };
    float smoothedLevel { 0.0f };
    float lastLevelForTransient { 0.0f };
    double t { 0.0 }; // reloj interno de la animación, en segundos

    juce::Random rng;

    struct Star     { float ang, r, speed, twinklePh; };
    struct Bubble   { float x, y, radius, speed, wobblePh; };
    struct Band     { float value, weight; };
    struct Pulse    { float radius, alpha; };
    struct Particle { float x, y, vx, vy, huePh; };
    struct OrbitDot { float baseAngle, speed, ring; };
    struct MatrixColumn { float headY, speed; int len; juce::String glyphs; double nextSwap; };

    std::vector<Star>     stars;
    std::vector<Bubble>   bubbles;
    std::vector<Band>     bands;
    std::vector<Pulse>    pulses;
    std::vector<Particle> particles;
    std::vector<OrbitDot> orbitDots;
    std::vector<float>    waveHistory;
    std::vector<MatrixColumn> matrixCols;
    float  matrixCharPxW { 15.0f };
    float  matrixCharPxH { 15.0f };
    juce::Font matrixFont { juce::Font::getDefaultMonospacedFontName(), 13.0f, juce::Font::plain };

    juce::TextButton customizeBtn { "Visuals" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioVisualizer)
};
