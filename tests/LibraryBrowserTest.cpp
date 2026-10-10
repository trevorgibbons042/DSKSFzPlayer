#include "UI/LibraryPanel.h"
#include "UI/DSKLookAndFeel.h"
#include <iostream>
#include <stdexcept>

static void require(bool ok, const char* message)
{
    if (!ok) throw std::runtime_error(message);
}

struct Listener : LibraryPanel::Listener
{
    juce::File lastLoaded;
    void libraryFileRequested(const juce::File& file) override { lastLoaded = file; }
    void libraryZipRequested(const juce::File&) override {}
};

static void writeFont(const juce::File& file, const char* name, int bank, int program)
{
    juce::MemoryOutputStream headers;
    const auto record = [&](const char* text, int b, int p)
    {
        char bytes[20]{};
        std::strncpy(bytes, text, 20);
        headers.write(bytes, 20);
        headers.writeShort(static_cast<short>(p));
        headers.writeShort(static_cast<short>(b));
        headers.writeShort(0);
        headers.writeInt(0); headers.writeInt(0); headers.writeInt(0);
    };
    record(name, bank, program);
    record("EOP", 0, 0);
    juce::MemoryOutputStream body;
    body.write("sfbk", 4);
    // Odd-sized unknown chunk checks RIFF padding handling.
    body.write("JUNK", 4); body.writeInt(1); body.writeByte(42); body.writeByte(0);
    body.write("LIST", 4); body.writeInt(12 + static_cast<int>(headers.getDataSize()));
    body.write("pdta", 4); body.write("phdr", 4);
    body.writeInt(static_cast<int>(headers.getDataSize()));
    body.write(headers.getData(), headers.getDataSize());
    juce::MemoryOutputStream riff;
    riff.write("RIFF", 4); riff.writeInt(static_cast<int>(body.getDataSize()));
    riff.write(body.getData(), body.getDataSize());
    require(file.replaceWithData(riff.getData(), riff.getDataSize()), "Cannot write fixture");
}

