#pragma once
#include <JuceHeader.h>
#include <vector>

// ══════════════════════════════════════════════════════════════════════════════
// DSPresetPicker — contenido de un CallOutBox para elegir preset dentro de un
// .dslibrary (paquete Decent Sampler con varios .dspreset). Misma idea que
// SF2PresetPicker pero para una lista plana de archivos .dspreset.
// ══════════════════════════════════════════════════════════════════════════════
class DSPresetPicker : public juce::Component,
                        private juce::TextEditor::Listener,
                        private juce::ListBoxModel
{
public:
    DSPresetPicker(const std::vector<juce::File>& presets, int currentPresetIndex);

    std::function<void(int presetIndex)> onPresetChosen;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void textEditorTextChanged(juce::TextEditor&) override;

    int  getNumRows() override;
    void paintListBoxItem(int row, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    void listBoxItemClicked(int row, const juce::MouseEvent&) override;

    void rebuildFilteredList();
    void choose(int presetIndex);

    const std::vector<juce::File>& allPresets;
    int currentPresetIndex;
    std::vector<int> filtered; // indices into allPresets

    juce::TextEditor searchBox;
    juce::ListBox    listBox;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DSPresetPicker)
};
