#pragma once
#include <algorithm>
#include <cmath>

// A linear glide in semitones, with a duration independent of audio block size.
class NoteGlide
{
public:
    void start(double offset, double milliseconds, double sampleRate)
    {
        remaining = milliseconds > 0.0 ? std::max(1, (int) std::lround(milliseconds * sampleRate / 1000.0)) : 0;
        current = remaining ? offset : 0.0;
        step = remaining ? -current / remaining : 0.0;
    }
    double value() const { return current; }
    double advance(int samples = 1)
    {
        const auto result = current;
        const int count = std::min(remaining, samples);
        current += step * count;
        remaining -= count;
        if (!remaining) current = 0.0;
        return result;
    }
private:
    double current = 0.0, step = 0.0;
    int remaining = 0;
};

class ModWheelDepth
{
public:
    // 0 = off, 1 = LFO 1, 2 = LFO 2, 3 = both. Knobs set the maximum depth.
    int target = 0;
    void controller(int cc, int value)
    {
        if (cc == 1) wheel = std::clamp(value, 0, 127) / 127.0;
        else if (cc == 121) wheel = 0.0;
    }
    double scale(int lfo) const { return target == lfo || target == 3 ? wheel : 1.0; }
private:
    double wheel = 0.0;
};