int main()
{
    juce::ScopedJuceInitialiser_GUI gui;
    const auto root = juce::File::getCurrentWorkingDirectory().getNonexistentChildFile("browser-fixtures", "", false);
    struct Cleanup { juce::File directory; ~Cleanup() { directory.deleteRecursively(); } } cleanup { root };
    require(root.createDirectory().wasOk(), "Cannot create fixtures");
    writeFont(root.getChildFile("Orchestra.sf2"), "Acoustic Bass", 5, 32);
    writeFont(root.getChildFile("Synth.sf2"), "Synth Bass", 128, 38);
    writeFont(root.getChildFile("Piano.sf2"), "Grand Piano", 0, 0);
    DSKLookAndFeel theme;
    LibraryPanel panel;
    panel.setLookAndFeel(&theme);
    panel.setSize(280, 600);
    juce::TextEditor* search = nullptr;
    juce::TreeView* instruments = nullptr;
    int trees = 0;
    juce::TextButton* collapseAll = nullptr;
    juce::TextButton* refresh = nullptr;
    for (auto* child : panel.getChildren())
    {
        if (auto* editor = dynamic_cast<juce::TextEditor*>(child)) search = editor;
        if (auto* button = dynamic_cast<juce::TextButton*>(child))
        {
            if (button->getButtonText() == "Collapse all") collapseAll = button;
            if (button->getButtonText() == "R") refresh = button;
        }
        require(dynamic_cast<juce::ListBox*>(child) == nullptr, "Separate search list must be removed");
        if (auto* tree = dynamic_cast<juce::TreeView*>(child)) { instruments = tree; ++trees; }
    }
    require(search && instruments && collapseAll && refresh && trees == 1, "There must be one unified Instruments tree");
    const auto waitGroups = [&](int count)
    {
        for (int i = 0; i < 100; ++i)
        {
            juce::MessageManager::getInstance()->runDispatchLoopUntil(20);
            if (instruments->getRootItem() && instruments->getRootItem()->getNumSubItems() == count) return;
        }
        require(false, "Instrument groups did not arrive");
    };
    juce::File loaded;
    int bank = -1, program = -1;
    panel.onSoundFontPresetRequested = [&](const juce::File& file, int b, int p)
    {
        loaded = file; bank = b; program = p;
        panel.setLoadedInstrument({ {}, {}, file, b, p });
    };
    panel.addFolder(root);
    waitGroups(3);
    auto* first = instruments->getRootItem()->getSubItem(0);
    require(first->getTooltip() == "Orchestra" && first->getNumSubItems() == 1,
            "SoundFonts must appear as named groups with their instruments underneath");
    require(first->getSubItem(0)->getTooltip() == "Acoustic Bass", "Preset rows must show names without paths");
    instruments->keyPressed(juce::KeyPress(juce::KeyPress::downKey));
    require(loaded.getFileName() == "Orchestra.sf2" && bank == 5 && program == 32,
            "Down must load the exact preset in the first group");
    search->keyPressed(juce::KeyPress(juce::KeyPress::downKey));
    require(loaded.getFileName() == "Piano.sf2", "Navigation must follow the displayed group order");
    waitGroups(3);
    search->keyPressed(juce::KeyPress(juce::KeyPress::upKey));
    instruments->getRootItem()->getSubItem(1)->setOpen(false);
    search->keyPressed(juce::KeyPress(juce::KeyPress::downKey));
    require(loaded.getFileName() == "Synth.sf2" && !instruments->getRootItem()->getSubItem(1)->isOpen(),
            "Arrows must skip closed groups without opening them");
    collapseAll->onClick();
    for (int i = 0; i < 3; ++i)
        require(!instruments->getRootItem()->getSubItem(i)->isOpen(), "Collapse all must close every group");
    search->keyPressed(juce::KeyPress(juce::KeyPress::upKey));
    require(loaded.getFileName() == "Synth.sf2", "With every group closed, arrows must leave the sound unchanged");
    panel.setLoadedInstrument({ {}, {}, root.getChildFile("Orchestra.sf2"), 5, 32 });
    require(!instruments->getRootItem()->getSubItem(0)->isOpen(), "Playback updates must not reopen a group");
    refresh->onClick();
    juce::MessageManager::getInstance()->runDispatchLoopUntil(100);
    for (int i = 0; i < 3; ++i)
    {
        require(!instruments->getRootItem()->getSubItem(i)->isOpen(), "Refreshing must preserve collapsed groups");
        instruments->getRootItem()->getSubItem(i)->setOpen(true);
    }
    panel.onSoundFontPresetRequested(root.getChildFile("Piano.sf2"), 0, 0);
    search->setText("bass");
    waitGroups(2);
    require(instruments->getRootItem()->getSubItem(0)->getTooltip() == "Orchestra"
         && instruments->getRootItem()->getSubItem(1)->getTooltip() == "Synth",
            "Search must preserve SoundFont groups in the same tree");
    search->keyPressed(juce::KeyPress(juce::KeyPress::downKey));
    require(loaded.getFileName() == "Orchestra.sf2", "Filtered navigation must load a matching instrument");
    instruments->keyPressed(juce::KeyPress(juce::KeyPress::downKey));
    require(loaded.getFileName() == "Synth.sf2" && bank == 128 && program == 38,
            "Filtered Down must load the next bank's matching preset");
    instruments->keyPressed(juce::KeyPress(juce::KeyPress::upKey));
    require(loaded.getFileName() == "Orchestra.sf2", "Filtered Up must load the previous preset");
    search->setText("");
    waitGroups(3);
    search->setText("piano");
    waitGroups(1);
    require(instruments->getRootItem()->getSubItem(0)->getTooltip() == "Piano", "Search must match any instrument name");
    for (int index : { 0, 4, 5 })
    {
        theme.setTheme(index);
        panel.sendLookAndFeelChange();
        const auto& colours = theme.getTheme();
        require(search->findColour(juce::TextEditor::backgroundColourId) == colours.libTreeBg,
                "Search must match the Instruments theme");
        for (auto* child : panel.getChildren())
            if (auto* filter = dynamic_cast<juce::ComboBox*>(child))
                require(filter->findColour(juce::ComboBox::backgroundColourId) == colours.libTreeBg,
                        "Filters must match the Instruments theme");
    }
    search->setText("bass");
    waitGroups(2);
    theme.setTheme(0);
    panel.sendLookAndFeelChange();
    const auto snapshot = panel.createComponentSnapshot(panel.getLocalBounds());
    const juce::File preview("/tmp/dsk-instruments-browser.png");
    preview.deleteFile();
    if (auto output = preview.createOutputStream())
        juce::PNGImageFormat().writeImageToStream(snapshot, *output);
    search->setText("");
    waitGroups(3);
    const auto orchestra = root.getChildFile("Orchestra.sf2");
    const auto originalSize = orchestra.getSize();
    const auto folderId = panel.createSoundFontFolder("My Banks");
    panel.moveSoundFontToFolder(orchestra, folderId);
    for (int i = 0; i < 100; ++i)
    {
        juce::MessageManager::getInstance()->runDispatchLoopUntil(20);
        if (instruments->getRootItem()->getNumSubItems() == 3
            && instruments->getRootItem()->getSubItem(0)->getTooltip() == "My Banks") break;
    }
    auto* collection = instruments->getRootItem()->getSubItem(0);
    require(collection->getTooltip() == "My Banks" && collection->getNumSubItems() == 1
        && collection->getSubItem(0)->getTooltip() == "Orchestra"
        && collection->getSubItem(0)->getSubItem(0)->getTooltip() == "Acoustic Bass",
        "SoundFont folders must contain banks with their instruments underneath");
    search->setText("bass");
    waitGroups(2);
    require(instruments->getRootItem()->getSubItem(0)->getTooltip() == "My Banks",
            "Search must retain custom folder and SoundFont hierarchy");
    collapseAll->onClick();
    collection = instruments->getRootItem()->getSubItem(0);
    require(!collection->isOpen() && !collection->getSubItem(0)->isOpen(), "Collapse all must close nested groups too");
    instruments->keyPressed(juce::KeyPress(juce::KeyPress::downKey));
    collection->setOpen(true); collection->getSubItem(0)->setOpen(true);
    collection->setSelected(true, true);
    instruments->keyPressed(juce::KeyPress(juce::KeyPress::downKey));
    require(loaded == orchestra && bank == 5 && program == 32, "Navigation must find presets in nested folders");
    instruments->keyPressed(juce::KeyPress(juce::KeyPress::downKey));
    require(loaded == orchestra, "Closed SoundFont groups must still be skipped inside the folder browser");
    const auto clickStar = [&](juce::TreeViewItem* item, int star)
    {
        const auto position = juce::Point<float>((float) (juce::jmax(4, item->getItemPosition(true).getWidth() - 64) + (star - 1) * 20 + 10), 12.0f);
        const auto event = juce::MouseEvent(juce::Desktop::getInstance().getMainMouseSource(), position,
            juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier), 1, 0, 0, 0, 0, instruments, instruments,
            juce::Time::getCurrentTime(), position, juce::Time::getCurrentTime(), 1, false);
        item->itemClicked(event);
        juce::MessageManager::getInstance()->runDispatchLoopUntil(100);
    };
    panel.setInstrumentRating({ {}, {}, orchestra }, 3);
    juce::MessageManager::getInstance()->runDispatchLoopUntil(100);
    collection = instruments->getRootItem()->getSubItem(0);
    clickStar(collection->getSubItem(0), 3);
    require(panel.instrumentInfo({ {}, {}, orchestra }).rating == 0, "Clicking the current bank rating must clear it");
    juce::PropertiesFile::Options options;
    juce::PropertiesFile properties(root.getChildFile("Browser.settings"), options);
    panel.saveState(properties);
    SoundFontFolders restoredFolders;
    restoredFolders.fromJson(juce::JSON::parse(properties.getValue("soundfontFolders")));
    require(restoredFolders.all()[0].rating == 0, "Unrated folders must stay unrated");
    require(restoredFolders.folderFor(orchestra) == folderId, "SoundFont folder assignments must persist");
    require(orchestra.existsAsFile() && orchestra.getSize() == originalSize, "Organizing folders must not alter or move SoundFont files");
    panel.removeSoundFontFolder(folderId);
    search->setText("");
    waitGroups(3);
    require(instruments->getRootItem()->getSubItem(0)->getTooltip() == "Orchestra", "Removing a folder must restore its SoundFonts to the top level");
    panel.setInstrumentRating({ {}, {}, root.getChildFile("Synth.sf2") }, 5);
    panel.setInstrumentRating({ {}, {}, root.getChildFile("Piano.sf2") }, 3);
    panel.setInstrumentRating({ {}, {}, orchestra }, 3);
    juce::MessageManager::getInstance()->runDispatchLoopUntil(200);
    require(instruments->getRootItem()->getSubItem(0)->getTooltip() == "Synth"
        && instruments->getRootItem()->getSubItem(1)->getTooltip() == "Orchestra"
        && instruments->getRootItem()->getSubItem(2)->getTooltip() == "Piano", "Banks must sort by descending stars then alphabetically");
    for (int i = 0; i < 3; ++i) instruments->getRootItem()->getSubItem(i)->setOpen(true);
    panel.onSoundFontPresetRequested(root.getChildFile("Synth.sf2"), 128, 38);
    instruments->keyPressed(juce::KeyPress(juce::KeyPress::rightKey));
    require(loaded == orchestra, "Right must load the next instrument in rating order");
    search->keyPressed(juce::KeyPress(juce::KeyPress::leftKey));
    require(loaded.getFileName() == "Synth.sf2", "Left must load the previous instrument");
    instruments->getRootItem()->getSubItem(1)->setOpen(false);
    instruments->keyPressed(juce::KeyPress(juce::KeyPress::rightKey));
    require(loaded.getFileName() == "Piano.sf2" && !instruments->getRootItem()->getSubItem(1)->isOpen(),
        "Right must skip closed banks without opening them");
    instruments->keyPressed(juce::KeyPress(juce::KeyPress::leftKey));
    require(loaded.getFileName() == "Synth.sf2" && instruments->getRootItem()->getSubItem(0)->isOpen(),
        "Left must load the previous instrument without closing banks");
    panel.setInstrumentRating({ {}, {}, orchestra }, 0);
    panel.setInstrumentRating({ {}, {}, root.getChildFile("Piano.sf2") }, 0);
    const auto lastNamedFolder = panel.createSoundFontFolder("Z collection");
    juce::MessageManager::getInstance()->runDispatchLoopUntil(200);
    require(instruments->getRootItem()->getSubItem(0)->getTooltip() == "Synth"
        && instruments->getRootItem()->getSubItem(1)->getTooltip() == "Z collection"
        && instruments->getRootItem()->getSubItem(2)->getTooltip() == "Orchestra",
        "Stars must sort first, then folders before equally rated banks regardless of name");
    const LibrarySearch::Result hiddenPreset { "Acoustic Bass", {}, orchestra, 5, 32 };
    panel.setInstrumentHidden(hiddenPreset, true);
    juce::MessageManager::getInstance()->runDispatchLoopUntil(200);
    auto* hiddenRoot = instruments->getRootItem()->getSubItem(instruments->getRootItem()->getNumSubItems() - 1);
    require(hiddenRoot->getTooltip() == "Hidden instruments" && !hiddenRoot->isOpen()
        && hiddenRoot->getSubItem(0)->getTooltip() == "Orchestra",
        "Hidden instruments must move into a collapsed section at the end");
    hiddenRoot->setOpen(true);
    hiddenRoot->getSubItem(0)->setOpen(true);
    panel.onSoundFontPresetRequested(root.getChildFile("Synth.sf2"), 128, 38);
    instruments->keyPressed(juce::KeyPress(juce::KeyPress::rightKey));
    require(loaded.getFileName() == "Piano.sf2", "Navigation must skip hidden presets even when their section is open");
    panel.setInstrumentHidden(hiddenPreset, false);
    juce::MessageManager::getInstance()->runDispatchLoopUntil(200);
    require(instruments->getRootItem()->getNumSubItems() == 4
        && instruments->getRootItem()->getSubItem(2)->getTooltip() == "Orchestra"
        && instruments->getRootItem()->getSubItem(3)->getTooltip() == "Piano",
        "Restoring an instrument must remove the empty hidden section and restore its bank");
    panel.removeSoundFontFolder(lastNamedFolder);
    panel.setLookAndFeel(nullptr);
    std::cout << "Unified grouped browser, path-free rows, search, exact preset navigation and theme checks passed.\n";
}
