#pragma once
#include <JuceHeader.h>

// Handle browser arrows before JUCE's default selection-only navigation.
template <typename Base>
class InstrumentNavigationControl : public Base
{
public:
    std::function<bool(const juce::KeyPress&)> onInstrumentNavigation;
    bool keyPressed(const juce::KeyPress& key) override
    {
        if ((key.isKeyCode(juce::KeyPress::upKey) || key.isKeyCode(juce::KeyPress::downKey)
            || key.isKeyCode(juce::KeyPress::leftKey) || key.isKeyCode(juce::KeyPress::rightKey))
            && onInstrumentNavigation && onInstrumentNavigation(key)) return true;
        return Base::keyPressed(key);
    }
};
