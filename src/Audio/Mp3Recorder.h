#pragma once
#include <JuceHeader.h>
#include <atomic>

// ══════════════════════════════════════════════════════════════════════════════
// Mp3Recorder — grabación de la salida de audio del plugin a un buffer en
// memoria, reproducción instantánea del último take, y guardado explícito a
// MP3 bajo demanda.
//
// Solo se usa en modo Standalone (ver DSKSFzEditor / PluginProcessor). La
// codificación MP3 usa Shine (vendorizado en libs/shine, LGPL v2 — ver
// libs/shine/COPYING) porque JUCE no incluye un codificador MP3 de fábrica.
// JUCE tampoco incluye un *decodificador* MP3, así que la reproducción del
// take grabado se hace desde un buffer PCM en memoria, no releyendo el MP3.
//
// Diseño: grabar solo escribe en `recordBuffer` (real-time-safe, sin E/S de
// disco). El volcado a MP3 ocurre solo al llamar a saveToFile(), de forma
// síncrona en el hilo de mensajes — Shine es un codificador de coma fija muy
// rápido (bastante más rápido que tiempo real), así que incluso una toma de
// los kMaxRecordMinutes máximos se codifica en un tiempo despreciable para
// una acción manual de "Guardar". Esto evita tener que mantener un hilo de
// fondo abierto durante toda la grabación.
// ══════════════════════════════════════════════════════════════════════════════
class Mp3Recorder
{
public:
    Mp3Recorder();
    ~Mp3Recorder();

    // Llamar desde prepareToPlay(). No reserva el buffer de grabación
    // todavía; eso ocurre de forma perezosa en startRecording().
    void prepare(double newSampleRate, int newNumChannels);

    // Llamar al final de processBlock(), después de aplicar todos los
    // efectos. Si se está grabando, captura `buffer` sin modificarlo
    // (tap). Si se está reproduciendo, sustituye el contenido de `buffer`
    // por el take grabado. Real-time-safe: no asigna memoria ni bloquea.
    void processBlock(juce::AudioBuffer<float>& buffer);

    // Deben llamarse desde el hilo de mensajes (nunca desde el audio thread).
    bool startRecording();
    void stopRecording();
    void startPlayback();
    void stopPlayback();

    // Codifica a MP3 (128 kbps) todo el buffer grabado y lo escribe en
    // destFile, de forma síncrona. Debe llamarse desde el hilo de mensajes,
    // nunca mientras se está grabando. Devuelve false si no hay nada que
    // guardar, la frecuencia de muestreo no está soportada por Shine, o no
    // se pudo escribir el archivo.
    bool saveToFile(const juce::File& destFile);

    // Sugiere un nombre de archivo con marca de tiempo, no existente, dentro
    // de destFolder (p.ej. "DSK_SFz_Player_2026-07-02_170512.mp3").
    static juce::File suggestFileName(const juce::File& destFolder);

    bool isRecording()  const { return recording.load(std::memory_order_relaxed); }
    bool isPlaying()    const { return playing.load(std::memory_order_relaxed); }
    bool hasRecording()const { return recordedLength.load(std::memory_order_relaxed) > 0; }

    // True si hay una grabación en el buffer que todavía no se ha guardado a
    // disco con saveToFile(). Útil para avisar antes de perderla (nueva
    // grabación o cierre de la aplicación).
    bool hasUnsavedRecording() const
    {
        return recordedLength.load(std::memory_order_relaxed) > 0
            && ! saved.load(std::memory_order_relaxed);
    }

    // Se pone a true cuando una grabación se detiene sola al alcanzar el
    // límite de duración máxima. El llamador debe comprobarlo periódicamente
    // (p.ej. desde un Timer en la UI) y llamar a acknowledgeLengthLimit().
    bool didHitLengthLimit() const { return limitReached.load(std::memory_order_relaxed); }
    void acknowledgeLengthLimit() { limitReached.store(false, std::memory_order_relaxed); }

    double getRecordedSeconds() const;
    double getPlaybackPositionSeconds() const;

    // Límite de duración: el buffer se guarda en memoria como PCM float, así
    // que se mantiene moderado. Se reserva solo al pulsar Grabar, nunca antes.
    static constexpr int kMaxRecordMinutes = 10;

private:
    double sampleRate  { 44100.0 };
    int    numChannels { 2 };

    juce::AudioBuffer<float> recordBuffer;
    std::atomic<int> recordedLength   { 0 }; // muestras válidas en recordBuffer
    std::atomic<int> playbackPosition { 0 };

    std::atomic<bool> recording    { false };
    std::atomic<bool> playing      { false };
    std::atomic<bool> limitReached { false };
    std::atomic<bool> saved        { true }; // no hay nada grabado todavía

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Mp3Recorder)
};
