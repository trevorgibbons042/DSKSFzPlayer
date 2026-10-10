#pragma once
#include <JuceHeader.h>

struct EQBandSettings
{
    std::array<float, 3> frequencies { 200.0f, 1000.0f, 4000.0f };
    std::array<float, 3> widths { 1.0f, 1.0f, 1.0f };
    static constexpr float spacing = 1.05f;
    void constrain(double sampleRate)
    {
        const float maximum = (float) juce::jmin(20000.0, (sampleRate > 0 ? sampleRate : 44100.0) * 0.49);
        frequencies[1] = juce::jlimit(20.0f * spacing, maximum / spacing, frequencies[1]);
        frequencies[0] = juce::jlimit(20.0f, frequencies[1] / spacing, frequencies[0]);
        frequencies[2] = juce::jlimit(frequencies[1] * spacing, maximum, frequencies[2]);
        for (auto& width : widths) width = juce::jlimit(0.25f, 4.0f, width);
    }
    float limitFrequency(int band, float frequency, double sampleRate) const
    {
        const float lo = band == 0 ? 20.0f : frequencies[(size_t) band - 1] * spacing;
        const float hi = band == 2 ? (float) juce::jmin(20000.0, sampleRate * 0.49) : frequencies[(size_t) band + 1] / spacing;
        return juce::jlimit(lo, juce::jmax(lo, hi), frequency);
    }
    float q(int band) const { return 0.707f / widths[(size_t) band]; }
    bool operator==(const EQBandSettings& other) const { return frequencies == other.frequencies && widths == other.widths; }
    bool operator!=(const EQBandSettings& other) const { return !(*this == other); }
};
