#pragma once
#include <JuceHeader.h>
#include "DSKLookAndFeel.h"
#include "../Effects/EQBandSettings.h"
#include "NumericKnob.h"

// Uses the same shelves, mid peak, frequencies and Q as the audio EQ.
class EQDisplay : public juce::Component, public juce::SettableTooltipClient
{
public:
    EQDisplay()
    {
        setResponse(44100.0, 0.0f, 0.0f, 0.0f, false);
        setTooltip("Drag a band to change frequency and gain. Scroll over it to change width. Right-click for exact values.");
    }
    std::function<void(int, float, float, float)> onBandEdit;
    std::function<void(int, bool)> onBandGesture;
    EQBandSettings getBands() const { return bands; }
    void setResponse(double rate, float low, float mid, float high, bool on, EQBandSettings settings = {})
    {
        rate = rate > 0.0 ? rate : 44100.0;
        settings.constrain(rate);
        if (settings == bands && lowCoeffs && sampleRate == rate && lowGain == low && midGain == mid && highGain == high && enabled == on) return;
        bands = settings;
        sampleRate = rate; lowGain = low; midGain = mid; highGain = high; enabled = on;
        using Coeffs = juce::dsp::IIR::Coefficients<double>;
        const auto freq = [rate](double hz) { return juce::jmin(hz, rate * 0.49); };
        lowCoeffs = Coeffs::makeLowShelf(rate, freq(bands.frequencies[0]), bands.q(0), juce::Decibels::decibelsToGain((double) low));
        midCoeffs = Coeffs::makePeakFilter(rate, freq(bands.frequencies[1]), bands.q(1), juce::Decibels::decibelsToGain((double) mid));
        highCoeffs = Coeffs::makeHighShelf(rate, freq(bands.frequencies[2]), bands.q(2), juce::Decibels::decibelsToGain((double) high));
        repaint();
    }
    double responseDb(double frequency) const
    {
        if (!enabled) return 0.0;
        const auto hz = juce::jlimit(1.0, sampleRate * 0.49, frequency);
        return juce::Decibels::gainToDecibels(lowCoeffs->getMagnitudeForFrequency(hz, sampleRate)
            * midCoeffs->getMagnitudeForFrequency(hz, sampleRate)
            * highCoeffs->getMagnitudeForFrequency(hz, sampleRate));
    }
    float spectrumMagnitude(int bin) const { return magnitudes[(size_t) juce::jlimit(0, 1023, bin)]; }
    void pushSample(float sample)
    {
        samples[(size_t) sampleCount++] = sample;
        if (sampleCount != 2048) return;
        sampleCount = 0;
        fftData.fill(0);
        std::copy(samples.begin(), samples.end(), fftData.begin());
        window.multiplyWithWindowingTable(fftData.data(), 2048);
        fft.performFrequencyOnlyForwardTransform(fftData.data());
        for (size_t i = 0; i < magnitudes.size(); ++i)
            magnitudes[i] = juce::jmax(fftData[i] / 512.0f, magnitudes[i] * 0.75f);
        repaint();
    }
    void paint(juce::Graphics& g) override
    {
        const auto* lf = dynamic_cast<const DSKLookAndFeel*>(&getLookAndFeel());
        const auto theme = lf ? lf->getTheme() : DSKTheme{};
        g.setColour(theme.lcdBg);
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(2), 3.0f);
        g.setFont(10.0f);
        g.setColour(enabled ? theme.accent : theme.subtext);
        const bool compact = getHeight() < 50;
        if (!compact) g.drawText(enabled ? "LIVE SPECTRUM + EQ" : "LIVE SPECTRUM (EQ BYPASSED)", 7, 3, getWidth() - 14, 12, juce::Justification::centredLeft);
        const auto plot = plotBounds();
        if (plot.getWidth() < 2 || plot.getHeight() < 2) return;
        const auto maxFreq = juce::jmin(20000.0, sampleRate * 0.49);
        const auto x = [&](double hz) { return plot.getX() + (float) (std::log(hz / 20.0) / std::log(maxFreq / 20.0)) * plot.getWidth(); };
        const auto y = [&](double db) { return plot.getCentreY() - (float) juce::jlimit(-18.0, 18.0, db) / 36.0f * plot.getHeight(); };
        g.setColour(theme.sectionBdr);
        for (double db : { -12.0, 0.0, 12.0 })
        {
            g.setColour(theme.sectionBdr);
            g.drawHorizontalLine((int) y(db), plot.getX(), plot.getRight());
            if (!compact)
            {
                g.setColour(theme.subtext);
                g.drawText(db > 0 ? "+12" : juce::String((int) db), 1, (int) y(db) - 6, 23, 12, juce::Justification::centredRight);
            }
        }
        g.setColour(theme.sectionBdr);
        for (double hz : { 100.0, 1000.0, 10000.0 })
            if (hz < maxFreq) g.drawVerticalLine((int) x(hz), plot.getY(), plot.getBottom());
        juce::Path spectrum;
        spectrum.startNewSubPath(plot.getX(), plot.getBottom());
        for (int i = 0; i <= (int) plot.getWidth(); ++i)
        {
            const double hz = 20.0 * std::pow(maxFreq / 20.0, i / (double) plot.getWidth());
            const int bin = juce::jlimit(0, 1023, (int) (hz * 2048.0 / sampleRate));
            const float db = juce::Decibels::gainToDecibels(magnitudes[(size_t) bin], -90.0f);
            spectrum.lineTo(plot.getX() + i, plot.getBottom() - juce::jlimit(0.0f, 1.0f, (db + 90.0f) / 90.0f) * plot.getHeight());
        }
        spectrum.lineTo(plot.getRight(), plot.getBottom());
        spectrum.closeSubPath();
        g.setColour(theme.accent.withAlpha(0.3f));
        g.fillPath(spectrum);
        juce::Path curve;
        for (int i = 0; i <= (int) plot.getWidth(); ++i)
        {
            const auto hz = 20.0 * std::pow(maxFreq / 20.0, i / (double) plot.getWidth());
            const auto px = plot.getX() + i;
            if (i == 0) curve.startNewSubPath(px, y(responseDb(hz)));
            else curve.lineTo(px, y(responseDb(hz)));
        }
        g.setColour(enabled ? theme.accent : theme.subtext);
        g.strokePath(curve, juce::PathStrokeType(1.5f));
        for (int band = 0; enabled && band < 3; ++band)
        {
            const auto point = bandPoint(band);
            g.setColour(theme.lcdBg);
            g.fillEllipse(point.x - 5, point.y - 5, 10, 10);
            g.setColour(theme.accent);
            g.drawEllipse(point.x - 5, point.y - 5, 10, 10, band == selectedBand ? 2.0f : 1.0f);
            if (!compact)
            {
                const char* labels[] = { "Low", "Mid", "High" };
                g.drawText(labels[band], (int) point.x - 18, (int) point.y - 18, 36, 12, juce::Justification::centred);
            }
        }
        if (enabled && !compact && selectedBand >= 0)
        {
            g.setColour(theme.subtext);
            const auto text = NumericKnob::displayNumber(bands.frequencies[(size_t) selectedBand]) + " Hz  |  Width "
                + NumericKnob::displayNumber(bands.widths[(size_t) selectedBand]);
            g.drawText(text, getWidth() / 2, 3, getWidth() / 2 - 85, 12, juce::Justification::centredRight);
        }
        g.setColour(theme.subtext);
        if (compact) return;
        g.drawText("20 Hz", (int) plot.getX(), getHeight() - 15, 45, 12, juce::Justification::centredLeft);
        g.drawText("1k", (int) x(1000) - 15, getHeight() - 15, 30, 12, juce::Justification::centred);
        g.drawText(juce::String((int) (maxFreq / 1000)) + "k", getWidth() - 43, getHeight() - 15, 35, 12, juce::Justification::centredRight);
    }
    juce::Point<float> bandPoint(int band) const
    {
        const auto plot = plotBounds();
        const double maximum = juce::jmin(20000.0, sampleRate * 0.49);
        const float gains[] = { lowGain, midGain, highGain };
        return { plot.getX() + (float) (std::log(bands.frequencies[(size_t) band] / 20.0) / std::log(maximum / 20.0)) * plot.getWidth(),
            plot.getCentreY() - gains[band] / 36.0f * plot.getHeight() };
    }
    void editBand(int band, float frequency, float gain, float width)
    {
        frequency = bands.limitFrequency(band, frequency, sampleRate);
        gain = juce::jlimit(-12.0f, 12.0f, gain);
        width = juce::jlimit(0.25f, 4.0f, width);
        auto settings = bands;
        settings.frequencies[(size_t) band] = frequency;
        settings.widths[(size_t) band] = width;
        float gains[] = { lowGain, midGain, highGain }; gains[band] = gain;
        setResponse(sampleRate, gains[0], gains[1], gains[2], true, settings);
        if (onBandEdit) onBandEdit(band, frequency, gain, width);
    }
    void mouseDown(const juce::MouseEvent& event) override
    {
        if (!enabled) return;
        selectedBand = nearestBand(event.position);
        repaint();
        if (event.mods.isPopupMenu()) { showBandEditor(selectedBand); return; }
        dragging = true;
        if (onBandGesture) onBandGesture(selectedBand, true);
    }
    void mouseDrag(const juce::MouseEvent& event) override
    {
        if (!enabled || !dragging || selectedBand < 0) return;
        const auto plot = plotBounds();
        if (plot.getWidth() <= 0 || plot.getHeight() <= 0) return;
        const auto proportion = juce::jlimit(0.0f, 1.0f, (event.position.x - plot.getX()) / plot.getWidth());
        const auto frequency = (float) (20.0 * std::pow(juce::jmin(20000.0, sampleRate * 0.49) / 20.0, proportion));
        const auto gain = (plot.getCentreY() - event.position.y) * 36.0f / plot.getHeight();
        editBand(selectedBand, frequency, gain, bands.widths[(size_t) selectedBand]);
    }
    void mouseUp(const juce::MouseEvent&) override
    {
        if (dragging && onBandGesture) onBandGesture(selectedBand, false);
        dragging = false;
    }
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override
    {
        if (!enabled || dragging) return;
        selectedBand = nearestBand(event.position);
        const float gains[] = { lowGain, midGain, highGain };
        const auto delta = wheel.deltaY != 0 ? wheel.deltaY : wheel.deltaX;
        if (onBandGesture) onBandGesture(selectedBand, true);
        editBand(selectedBand, bands.frequencies[(size_t) selectedBand], gains[selectedBand],
            bands.widths[(size_t) selectedBand] * std::exp(delta * (wheel.isReversed ? -2.0f : 2.0f)));
        if (onBandGesture) onBandGesture(selectedBand, false);
    }

