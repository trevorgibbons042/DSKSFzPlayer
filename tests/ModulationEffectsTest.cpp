#include "Effects/EffectsChain.h"
#include <cmath>
#include <iostream>
#include <stdexcept>

static void require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

int main()
{
    for (double rate : { 44100.0, 48000.0, 96000.0 })
    for (int blockSize : { 64, 512 })
    for (bool testPhaser : { true, false })
    {
        EffectsChain fx;
        fx.driveBypass = fx.chorusBypass = fx.delayBypass = fx.reverbBypass = fx.eqBypass = true;
        fx.phaserBypass = !testPhaser;
        fx.flangerBypass = testPhaser;
        fx.prepare(rate, blockSize);
        juce::AudioBuffer<float> buffer(2, blockSize);
        double difference = 0;
        for (int block = 0; block < 200; ++block)
        {
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < blockSize; ++i)
                    buffer.setSample(ch, i, 0.2f * std::sin(float((block * blockSize + i) * 0.13 + ch)));
            juce::AudioBuffer<float> dry;
            dry.makeCopyOf(buffer);
            fx.process(buffer);
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < blockSize; ++i)
                {
                    const auto sample = buffer.getSample(ch, i);
                    require(std::isfinite(sample) && std::abs(sample) < 2.0f, "Unstable output");
                    difference += std::abs(sample - dry.getSample(ch, i));
                }
        }
        require(difference > 1.0, "Enabled effect did not change the signal");
        fx.reset();
        buffer.clear();
        fx.process(buffer);
        require(buffer.getMagnitude(0, blockSize) < 1.0e-6f, "Reset left stale audio");
        for (bool bypass : { false, true })
        {
            fx.phaserBypass = fx.flangerBypass = bypass;
            fx.phaserMix = fx.flangerMix = bypass ? 0.5f : 0.0f;
            buffer.clear();
            buffer.setSample(0, 0, 0.5f);
            buffer.setSample(1, 0, -0.25f);
            fx.process(buffer);
            require(buffer.getSample(0, 0) == 0.5f && buffer.getSample(1, 0) == -0.25f,
                    "Bypass or zero mix changed dry audio");
            require(buffer.getMagnitude(1, blockSize - 1) == 0.0f, "Bypassed effect emitted a tail");
        }
    }
    std::cout << "Phaser and flanger: signal, stability, reset, bypass and zero-mix checks passed at three sample rates and two block sizes.\n";
}
