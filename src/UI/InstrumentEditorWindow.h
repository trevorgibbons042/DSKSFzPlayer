#pragma once
#include <JuceHeader.h>

// Top-level window used by "Options > Edit Instrument..." to show the raw
// text of the currently loaded SFZ file or Decent Sampler .dspreset XML,
// letting the user copy/paste/edit it directly and save it back to disk.
// All dirty-state tracking and save/close confirmation logic lives in the
// owner (DSKSFzEditor); this class only reports UI events via callbacks.
class InstrumentEditorWindow : public juce::DocumentWindow
{
public:
    explicit InstrumentEditorWindow(const juce::File& fileToEdit);
    ~InstrumentEditorWindow() override;

    void closeButtonPressed() override;

    juce::String getEditorText() const;
    void setStatusText(const juce::String& text);
    void setDirtyIndicator(bool isDirty);

    // Fired on every keystroke.
    std::function<void()> onTextChanged;
    // Fired when the Save button is clicked.
    std::function<void()> onSaveClicked;
    // Fired when the window's own close button is pressed; the callback
    // parameter reports whether the caller may proceed to close/destroy
    // this window (true) or the user cancelled (false).
    std::function<void(std::function<void(bool)>)> onCloseRequested;

private:
    class Content;
    Content* content { nullptr };
    juce::String baseTitle;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InstrumentEditorWindow)
};
