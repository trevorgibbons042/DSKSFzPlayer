#pragma once
#include <JuceHeader.h>
#include <cmath>
#include <cstdlib>

class NumericKnob : public juce::Slider
{
public:
    static juce::String displayNumber(double value)
    {
        auto text = juce::String(std::abs(value) < 0.005 ? 0.0 : value, 2);
        return text.trimCharactersAtEnd("0").trimCharactersAtEnd(".");
    }
    juce::String getTextFromValue(double value) override
    { return displayNumber(value) + getTextValueSuffix(); }
    void mouseDown(const juce::MouseEvent& event) override
    {
        numericClick = event.mods.isPopupMenu();
        if (numericClick)
            juce::CallOutBox::launchAsynchronously(std::make_unique<ValueEditor>(*this), getScreenBounds(), nullptr);
        else
            juce::Slider::mouseDown(event);
    }
    void mouseDrag(const juce::MouseEvent& event) override
    {
        if (!numericClick) juce::Slider::mouseDrag(event);
    }
    void mouseUp(const juce::MouseEvent& event) override
    {
        if (!numericClick) juce::Slider::mouseUp(event);
        numericClick = false;
    }

private:
    bool numericClick = false;
    class ValueEditor : public juce::Component
    {
    public:
        explicit ValueEditor(NumericKnob& slider) : knob(&slider)
        {
            title.setText(slider.getName().isEmpty() ? "Enter value" : slider.getName(), juce::dontSendNotification);
            title.setFont(juce::Font(14.0f, juce::Font::bold));
            addAndMakeVisible(title);
            value.setText(displayNumber(slider.getValue()), false);
            value.setInputRestrictions(0, "0123456789.-+eE");
            value.onReturnKey = [this] { apply(); };
            value.onEscapeKey = [this] { dismiss(); };
            addAndMakeVisible(value);
            range.setFont(juce::Font(11.0f));
            range.setText("Range: " + displayNumber(slider.getMinimum()) + " to "
                          + displayNumber(slider.getMaximum()), juce::dontSendNotification);
            addAndMakeVisible(range);
            applyButton.onClick = [this] { apply(); };
            addAndMakeVisible(applyButton);
            setSize(280, 114);
            value.selectAll();
        }
        void parentHierarchyChanged() override
        {
            if (getParentComponent() != nullptr) value.grabKeyboardFocus();
        }
        void resized() override
        {
            title.setBounds(8, 4, getWidth() - 16, 24);
            value.setBounds(12, 34, getWidth() - 96, 26);
            applyButton.setBounds(getWidth() - 76, 34, 64, 26);
            range.setBounds(8, 66, getWidth() - 16, 36);
        }
    private:
        juce::Component::SafePointer<NumericKnob> knob;
        juce::Label title, range;
        juce::TextEditor value;
        juce::TextButton applyButton { "Apply" };
        void dismiss()
        {
            if (auto* popup = findParentComponentOfClass<juce::CallOutBox>()) popup->dismiss();
        }
        void apply()
        {
            if (!knob) { dismiss(); return; }
            const auto text = value.getText().trim();
            const auto* start = text.toRawUTF8();
            char* end = nullptr;
            const auto number = std::strtod(start, &end);
            if (text.isEmpty() || end == start || *end != '\0' || !std::isfinite(number)
                || number < knob->getMinimum() || number > knob->getMaximum())
            {
                range.setText("Enter a number from " + displayNumber(knob->getMinimum()) + " to "
                              + displayNumber(knob->getMaximum()), juce::dontSendNotification);
                value.grabKeyboardFocus();
                value.selectAll();
                return;
            }
            // Sends the same automation gesture and effect-enable callback as dragging.
            {
                juce::Slider::ScopedDragNotification gesture(*knob);
                knob->setValue(number, juce::sendNotificationSync);
            }
            dismiss();
        }
    };
};
