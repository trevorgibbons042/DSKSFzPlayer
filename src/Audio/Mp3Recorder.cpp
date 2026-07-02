#include "Mp3Recorder.h"
#include "layer3.h" // Shine encoder (vendorizado en libs/shine, LGPL v2)
#include <algorithm>
#include <vector>

Mp3Recorder::Mp3Recorder() = default;

Mp3Recorder::~Mp3Recorder()
{
    stopRecording();
}

void Mp3Recorder::prepare(double newSampleRate, int newNumChannels)
{
    sampleRate  = newSampleRate;
    numChannels = juce::jmax(1, newNumChannels);
}

void Mp3Recorder::processBlock(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();

    if (recording.load(std::memory_order_relaxed))
    {
        const int maxSamples = recordBuffer.getNumSamples();
        int pos = recordedLength.load(std::memory_order_relaxed);
        const int toCopy = juce::jmin(numSamples, maxSamples - pos);

        if (toCopy > 0)
        {
            for (int ch = 0; ch < recordBuffer.getNumChannels(); ++ch)
                recordBuffer.copyFrom(ch, pos, buffer, juce::jmin(ch, buffer.getNumChannels() - 1), 0, toCopy);
            recordedLength.store(pos + toCopy, std::memory_order_relaxed);
        }

        if (toCopy < numSamples)
        {
            // Límite de duración máxima alcanzado: dejamos de aceptar audio.
            // stopRecording() no hace falta llamarlo aquí (ya no gestiona
            // ningún recurso que cerrar); el llamador (Timer de la UI) solo
            // necesita enterarse vía didHitLengthLimit() para avisar al
            // usuario y, si quiere, guardar el resultado.
            recording.store(false, std::memory_order_relaxed);
            limitReached.store(true, std::memory_order_relaxed);
        }
        return;
    }

    if (playing.load(std::memory_order_relaxed))
    {
        const int total = recordedLength.load(std::memory_order_relaxed);
        int pos = playbackPosition.load(std::memory_order_relaxed);
        const int toPlay = juce::jmin(numSamples, total - pos);

        if (toPlay > 0)
        {
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                buffer.copyFrom(ch, 0, recordBuffer, juce::jmin(ch, recordBuffer.getNumChannels() - 1), pos, toPlay);
            if (toPlay < numSamples)
                buffer.clear(toPlay, numSamples - toPlay);
            playbackPosition.store(pos + toPlay, std::memory_order_relaxed);
        }
        else
        {
            buffer.clear();
        }

        if (pos + toPlay >= total)
            playing.store(false, std::memory_order_relaxed);
    }
}

bool Mp3Recorder::startRecording()
{
    if (recording.load(std::memory_order_relaxed))
        return false;

    // Comprobación temprana: si Shine no soporta la frecuencia de muestreo
    // actual del dispositivo, es mejor avisar ahora que al cabo de varios
    // minutos de grabación cuando el usuario pulse Guardar.
    if (shine_check_config((int) sampleRate, 128) < 0)
        return false;

    stopPlayback();

    const int maxSamples = (int) (sampleRate * 60.0 * (double) kMaxRecordMinutes);
    recordBuffer.setSize(numChannels, maxSamples, false, false, true);
    recordedLength.store(0, std::memory_order_relaxed);
    playbackPosition.store(0, std::memory_order_relaxed);
    limitReached.store(false, std::memory_order_relaxed);
    saved.store(false, std::memory_order_relaxed);

    recording.store(true, std::memory_order_relaxed);
    return true;
}

void Mp3Recorder::stopRecording()
{
    recording.store(false, std::memory_order_relaxed);
}

void Mp3Recorder::startPlayback()
{
    if (recording.load(std::memory_order_relaxed))
        return;
    if (recordedLength.load(std::memory_order_relaxed) <= 0)
        return;

    playbackPosition.store(0, std::memory_order_relaxed);
    playing.store(true, std::memory_order_relaxed);
}

void Mp3Recorder::stopPlayback()
{
    playing.store(false, std::memory_order_relaxed);
}

double Mp3Recorder::getRecordedSeconds() const
{
    return sampleRate > 0.0 ? (double) recordedLength.load(std::memory_order_relaxed) / sampleRate : 0.0;
}

double Mp3Recorder::getPlaybackPositionSeconds() const
{
    return sampleRate > 0.0 ? (double) playbackPosition.load(std::memory_order_relaxed) / sampleRate : 0.0;
}

bool Mp3Recorder::saveToFile(const juce::File& destFile)
{
    const int total = recordedLength.load(std::memory_order_relaxed);
    if (total <= 0)
        return false;

    if (shine_check_config((int) sampleRate, 128) < 0)
        return false;

    shine_config_t cfg;
    shine_set_config_mpeg_defaults(&cfg.mpeg);
    cfg.wave.samplerate = (int) sampleRate;
    cfg.wave.channels   = (numChannels == 1) ? PCM_MONO : PCM_STEREO;
    cfg.mpeg.mode       = (numChannels == 1) ? MONO : STEREO;
    cfg.mpeg.bitr       = 128;

    shine_t shine = shine_initialise(&cfg);
    if (shine == nullptr)
        return false;

    const int samplesPerPass = shine_samples_per_pass(shine);
    const int frameSamples   = samplesPerPass * numChannels;

    destFile.getParentDirectory().createDirectory();
    destFile.deleteFile();
    juce::FileOutputStream out(destFile);
    if (! out.openedOk())
    {
        shine_close(shine);
        return false;
    }

    std::vector<int16_t> chunk((size_t) frameSamples, 0);
    const int numRecordChannels = recordBuffer.getNumChannels();

    for (int pos = 0; pos < total; pos += samplesPerPass)
    {
        const int thisPass = juce::jmin(samplesPerPass, total - pos);
        if (thisPass < samplesPerPass)
            std::fill(chunk.begin(), chunk.end(), (int16_t) 0); // relleno del último frame parcial

        for (int i = 0; i < thisPass; ++i)
        {
            for (int ch = 0; ch < numChannels; ++ch)
            {
                const float* src = recordBuffer.getReadPointer(juce::jmin(ch, numRecordChannels - 1));
                const float s = juce::jlimit(-1.0f, 1.0f, src[pos + i]);
                chunk[(size_t) (i * numChannels + ch)] = (int16_t) juce::roundToInt(s * 32767.0f);
            }
        }

        int written = 0;
        auto* data = shine_encode_buffer_interleaved(shine, chunk.data(), &written);
        if (written > 0)
            out.write(data, (size_t) written);
    }

    int written = 0;
    auto* data = shine_flush(shine, &written);
    if (written > 0)
        out.write(data, (size_t) written);
    out.flush();

    shine_close(shine);

    saved.store(true, std::memory_order_relaxed);
    return true;
}

juce::File Mp3Recorder::suggestFileName(const juce::File& destFolder)
{
    const auto stamp = juce::Time::getCurrentTime().formatted("%Y-%m-%d_%H%M%S");
    return destFolder.getNonexistentChildFile("DSK_SFz_Player_" + stamp, ".mp3", false);
}
