#include "DSPresetPicker.h"

DSPresetPicker::DSPresetPicker(const std::vector<juce::File>& presets, int currentIndex)
    : allPresets(presets), currentPresetIndex(currentIndex)
{
    searchBox.setTextToShowWhenEmpty("Search presets...", juce::Colours::grey);
    searchBox.addListener(this);
    addAndMakeVisible(searchBox);

    listBox.setModel(this);
    listBox.setRowHeight(22);
    listBox.setColour(juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(listBox);

    rebuildFilteredList();

    const int visibleRows = juce::jlimit(6, 14, (int)allPresets.size());
    const int headerH = 8 + 26 + 4;
    setSize(320, headerH + visibleRows * 22 + 8);
}

void DSPresetPicker::paint(juce::Graphics& g)
{
    g.fillAll(findColour(juce::PopupMenu::backgroundColourId));
}

void DSPresetPicker::resized()
{
    auto r = getLocalBounds().reduced(6);
    searchBox.setBounds(r.removeFromTop(26));
    r.removeFromTop(4);
    listBox.setBounds(r);
}

void DSPresetPicker::textEditorTextChanged(juce::TextEditor&)
{
    rebuildFilteredList();
}

void DSPresetPicker::rebuildFilteredList()
{
    const juce::String query = searchBox.getText().trim().toLowerCase();

    filtered.clear();
    for (int i = 0; i < (int)allPresets.size(); ++i)
    {
        auto name = allPresets[(size_t)i].getFileNameWithoutExtension();
        if (query.isNotEmpty() && !name.toLowerCase().contains(query)) continue;
        filtered.push_back(i);
    }

    listBox.updateContent();
    listBox.repaint();
}

int DSPresetPicker::getNumRows()
{
    return (int)filtered.size();
}

void DSPresetPicker::paintListBoxItem(int row, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    if (row < 0 || row >= (int)filtered.size()) return;
    const int idx = filtered[(size_t)row];
    const bool isCurrent = (idx == currentPresetIndex);

    if (rowIsSelected)
        g.fillAll(findColour(juce::PopupMenu::highlightedBackgroundColourId));

    g.setColour(isCurrent ? findColour(juce::PopupMenu::highlightedTextColourId)
                          : findColour(juce::PopupMenu::textColourId));
    g.setFont(juce::Font(13.0f, isCurrent ? juce::Font::bold : juce::Font::plain));
    g.drawText(allPresets[(size_t)idx].getFileNameWithoutExtension(), 6, 0, width - 8, height,
               juce::Justification::centredLeft);
}

void DSPresetPicker::listBoxItemClicked(int row, const juce::MouseEvent&)
{
    if (row < 0 || row >= (int)filtered.size()) return;
    choose(filtered[(size_t)row]);
}

void DSPresetPicker::choose(int presetIndex)
{
    if (onPresetChosen) onPresetChosen(presetIndex);
    if (auto* cb = findParentComponentOfClass<juce::CallOutBox>())
        cb->dismiss();
}