private:
    EQBandSettings bands;
    int selectedBand = -1;
    bool dragging = false;
    juce::Rectangle<float> plotBounds() const
    {
        return getHeight() < 50 ? getLocalBounds().toFloat().reduced(8, 2)
            : getLocalBounds().toFloat().reduced(8, 0).withTrimmedLeft(20).withTrimmedTop(20).withTrimmedBottom(16);
    }
    int nearestBand(juce::Point<float> point) const
    {
        int nearest = 0;
        for (int i = 1; i < 3; ++i)
            if (point.getDistanceFrom(bandPoint(i)) < point.getDistanceFrom(bandPoint(nearest))) nearest = i;
        return nearest;
    }
    void showBandEditor(int band)
    {
        const char* names[] = { "Low", "Mid", "High" };
        const float gains[] = { lowGain, midGain, highGain };
        auto* dialog = new juce::AlertWindow(juce::String(names[band]) + " EQ band",
            "Frequency stays between neighboring bands. Larger width affects a broader range.", juce::MessageBoxIconType::NoIcon);
        dialog->addTextEditor("frequency", NumericKnob::displayNumber(bands.frequencies[(size_t) band]), "Frequency (Hz):");
        dialog->addTextEditor("gain", NumericKnob::displayNumber(gains[band]), "Gain (-12 to 12 dB):");
        dialog->addTextEditor("width", NumericKnob::displayNumber(bands.widths[(size_t) band]), "Width (0.25 to 4):");
        dialog->addButton("Apply", 1, juce::KeyPress(juce::KeyPress::returnKey));
        dialog->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
        juce::Component::SafePointer<EQDisplay> safeThis(this);
        dialog->enterModalState(true, juce::ModalCallbackFunction::create([safeThis, dialog, band](int result)
        {
            if (!safeThis || result != 1) return;
            float values[3]; int index = 0;
            for (const auto* id : { "frequency", "gain", "width" })
            {
                const auto text = dialog->getTextEditorContents(id).trim();
                const auto* start = text.toRawUTF8(); char* end = nullptr;
                const auto value = std::strtod(start, &end);
                if (text.isEmpty() || end == start || *end != '\0' || !std::isfinite(value) || std::abs(value) > 1.0e9)
                {
                    juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "EQ band", "Please enter valid numbers.");
                    return;
                }
                values[index++] = (float) value;
            }
            if (safeThis->onBandGesture) safeThis->onBandGesture(band, true);
            safeThis->editBand(band, values[0], values[1], values[2]);
            if (safeThis->onBandGesture) safeThis->onBandGesture(band, false);
        }), true);
    }
    juce::dsp::FFT fft { 11 };
    juce::dsp::WindowingFunction<float> window { 2048, juce::dsp::WindowingFunction<float>::hann };
    std::array<float, 2048> samples {};
    std::array<float, 4096> fftData {};
    std::array<float, 1024> magnitudes {};
    int sampleCount = 0;
    double sampleRate = 0;
    float lowGain = 0, midGain = 0, highGain = 0;
    bool enabled = false;
    juce::dsp::IIR::Coefficients<double>::Ptr lowCoeffs, midCoeffs, highCoeffs;
};
