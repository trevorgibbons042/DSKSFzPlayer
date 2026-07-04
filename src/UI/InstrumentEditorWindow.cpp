#include "InstrumentEditorWindow.h"

class InstrumentEditorWindow::Content : public juce::Component,
                                         private juce::TextEditor::Listener
{
public:
    explicit Content(InstrumentEditorWindow& ownerRef) : owner(ownerRef)
    {
        editor.setMultiLine(true, false);
        editor.setReturnKeyStartsNewLine(true);
        editor.setTabKeyUsedAsCharacter(true);
        editor.setScrollbarsShown(true);
        editor.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 13.0f, 0));
        editor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xFF0D1828));
        editor.setColour(juce::TextEditor::textColourId, juce::Colour(0xFFCCDDEE));
        editor.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xFF334466));
        editor.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(0xFF00D4FF));
        editor.addListener(this);
        addAndMakeVisible(editor);

        statusLabel.setFont(juce::Font(11.5f));
        statusLabel.setColour(juce::Label::textColourId, juce::Colour(0xFF88AACC));
        addAndMakeVisible(statusLabel);

        saveBtn.setButtonText("Save");
        saveBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF0F3460));
        saveBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFF00D4FF));
        saveBtn.onClick = [this] { if (owner.onSaveClicked) owner.onSaveClicked(); };
        addAndMakeVisible(saveBtn);

        closeBtn.setButtonText("Close");
        closeBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF0F3460));
        closeBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFF8899AA));
        closeBtn.onClick = [this] { owner.closeButtonPressed(); };
        addAndMakeVisible(closeBtn);

        setSize(760, 640);
    }

    void resized() override
    {
        auto b = getLocalBounds().reduced(8);
        auto footer = b.removeFromBottom(30);
        closeBtn.setBounds(footer.removeFromRight(90));
        footer.removeFromRight(8);
        saveBtn.setBounds(footer.removeFromRight(90));
        footer.removeFromRight(8);
        statusLabel.setBounds(footer);
        b.removeFromBottom(6);
        editor.setBounds(b);
    }

    void textEditorTextChanged(juce::TextEditor&) override
    {
        if (owner.onTextChanged)
            owner.onTextChanged();
    }

    juce::TextEditor  editor;
    juce::TextButton  saveBtn, closeBtn;
    juce::Label       statusLabel;

private:
    InstrumentEditorWindow& owner;
};

InstrumentEditorWindow::InstrumentEditorWindow(const juce::File& fileToEdit)
    : juce::DocumentWindow("Edit Instrument — " + fileToEdit.getFileName(),
                            juce::Colour(0xFF16213E),
                            juce::DocumentWindow::closeButton | juce::DocumentWindow::minimiseButton,
                            true),
      baseTitle("Edit Instrument — " + fileToEdit.getFileName())
{
    auto* c = new Content(*this);
    c->editor.setText(fileToEdit.loadFileAsString(), false);
    c->editor.moveCaretToTop(false);
    c->statusLabel.setText("Editing: " + fileToEdit.getFullPathName(), juce::dontSendNotification);
    content = c;

    setContentOwned(c, false);
    setResizable(true, false);
    setUsingNativeTitleBar(true);
    centreWithSize(760, 640);
    setVisible(true);
}

InstrumentEditorWindow::~InstrumentEditorWindow() = default;

void InstrumentEditorWindow::closeButtonPressed()
{
    if (onCloseRequested)
        onCloseRequested([](bool) {});
}

juce::String InstrumentEditorWindow::getEditorText() const
{
    return content != nullptr ? content->editor.getText() : juce::String();
}

void InstrumentEditorWindow::setStatusText(const juce::String& text)
{
    if (content != nullptr)
        content->statusLabel.setText(text, juce::dontSendNotification);
}

void InstrumentEditorWindow::setDirtyIndicator(bool isDirty)
{
    setName(isDirty ? ("• " + baseTitle) : baseTitle);
}
