#pragma once
#include <JuceHeader.h>

// Draw stars directly so narrow rating buttons never turn into truncated text.
class StarButton : public juce::TextButton
{
public:
    void setFilled(bool value) { if (filled != value) { filled = value; repaint(); } }
    void paintButton(juce::Graphics& g, bool over, bool down) override
    {
        const auto bounds = getLocalBounds().toFloat().reduced(2.0f);
        const auto colour = findColour(juce::TextButton::textColourOffId)
            .withMultipliedAlpha(isEnabled() ? 1.0f : 0.4f);
        if (over || down)
        {
            g.setColour(colour.withAlpha(down ? 0.2f : 0.1f));
            g.fillRoundedRectangle(bounds, 3.0f);
        }
        drawStar(g, bounds, colour, filled);
    }
    static void drawStar(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour colour, bool filled)
    {
        juce::Path star;
        const auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.46f;
        star.addStar(bounds.getCentre(), 5, radius * 0.45f, radius);
        g.setColour(colour);
        if (filled) g.fillPath(star);
        else g.strokePath(star, juce::PathStrokeType(1.2f));
    }
private:
    bool filled = false;
};
