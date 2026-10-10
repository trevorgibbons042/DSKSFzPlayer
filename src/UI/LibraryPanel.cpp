#include <JuceHeader.h>
#include "LibraryPanel.h"
#include "DSKLookAndFeel.h"
#include "StarButton.h"

// ── theme helper —————————————————————————————————————————————————————————————
static DSKTheme libTheme(const juce::Component* c)
{
    if (c)
        if (auto* lf = dynamic_cast<const DSKLookAndFeel*>(&c->getLookAndFeel()))
            return lf->getTheme();
    return DSKLookAndFeel::themePreset(0);
}

static DSKTheme libTheme(const juce::TreeViewItem& item)
{
    return libTheme(item.getOwnerView());
}

// helper: fire a callback deferred to the next message loop tick
static void callLater(std::function<void()> fn)
{
    juce::MessageManager::callAsync(std::move(fn));
}

// ══════════════════════════════════════════════════════════════════════════════
// FavoritesManager
// ══════════════════════════════════════════════════════════════════════════════
void FavoritesManager::save(juce::PropertiesFile& props) const
{
    // root entries
    props.setValue("fav_root_count", (int)rootEntries.size());
    for (int i = 0; i < (int)rootEntries.size(); ++i)
    {
        juce::String p = "fav_root_" + juce::String(i);
        props.setValue(p + "_name", rootEntries[i].name);
        props.setValue(p + "_path", rootEntries[i].file.getFullPathName());
    }

    // folders
    props.setValue("fav_count", (int)folders.size());
    for (int i = 0; i < (int)folders.size(); ++i)
    {
        const auto& f          = folders[i];
        const juce::String pfx = "fav_" + juce::String(i);
        props.setValue(pfx + "_name",  f.name);
        props.setValue(pfx + "_count", (int)f.entries.size());
        for (int j = 0; j < (int)f.entries.size(); ++j)
        {
            const juce::String ep = pfx + "_" + juce::String(j);
            props.setValue(ep + "_name", f.entries[j].name);
            props.setValue(ep + "_path", f.entries[j].file.getFullPathName());
        }
    }
}

void FavoritesManager::load(juce::PropertiesFile& props)
{
    rootEntries.clear();
    const int rc = props.getIntValue("fav_root_count", 0);
    for (int i = 0; i < rc; ++i)
    {
        juce::String p = "fav_root_" + juce::String(i);
        Entry e;
        e.name = props.getValue(p + "_name", "");
        e.file = juce::File(props.getValue(p + "_path", ""));
        if (e.name.isNotEmpty() && e.file != juce::File())
            rootEntries.push_back(e);
    }

    folders.clear();
    const int count = props.getIntValue("fav_count", 0);
    for (int i = 0; i < count; ++i)
    {
        const juce::String pfx = "fav_" + juce::String(i);
        Folder f;
        f.name = props.getValue(pfx + "_name", "");
        if (f.name.isEmpty()) continue;
        const int ec = props.getIntValue(pfx + "_count", 0);
        for (int j = 0; j < ec; ++j)
        {
            const juce::String ep = pfx + "_" + juce::String(j);
            Entry e;
            e.name = props.getValue(ep + "_name", "");
            e.file = juce::File(props.getValue(ep + "_path", ""));
            if (e.name.isNotEmpty() && e.file != juce::File())
                f.entries.push_back(e);
        }
        folders.push_back(f);
    }
}

bool FavoritesManager::addRootEntry(const juce::String& name, const juce::File& file)
{
    for (auto& e : rootEntries) if (e.file == file) return false;
    rootEntries.push_back({ name, file });
    return true;
}

bool FavoritesManager::removeRootEntry(int idx)
{
    if (idx < 0 || idx >= (int)rootEntries.size()) return false;
    rootEntries.erase(rootEntries.begin() + idx);
    return true;
}

bool FavoritesManager::addFolder(const juce::String& name)
{
    if (name.isEmpty() || findFolder(name) >= 0) return false;
    folders.push_back({ name, {} });
    return true;
}

bool FavoritesManager::removeFolder(int idx)
{
    if (idx < 0 || idx >= (int)folders.size()) return false;
    folders.erase(folders.begin() + idx);
    return true;
}

bool FavoritesManager::renameFolder(int idx, const juce::String& newName)
{
    if (idx < 0 || idx >= (int)folders.size() || newName.isEmpty()) return false;
    if (findFolder(newName) >= 0 && findFolder(newName) != idx) return false;
    folders[idx].name = newName;
    return true;
}

bool FavoritesManager::addEntry(int folderIdx, const juce::String& name, const juce::File& file)
{
    if (folderIdx < 0 || folderIdx >= (int)folders.size()) return false;
    for (auto& e : folders[folderIdx].entries) if (e.file == file) return false;
    folders[folderIdx].entries.push_back({ name, file });
    return true;
}

bool FavoritesManager::removeEntry(int folderIdx, int entryIdx)
{
    if (folderIdx < 0 || folderIdx >= (int)folders.size()) return false;
    auto& v = folders[folderIdx].entries;
    if (entryIdx < 0 || entryIdx >= (int)v.size()) return false;
    v.erase(v.begin() + entryIdx);
    return true;
}

bool FavoritesManager::moveEntry(int fromFolder, int entryIdx, int toFolder)
{
    if (fromFolder < 0 || fromFolder >= (int)folders.size()) return false;
    if (toFolder   < 0 || toFolder   >= (int)folders.size()) return false;
    if (fromFolder == toFolder) return false;
    auto& src = folders[fromFolder].entries;
    if (entryIdx < 0 || entryIdx >= (int)src.size()) return false;
    Entry e = src[entryIdx];
    src.erase(src.begin() + entryIdx);
    folders[toFolder].entries.push_back(e);
    return true;
}

int FavoritesManager::findFolder(const juce::String& name) const
{
    for (int i = 0; i < (int)folders.size(); ++i)
        if (folders[i].name == name) return i;
    return -1;
}

// ══════════════════════════════════════════════════════════════════════════════
// LibraryTreeItem
// ══════════════════════════════════════════════════════════════════════════════
LibraryTreeItem::LibraryTreeItem(Type t, const juce::File& f)
    : type(t), file(f) {}

juce::String LibraryTreeItem::getUniqueName() const { return file.getFullPathName(); }

bool LibraryTreeItem::mightContainSubItems()
{
    return type == Type::RootFolder || type == Type::SubFolder;
}

void LibraryTreeItem::itemOpennessChanged(bool isOpen)
{
    if (file == juce::File()) return;
    if (isOpen) populateChildren();
    else        clearSubItems();
}

void LibraryTreeItem::populateChildren()
{
    clearSubItems();
    juce::Array<juce::File> children;
    file.findChildFiles(children,
        juce::File::findFilesAndDirectories | juce::File::ignoreHiddenFiles, false);

    std::sort(children.begin(), children.end(),
        [](const juce::File& a, const juce::File& b)
        {
            if (a.isDirectory() && !b.isDirectory()) return true;
    if (!a.isDirectory() && b.isDirectory()) return false;
    return a.getFileName().compareIgnoreCase(b.getFileName()) < 0;
        });

    for (auto& child : children)
    {
        if (child.isDirectory())
        {
            // Escanear si hay algún archivo jugable dentro de esta carpeta (o sus subcarpetas)
            juce::Array<juce::File> playableFiles;
            child.findChildFiles(playableFiles, juce::File::findFiles, true,
                "*.sfz;*.sf2;*.zip;*.dspreset;*.dslibrary;*.SFZ;*.SF2;*.ZIP;*.DSPRESET;*.DSLIBRARY");

            // Solo mostramos la carpeta si tiene al menos un archivo jugable dentro
            if (!playableFiles.isEmpty())
            {
                auto* item = new LibraryTreeItem(Type::SubFolder, child);
                item->onFileActivated = onFileActivated;
                item->onSFZRightClicked = onSFZRightClicked;
                item->displayName = displayName;
                item->onSFZPreview = onSFZPreview;
                addSubItem(item);
            }
        }
        else
        {
            juce::String ext = child.getFileExtension().toLowerCase();
            if (ext == ".sfz" || ext == ".sf2" || ext == ".zip" || ext == ".dspreset" || ext == ".dslibrary")
            {
                Type childType = ext == ".sfz"       ? Type::SFZFile
                                : ext == ".sf2"       ? Type::SF2File
                                : ext == ".dspreset"  ? Type::DSFile
                                : ext == ".dslibrary" ? Type::DSLibFile
                                :                        Type::ZIPFile;
                auto* item = new LibraryTreeItem(childType, child);
                item->onFileActivated = onFileActivated;
                item->onSFZRightClicked = onSFZRightClicked;
                item->displayName = displayName;
                item->onSFZPreview = onSFZPreview;
                addSubItem(item);
            }
        }
    }
}

void LibraryTreeItem::paintItem(juce::Graphics& g, int w, int h)
{
    const auto t = libTheme(*this);
    if (isSelected()) g.fillAll(t.libSel);

    juce::String icon;
    juce::Colour col = t.libText;
    switch (type)
    {
    case Type::RootFolder: icon = "[R]"; col = t.accent;   break;
    case Type::SubFolder:  icon = "[F]"; col = t.libDim;   break;
    case Type::SFZFile:    icon = "[S]"; col = t.accent2;  break;
    case Type::SF2File:    icon = "[2]"; col = t.favAmber; break;
    case Type::ZIPFile:    icon = "[Z]"; col = t.accent;   break;
    case Type::DSFile:     icon = "[D]"; col = t.favGold;  break;
    case Type::DSLibFile:  icon = "[L]"; col = t.favGold;  break;
    }

    g.setColour(col);
    g.setFont(juce::Font(11.5f, juce::Font::bold));
    g.drawText(icon, 2, 0, 24, h, juce::Justification::centred);

    g.setColour(t.libText);
    g.setFont(juce::Font(12.5f));
    g.drawText(displayName ? displayName(file) : file.getFileName(), 28, 0, w - 30, h, juce::Justification::centredLeft);
}

void LibraryTreeItem::itemDoubleClicked(const juce::MouseEvent&)
{
    if (type == Type::RootFolder || type == Type::SubFolder)
        setOpen(!isOpen());
    else if (onFileActivated)
        onFileActivated(file, type == Type::ZIPFile);
}

void LibraryTreeItem::itemClicked(const juce::MouseEvent& e)
{
    const bool isFavoritable = type == Type::SFZFile || type == Type::SF2File
                             || type == Type::DSFile  || type == Type::DSLibFile;
    if (e.mods.isRightButtonDown())
    {
        if (isFavoritable && onSFZRightClicked)
            onSFZRightClicked(file, e);
        return;
    }
    if (isFavoritable && e.x < 28 && onSFZPreview)
        onSFZPreview(file);
}

// ══════════════════════════════════════════════════════════════════════════════
// FavoritesFileItem  (folderIdx == -1 → root entry)
// ══════════════════════════════════════════════════════════════════════════════
FavoritesFileItem::FavoritesFileItem(FavoritesManager* fm, int fi, int ei)
    : favorites(fm), folderIdx(fi), entryIdx(ei) {}

juce::String FavoritesFileItem::getUniqueName() const
{
    return "__FAV_FILE_" + juce::String(folderIdx) + "_" + juce::String(entryIdx);
}

static bool favGetEntry(FavoritesManager* fav, int fi, int ei,
                        FavoritesManager::Entry& out)
{
    if (fi == -1)
    {
        if (ei < 0 || ei >= (int)fav->rootEntries.size()) return false;
        out = fav->rootEntries[ei];
        return true;
    }
    if (fi < 0 || fi >= (int)fav->folders.size()) return false;
    auto& folder = fav->folders[fi];
    if (ei < 0 || ei >= (int)folder.entries.size()) return false;
    out = folder.entries[ei];
    return true;
}

void FavoritesFileItem::paintItem(juce::Graphics& g, int w, int h)
{
    const auto t = libTheme(*this);
    if (isSelected()) g.fillAll(t.favSel);

    FavoritesManager::Entry entry;
    if (!favGetEntry(favorites, folderIdx, entryIdx, entry)) return;

    g.setColour(t.favGold);
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.drawText("[*]", 4, 0, 18, h, juce::Justification::centred); // Cambiado a un asterisco estándar

    g.setColour(t.libText);
    g.setFont(juce::Font(12.5f));
    g.drawText(entry.name, 28, 0, w - 30, h, juce::Justification::centredLeft);
}

void FavoritesFileItem::itemDoubleClicked(const juce::MouseEvent&)
{
    FavoritesManager::Entry e;
    if (favGetEntry(favorites, folderIdx, entryIdx, e) && onActivated)
        onActivated(e.file);
}

void FavoritesFileItem::itemClicked(const juce::MouseEvent& e)
{
    if (!e.mods.isRightButtonDown())
    {
        if (e.x < 28 && onSFZPreview)
        {
            FavoritesManager::Entry entry;
            if (favGetEntry(favorites, folderIdx, entryIdx, entry))
                onSFZPreview(entry.file);
        }
        return;
    }

    FavoritesManager::Entry entry;
    if (!favGetEntry(favorites, folderIdx, entryIdx, entry)) return;

    const juce::String instrName = entry.name;
    FavoritesManager*  fav       = favorites;
    const int          fi        = folderIdx;
    const int          ei        = entryIdx;
    auto               onChange  = onChanged;
    auto               onAct     = onActivated;

    juce::PopupMenu moveMenu;
    // Can move to root if in a folder, or to a folder if at root
    if (fi != -1)
        moveMenu.addItem(198, "Favorites (no folder)");
    for (int i = 0; i < (int)fav->folders.size(); ++i)
        if (i != fi)
            moveMenu.addItem(200 + i, fav->folders[i].name);

    juce::PopupMenu menu;
    menu.addItem(1, "Load \"" + instrName + "\"");
    menu.addSeparator();
    menu.addSubMenu("Move to...", moveMenu, moveMenu.getNumItems() > 0);
    menu.addSeparator();
    menu.addItem(2, "Remove from favorites");

    menu.showMenuAsync(
        juce::PopupMenu::Options(),
        [fav, fi, ei, onAct, onChange](int result)
        {
            if (result == 1)
            {
                FavoritesManager::Entry entry2;
                if (favGetEntry(fav, fi, ei, entry2) && onAct)
                    onAct(entry2.file);
            }
            else if (result == 2)
            {
                FavoritesManager::Entry entry2;
                if (!favGetEntry(fav, fi, ei, entry2)) return;
                const juce::String nm = entry2.name;
                juce::AlertWindow::showAsync(
                    juce::MessageBoxOptions()
                        .withIconType(juce::MessageBoxIconType::QuestionIcon)
                        .withTitle("Remove from Favorites")
                        .withMessage("Remove \"" + nm + "\" from favorites?")
                        .withButton("Remove")
                        .withButton("Cancel"),
                    [fav, fi, ei, onChange](int r)
                    {
                        if (r == 1)
                        {
                            if (fi == -1) fav->removeRootEntry(ei);
                            else          fav->removeEntry(fi, ei);
                            if (onChange) callLater(onChange);
                        }
                    });
            }
            else if (result == 198)
            {
                // move from folder to root
                FavoritesManager::Entry entry2;
                if (!favGetEntry(fav, fi, ei, entry2)) return;
                if (fi >= 0) fav->removeEntry(fi, ei);
                fav->addRootEntry(entry2.name, entry2.file);
                if (onChange) callLater(onChange);
            }
            else if (result >= 200)
            {
                const int toFolder = result - 200;
                FavoritesManager::Entry entry2;
                if (!favGetEntry(fav, fi, ei, entry2)) return;
                if (fi == -1)
                {
                    fav->addEntry(toFolder, entry2.name, entry2.file);
                    fav->removeRootEntry(ei);
                }
                else
                {
                    fav->moveEntry(fi, ei, toFolder);
                }
                if (onChange) callLater(onChange);
            }
        });
}

// ══════════════════════════════════════════════════════════════════════════════
// FavoritesFolderItem
// ══════════════════════════════════════════════════════════════════════════════
FavoritesFolderItem::FavoritesFolderItem(FavoritesManager* fm, int fi)
    : favorites(fm), folderIdx(fi) {}

juce::String FavoritesFolderItem::getUniqueName() const
{
    juce::String name;
    if (folderIdx >= 0 && folderIdx < (int)favorites->folders.size())
        name = favorites->folders[folderIdx].name;
    return "__FAV_FOLDER_" + juce::String(folderIdx) + "_" + name;
}

void FavoritesFolderItem::paintItem(juce::Graphics& g, int w, int h)
{
    const auto t = libTheme(*this);
    if (isSelected()) g.fillAll(t.favSel);
    if (folderIdx < 0 || folderIdx >= (int)favorites->folders.size()) return;

    g.setColour(t.favAmber);
    g.setFont(juce::Font(11.5f, juce::Font::bold));
    g.drawText("[F]", 2, 0, 24, h, juce::Justification::centred);

    g.setColour(t.favGold);
    g.setFont(juce::Font(12.5f, juce::Font::bold));
    g.drawText(favorites->folders[folderIdx].name, 28, 0, w - 30, h,
               juce::Justification::centredLeft);
}

void FavoritesFolderItem::itemOpennessChanged(bool isOpen)
{
    if (isOpen) populateChildren();
    else        clearSubItems();
}

void FavoritesFolderItem::populateChildren()
{
    clearSubItems();
    if (folderIdx < 0 || folderIdx >= (int)favorites->folders.size()) return;
    auto& folder = favorites->folders[folderIdx];
    for (int i = 0; i < (int)folder.entries.size(); ++i)
    {
        auto* fi   = new FavoritesFileItem(favorites, folderIdx, i);
        fi->onActivated  = onActivated;
        fi->onChanged    = onChanged;
        fi->onSFZPreview = onSFZPreview;
        addSubItem(fi);
    }
}

void FavoritesFolderItem::itemDoubleClicked(const juce::MouseEvent&)
{
    setOpen(!isOpen());
}

void FavoritesFolderItem::rebuild()
{
    clearSubItems();
    if (isOpen()) populateChildren();
}

void FavoritesFolderItem::itemClicked(const juce::MouseEvent& e)
{
    if (!e.mods.isRightButtonDown()) return;
    if (folderIdx < 0 || folderIdx >= (int)favorites->folders.size()) return;

    const juce::String folderName = favorites->folders[folderIdx].name;
    FavoritesManager*  fav        = favorites;
    const int          fi         = folderIdx;
    auto               onChange   = onChanged;

    juce::PopupMenu menu;
    menu.addItem(1, "Rename...");
    menu.addItem(2, "Delete folder");

    menu.showMenuAsync(
        juce::PopupMenu::Options(),
        [fav, fi, folderName, onChange](int result)
        {
            if (result == 1)
            {
                auto* aw = new juce::AlertWindow("Rename Folder",
                    "Enter a new name for \"" + folderName + "\":",
                    juce::MessageBoxIconType::NoIcon);
                aw->addTextEditor("name", folderName, "New name:");
                aw->addButton("Rename", 1, juce::KeyPress(juce::KeyPress::returnKey));
                aw->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
                aw->enterModalState(false,
                    juce::ModalCallbackFunction::create([aw, fav, fi, onChange](int r)
                    {
                        if (r == 1)
                        {
                            juce::String nm = aw->getTextEditorContents("name").trim();
                            if (nm.isNotEmpty() && fav->renameFolder(fi, nm))
                                if (onChange) callLater(onChange);
                        }
                    }), true);
            }
            else if (result == 2)
            {
                juce::AlertWindow::showAsync(
                    juce::MessageBoxOptions()
                        .withIconType(juce::MessageBoxIconType::QuestionIcon)
                        .withTitle("Delete Folder")
                        .withMessage("Delete folder \"" + folderName + "\" and all its entries?")
                        .withButton("Delete")
                        .withButton("Cancel"),
                    [fav, fi, onChange](int r)
                    {
                        if (r == 1)
                        {
                            fav->removeFolder(fi);
                            if (onChange) callLater(onChange);
                        }
                    });
            }
        });
}

// ══════════════════════════════════════════════════════════════════════════════
// FavoritesRootItem
// ══════════════════════════════════════════════════════════════════════════════
FavoritesRootItem::FavoritesRootItem(FavoritesManager* fm)
    : favorites(fm) {}

void FavoritesRootItem::paintItem(juce::Graphics& g, int w, int h)
{
    const auto t = libTheme(*this);
    g.fillAll(t.favBg);
    if (isSelected()) g.fillAll(t.favSel);

    g.setColour(t.favGold);
    g.setFont(juce::Font(12.5f, juce::Font::bold));
    g.drawText("FAVORITES", 8, 0, w - 10, h, juce::Justification::centredLeft);

    g.setColour(t.libDim);
    g.setFont(juce::Font(10.0f));
    g.drawText("right-click", 24, 0, w - 28, h, juce::Justification::centredRight);

    g.setColour(t.favGold.withAlpha(0.25f));
    g.drawLine(0.0f, (float)h - 1.0f, (float)w, (float)h - 1.0f, 1.0f);
}

void FavoritesRootItem::itemDoubleClicked(const juce::MouseEvent&)
{
    setOpen(!isOpen());
}

void FavoritesRootItem::rebuild()
{
    // capture open-state of folder items before clearing
    int rootFileCount = 0;
    std::vector<bool> folderWasOpen;
    for (int i = 0; i < getNumSubItems(); ++i)
    {
        auto* sub = getSubItem(i);
        if (dynamic_cast<FavoritesFolderItem*>(sub))
            folderWasOpen.push_back(sub->isOpen());
        else
            ++rootFileCount;
    }

    clearSubItems();

    // root entries first
    for (int i = 0; i < (int)favorites->rootEntries.size(); ++i)
    {
        auto* fi = new FavoritesFileItem(favorites, -1, i);
        fi->onActivated  = onActivated;
        fi->onChanged    = onChanged;
        fi->onSFZPreview = onSFZPreview;
        addSubItem(fi);
    }

    // then folders
    for (int i = 0; i < (int)favorites->folders.size(); ++i)
    {
        auto* fi = new FavoritesFolderItem(favorites, i);
        fi->onActivated  = onActivated;
        fi->onChanged    = onChanged;
        fi->onSFZPreview = onSFZPreview;
        addSubItem(fi);
        if (i < (int)folderWasOpen.size() && folderWasOpen[i])
            fi->setOpen(true);
    }
}

void FavoritesRootItem::itemClicked(const juce::MouseEvent& e)
{
    if (!e.mods.isRightButtonDown()) return;

    FavoritesManager* fav      = favorites;
    auto              onChange = onChanged;

    juce::PopupMenu menu;
    menu.addItem(1, "New folder...");

    menu.showMenuAsync(
        juce::PopupMenu::Options(),
        [fav, onChange](int result)
        {
            if (result != 1) return;
            auto* aw = new juce::AlertWindow("New Favorites Folder",
                "Enter a name for the new folder:",
                juce::MessageBoxIconType::NoIcon);
            aw->addTextEditor("name", "", "Folder name:");
            aw->addButton("Create", 1, juce::KeyPress(juce::KeyPress::returnKey));
            aw->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
            aw->enterModalState(false,
                juce::ModalCallbackFunction::create([aw, fav, onChange](int r)
                {
                    if (r == 1)
                    {
                        juce::String nm = aw->getTextEditorContents("name").trim();
                        if (nm.isNotEmpty() && fav->addFolder(nm))
                            if (onChange) callLater(onChange);
                    }
                }), true);
        });
}

// ══════════════════════════════════════════════════════════════════════════════
// LibraryPanel
// ══════════════════════════════════════════════════════════════════════════════
namespace
{
class StarRatingMenuItem : public juce::PopupMenu::CustomComponent
{
public:
    StarRatingMenuItem(int value, bool selected, DSKTheme colours)
        : rating(value), current(selected), theme(colours) {}
    void getIdealSize(int& width, int& height) override { width = 140; height = 26; }
    void paint(juce::Graphics& g) override
    {
        if (isItemHighlighted()) g.fillAll(theme.accent.withAlpha(0.15f));
        if (current)
        {
            g.setColour(theme.favGold);
            g.fillEllipse(7.0f, getHeight() * 0.5f - 2.0f, 4.0f, 4.0f);
        }
        for (int i = 0; i < rating; ++i)
            StarButton::drawStar(g, { 22.0f + i * 20, (getHeight() - 16) * 0.5f, 16.0f, 16.0f }, theme.favGold, true);
    }
private:
    int rating;
    bool current;
    DSKTheme theme;
};

void addStarRatingMenu(juce::PopupMenu& menu, int current, DSKTheme theme)
{
    menu.addItem(10, "Clear rating", true, current == 0);
    for (int stars = 1; stars <= 5; ++stars)
        menu.addCustomItem(10 + stars, std::make_unique<StarRatingMenuItem>(stars, current == stars, theme),
            nullptr, juce::String(stars) + (stars == 1 ? " star" : " stars"));
}

class InstrumentPresetItem : public juce::TreeViewItem
{
public:
    juce::String name;
    juce::String identity;
    bool folder = false;
    int rating = 0;
    std::function<void(int)> rate;
    int starStart(int width) const { return juce::jmax(4, width - rating * 20 - 4); }
    std::function<void()> activate;
    std::function<void()> rename;
    explicit InstrumentPresetItem(juce::String text) : name(std::move(text)) {}
    bool mightContainSubItems() override { return folder || getNumSubItems() > 0; }
    juce::String getUniqueName() const override { return identity.isEmpty() ? name : identity; }
    int getItemHeight() const override { return name.containsChar('\n') ? 36 : 24; }
    juce::String getTooltip() override { return name; }
    void paintItem(juce::Graphics& g, int w, int h) override
    {
        auto t = libTheme(*this);
        if (isSelected()) g.fillAll(t.libPanel);
        g.setColour(isSelected() ? t.accent : t.libText);
        const int textX = folder ? 24 : 4;
        const int textWidth = juce::jmax(0, (rate && rating > 0 ? starStart(w) - 4 : w - 4) - textX);
        if (folder)
        {
            const float y = (h - 14) * 0.5f;
            juce::Path icon;
            icon.startNewSubPath(5.0f, y + 2.0f);
            icon.lineTo(11.0f, y + 2.0f);
            icon.lineTo(13.0f, y + 5.0f);
            icon.lineTo(20.0f, y + 5.0f);
            icon.lineTo(20.0f, y + 13.0f);
            icon.lineTo(5.0f, y + 13.0f);
            icon.closeSubPath();
            g.setColour(t.favGold.withAlpha(0.2f));
            g.fillPath(icon);
            g.setColour(t.favGold);
            g.strokePath(icon, juce::PathStrokeType(1.2f, juce::PathStrokeType::curved));
            g.drawLine(5.0f, y + 5.0f, 20.0f, y + 5.0f, 1.0f);
        }
        if (rate)
            for (int i = 0; i < rating; ++i)
                StarButton::drawStar(g, juce::Rectangle<float>((float) (starStart(w) + i * 20 + 2), ((name.containsChar('\n') ? h / 2 : h) - 16) * 0.5f, 16.0f, 16.0f), t.favGold, i < rating);
        g.setColour(isSelected() ? t.accent : t.libText);
        g.setFont(13.0f);
        if (name.containsChar('\n'))
        {
            g.drawText(name.upToFirstOccurrenceOf("\n", false, false), textX, 0, textWidth, h / 2, juce::Justification::centredLeft, true);
            g.setFont(11.0f);
            g.setColour(t.libDim);
            g.drawText(name.fromFirstOccurrenceOf("\n", false, false), textX, h / 2, textWidth, h / 2, juce::Justification::centredLeft, true);
        }
        else g.drawText(name, textX, 0, textWidth, h, juce::Justification::centredLeft, true);
    }
    void itemClicked(const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu()) { if (rename) rename(); }
        else if (rate && rating > 0 && e.x >= starStart(getItemPosition(true).getWidth()) && e.x < starStart(getItemPosition(true).getWidth()) + rating * 20
                 && e.y < (name.containsChar('\n') ? getItemHeight() / 2 : getItemHeight()))
        {
            const int selected = (e.x - starStart(getItemPosition(true).getWidth())) / 20 + 1;
            auto callback = rate;
            callback(selected == rating ? 0 : selected);
        }
        else if (activate) activate();
        else if (mightContainSubItems()) setOpen(!isOpen());
    }
};
}

namespace
{
void visitInstrumentTree(juce::TreeViewItem* root, const std::function<void(juce::TreeViewItem*)>& visitor)
{
    if (!root) return;
    for (int i = 0; i < root->getNumSubItems(); ++i)
    {
        auto* child = root->getSubItem(i);
        visitor(child);
        visitInstrumentTree(child, visitor);
    }
}

juce::String instrumentIdentity(const LibrarySearch::Result& instrument)
{
    return instrument.file.getFullPathName() + ":" + juce::String(instrument.bank) + ":" + juce::String(instrument.program);
}
}

void LibraryPanel::setLoadedInstrument(const LibrarySearch::Result& instrument)
{
    if (instrument.file == juce::File()) return;
    const bool changed = instrumentIdentity(instrument) != instrumentIdentity(loadedInstrument);
    loadedInstrument = instrument;
    if (!additionalInstrumentFiles.contains(instrument.file.getFullPathName()))
    {
        additionalInstrumentFiles.add(instrument.file.getFullPathName());
        refreshTree();
    }
    if (changed) selectBrowserInstrument(instrument);
}

void LibraryPanel::selectBrowserInstrument(const LibrarySearch::Result& instrument)
{
    visitInstrumentTree(presetRoot.get(), [&](juce::TreeViewItem* item)
    {
        if (item->getUniqueName() != instrumentIdentity(instrument)) return;
        item->setSelected(true, true);
        bool visible = true;
        for (auto* parent = item->getParentItem(); parent != nullptr; parent = parent->getParentItem())
            visible = visible && parent->isOpen();
        if (visible) presetTree.scrollToKeepItemVisible(item);
    });
}

void LibraryPanel::activateBrowserInstrument(const LibrarySearch::Result& instrument)
{
    selectBrowserInstrument(instrument);
    if (instrument.bank >= 0 && onSoundFontPresetRequested)
        onSoundFontPresetRequested(instrument.file, instrument.bank, instrument.program);
    else
        handleFileActivated(instrument.file, false);
}

void LibraryPanel::rebuildInstrumentBrowser(std::vector<LibrarySearch::Result> instruments)
{
    std::map<juce::String, bool> openGroups;
    juce::String selection;
    if (const auto* selected = presetTree.getSelectedItem(0)) selection = selected->getUniqueName();
    visitInstrumentTree(presetRoot.get(), [&](juce::TreeViewItem* item)
    { if (item->mightContainSubItems()) openGroups[item->getUniqueName()] = item->isOpen(); });
    browserInstruments = std::move(instruments);
    presetTree.setRootItem(nullptr);
    presetRoot = std::make_unique<InstrumentPresetItem>("Instruments");
    std::map<juce::String, InstrumentPresetItem*> groups, collections;
    InstrumentPresetItem* hiddenSection = nullptr;
    const auto collection = [&](const juce::String& id) -> InstrumentPresetItem*
    {
        if (id.isEmpty()) return nullptr;
        auto*& item = collections[id];
        if (!item)
            for (const auto& folder : soundFontFolders.all())
                if (folder.id == id)
                {
                    item = new InstrumentPresetItem(folder.name);
                    item->rating = folder.rating;
                    item->rate = [safe = juce::Component::SafePointer<LibraryPanel>(this), id](int stars)
                    { if (safe) { safe->soundFontFolders.setRating(id, stars); safe->foldersChanged(); } };
                    item->folder = true;
                    item->identity = "collection:" + id;
                    item->rename = [this, id] { showSoundFontFolderMenu(id); };
                    presetRoot->addSubItem(item);
                    break;
                }
        return item;
    };
    if (!browsingResults()) for (const auto& folder : soundFontFolders.all()) collection(folder.id);
    for (const auto& instrument : browserInstruments)
    {
        const bool soundFont = instrument.file.hasFileExtension("sf2");
        const bool hidden = instrument.metadata.hidden;
        if (hidden && !hiddenSection)
        {
            hiddenSection = new InstrumentPresetItem("Hidden instruments");
            hiddenSection->folder = true;
            hiddenSection->identity = "__hidden__";
            presetRoot->addSubItem(hiddenSection);
        }
        const auto groupId = (hidden ? juce::String("hidden:") : juce::String())
            + (soundFont ? "sf2:" + instrument.file.getFullPathName()
                : "folder:" + instrument.file.getParentDirectory().getFullPathName());
        auto*& group = groups[groupId];
        if (!group)
        {
            group = new InstrumentPresetItem(instrument.group.isEmpty() ? "Instruments" : instrument.group);
            if (soundFont)
            {
                group->rating = instrumentMetadata.get(instrument.file).rating;
                group->rate = [safe = juce::Component::SafePointer<LibraryPanel>(this), file = instrument.file](int stars)
                { if (safe) { safe->instrumentMetadata.setRating(file, -1, -1, stars); safe->metadataChanged(); } };
            }
            group->folder = !soundFont;
            group->identity = groupId;
            if (soundFont)
                group->rename = [this, file = instrument.file, name = instrument.group]
                { showInstrumentMenu({ name, {}, file }); };
            auto* parent = hidden ? hiddenSection : (soundFont ? collection(soundFontFolders.folderFor(instrument.file)) : nullptr);
            (parent ? static_cast<juce::TreeViewItem*>(parent) : presetRoot.get())->addSubItem(group);
        }
        const auto details = instrument.metadata.category;
        auto* item = new InstrumentPresetItem(instrument.name + (instrument.metadata.hidden ? " (hidden)" : "") + (details.isEmpty() ? juce::String() : "\n" + details));
        item->rating = instrument.metadata.rating;
        item->rate = [safe = juce::Component::SafePointer<LibraryPanel>(this), instrument](int stars)
        { if (safe) safe->setInstrumentRating(instrument, stars); };
        item->identity = instrumentIdentity(instrument);
        item->activate = [this, instrument] { activateBrowserInstrument(instrument); };
        item->rename = [this, instrument] { showInstrumentMenu(instrument); };
        group->addSubItem(item);
    }
    struct GroupComparator
    {
        int compareElements(juce::TreeViewItem* a, juce::TreeViewItem* b) const
        {
            const auto* first = static_cast<InstrumentPresetItem*>(a);
            const auto* second = static_cast<InstrumentPresetItem*>(b);
            if ((first->identity == "__hidden__") != (second->identity == "__hidden__"))
                return first->identity == "__hidden__" ? 1 : -1;
            if (first->rating != second->rating) return second->rating - first->rating;
            if (first->folder != second->folder) return first->folder ? -1 : 1;
            return first->name.compareIgnoreCase(second->name);
        }
    } comparator;
    presetRoot->sortSubItems(comparator);
    for (const auto& entry : collections) entry.second->sortSubItems(comparator);
    if (hiddenSection) hiddenSection->sortSubItems(comparator);
    std::map<juce::String, size_t> indices;
    for (size_t i = 0; i < browserInstruments.size(); ++i) indices.emplace(instrumentIdentity(browserInstruments[i]), i);
    std::vector<LibrarySearch::Result> ordered;
    ordered.reserve(browserInstruments.size());
    visitInstrumentTree(presetRoot.get(), [&](juce::TreeViewItem* item)
    {
        const auto found = indices.find(item->getUniqueName());
        if (found != indices.end()) ordered.push_back(std::move(browserInstruments[found->second]));
    });
    browserInstruments = std::move(ordered);
    presetTree.setRootItem(presetRoot.get());
    presetRoot->setOpen(true);
    visitInstrumentTree(presetRoot.get(), [&](juce::TreeViewItem* item)
    {
        if (item->mightContainSubItems())
        {
            const auto previous = openGroups.find(item->getUniqueName());
            item->setOpen(previous == openGroups.end() ? item->getUniqueName() != "__hidden__" : previous->second);
        }
        if (item->getUniqueName() == selection) item->setSelected(true, true);
    });
    if (savedTreeOpenState.isNotEmpty() && !browsingResults())
    {
        if (auto xml = juce::parseXML(savedTreeOpenState)) presetTree.restoreOpennessState(*xml, false);
        savedTreeOpenState.clear();
    }
    if (!presetTree.getSelectedItem(0)) selectBrowserInstrument(loadedInstrument);
    const int count = (int) browserInstruments.size();
    searchStatus.setText(count == 0 ? (browsingResults() ? "No matching instruments" : "Add a library folder to browse instruments")
        : juce::String(count) + " instruments", juce::dontSendNotification);
    previousPresetBtn.setEnabled(count > 0);
    nextPresetBtn.setEnabled(count > 0);
}

LibraryPanel::LibraryPanel()
{
    const auto navigate = [this](const juce::KeyPress& key) { return navigateInstruments(key); };
    searchBox.onInstrumentNavigation = navigate;
    presetTree.onInstrumentNavigation = navigate;

    presetTree.setRootItemVisible(false);
    presetTree.setIndentSize(12);
    addAndMakeVisible(presetTree);
    presetsLabel.setText("INSTRUMENTS", juce::dontSendNotification);

    addAndMakeVisible(collapseAllBtn);
    collapseAllBtn.setTooltip("Close all SoundFont and instrument groups");
    collapseAllBtn.onClick = [this]
    {
        if (!presetRoot) return;
        visitInstrumentTree(presetRoot.get(), [](juce::TreeViewItem* item)
        { if (item->mightContainSubItems()) item->setOpen(false); });
    };
    addAndMakeVisible(previousPresetBtn);
    addAndMakeVisible(nextPresetBtn);
    previousPresetBtn.setTooltip("Previous instrument");
    nextPresetBtn.setTooltip("Next instrument");
    previousPresetBtn.onClick = [this] { navigateInstruments(juce::KeyPress(juce::KeyPress::upKey)); };
    nextPresetBtn.onClick = [this] { navigateInstruments(juce::KeyPress(juce::KeyPress::downKey)); };
    searchBox.setTextToShowWhenEmpty("Search instruments...", juce::Colour(0xFF667788));
    searchBox.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xFF0D1B35));
    searchBox.setColour(juce::TextEditor::textColourId,       juce::Colour(0xFFCCDDEE));
    searchBox.setColour(juce::TextEditor::outlineColourId,    juce::Colour(0xFF334466));
    categoryFilter.setTooltip("Filter by instrument category");
    ratingFilter.setTooltip("Filter by minimum star rating");
    ratingFilter.addItem("All ratings", 1);
    ratingFilter.addItem("Unrated", 2);
    for (int stars = 1; stars <= 5; ++stars)
        ratingFilter.addItem(juce::String(stars) + (stars == 1 ? " star & up" : " stars & up"), stars + 2);
    ratingFilter.setSelectedId(1, juce::dontSendNotification);
    updateFilters();
    categoryFilter.onChange = [this] { updateSearch(); };
    ratingFilter.onChange = [this] { updateSearch(); };
    addAndMakeVisible(categoryFilter);
    addAndMakeVisible(ratingFilter);
    searchBox.setFont(juce::Font(13.0f));
    searchBox.setIndents(6, 4);
    searchBox.addListener(this);
    addAndMakeVisible(searchBox);

    clearSearchBtn.setColour(juce::TextButton::buttonColourId,  juce::Colours::transparentBlack);
    clearSearchBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFF667788));
    clearSearchBtn.setColour(juce::TextButton::textColourOnId,  juce::Colour(0xFFCCDDEE));
    clearSearchBtn.setVisible(false);
    clearSearchBtn.onClick = [this]()
    {
        searchBox.clear();
        updateSearch();
        searchBox.grabKeyboardFocus();
    };
    addAndMakeVisible(clearSearchBtn);

    for (auto* btn : { &addFolderBtn, &removeFolderBtn, &refreshBtn, &newFolderBtn })
    {
        btn->setColour(juce::TextButton::buttonColourId,  juce::Colour(0xFF1A2A4A));
        btn->setColour(juce::TextButton::textColourOffId, juce::Colour(0xFF00D4FF));
        addAndMakeVisible(btn);
    }
    addFolderBtn.setTooltip("Add library folder");
    removeFolderBtn.setTooltip("Remove a library folder");
    refreshBtn.setTooltip("Expand / Collapse all folders");

    addFolderBtn.onClick = [this]()
    {
        fileChooser = std::make_unique<juce::FileChooser>(
            "Select a folder containing SFZ instruments",
            juce::File::getSpecialLocation(juce::File::userMusicDirectory));
        fileChooser->launchAsync(
            juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
            [this](const juce::FileChooser& fc)
            {
                auto result = fc.getResult();
                if (result.isDirectory()) addFolder(result);
            });
    };

    removeFolderBtn.onClick = [this]() { removeSelectedFolder(); };

    refreshBtn.setButtonText("R");
    refreshBtn.setTooltip("Refresh instruments from all library folders");
    refreshBtn.onClick = [this] { refreshTree(); };
    newFolderBtn.setTooltip("Create a folder for SoundFonts");
    newFolderBtn.onClick = [this] { showCreateSoundFontFolder(); };
    addAndMakeVisible(searchStatus);
    searchStatus.setFont(juce::Font(11.0f));

    refreshTree();
    lookAndFeelChanged();
}

LibraryPanel::~LibraryPanel()
{
    ++*searchGeneration;
    searchPool.removeAllJobs(true, -1);
    presetTree.setRootItem(nullptr);
    favoritesRootItem = nullptr;
    treeView.setRootItem(nullptr);
}

void LibraryPanel::lookAndFeelChanged()
{
    const auto t = libTheme(this);
    searchBox.setTextToShowWhenEmpty("Search instruments...", t.libDim);
    searchBox.setColour(juce::TextEditor::backgroundColourId, t.libTreeBg);
    searchBox.setColour(juce::TextEditor::textColourId,       t.libText);
    searchBox.applyColourToAllText(t.libText);
    searchBox.setColour(juce::TextEditor::outlineColourId, t.divider);
    searchBox.setColour(juce::TextEditor::focusedOutlineColourId, t.accent);
    searchBox.setColour(juce::TextEditor::highlightColourId, t.libSel);
    searchBox.setColour(juce::TextEditor::highlightedTextColourId, t.libText);
    for (auto* filter : { &categoryFilter, &ratingFilter })
    {
        filter->setColour(juce::ComboBox::backgroundColourId, t.libTreeBg);
        filter->setColour(juce::ComboBox::textColourId, t.libText);
        filter->setColour(juce::ComboBox::outlineColourId, t.divider);
        filter->setColour(juce::ComboBox::arrowColourId, t.accent);
        filter->setColour(juce::ComboBox::focusedOutlineColourId, t.accent);
    }
    presetsLabel.setColour(juce::Label::textColourId, t.accent);
    presetsLabel.setFont(juce::Font(11.0f, juce::Font::bold));
    for (auto* button : { &collapseAllBtn, &previousPresetBtn, &nextPresetBtn })
    {
        button->setColour(juce::TextButton::buttonColourId, t.libPanel);
        button->setColour(juce::TextButton::textColourOffId, t.accent);
    }
    presetTree.setColour(juce::TreeView::backgroundColourId, t.libTreeBg);
    treeView.setColour(juce::TreeView::backgroundColourId, t.libTreeBg);

    searchStatus.setColour(juce::Label::textColourId, t.libDim);
    for (auto* btn : { &addFolderBtn, &removeFolderBtn, &refreshBtn, &newFolderBtn })
    {
        btn->setColour(juce::TextButton::buttonColourId,  t.libPanel);
        btn->setColour(juce::TextButton::textColourOffId, t.accent);
    }
    clearSearchBtn.setColour(juce::TextButton::textColourOffId, t.libDim);
    treeView.repaint();
    presetTree.repaint();

    repaint();
}

void LibraryPanel::paint(juce::Graphics& g)
{
    const auto t = libTheme(this);
    g.fillAll(t.libBg);

    g.setColour(t.libPanel);
    g.fillRect(0, 0, getWidth(), 26);

    g.setColour(t.accent);
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.drawText("INSTRUMENTS", 6, 4, juce::jmax(0, getWidth() - 120), 18, juce::Justification::centredLeft);

}

void LibraryPanel::resized()
{
    auto area = getLocalBounds();

    auto header = area.removeFromTop(26);
    refreshBtn.setBounds     (header.removeFromRight(30).reduced(2));
    newFolderBtn.setBounds   (header.removeFromRight(30).reduced(2));
    removeFolderBtn.setBounds(header.removeFromRight(26).reduced(2));
    addFolderBtn.setBounds   (header.removeFromRight(26).reduced(2));

    searchBox.setBounds(area.removeFromTop(28).reduced(4, 3));
    clearSearchBtn.setBounds(searchBox.getRight() - 24, searchBox.getY() + 2,
                             20, searchBox.getHeight() - 4);

    categoryFilter.setBounds(area.removeFromTop(26).reduced(4, 2));
    ratingFilter.setBounds(area.removeFromTop(26).reduced(4, 2));
    auto controls = area.removeFromTop(26);
    nextPresetBtn.setBounds(controls.removeFromRight(28).reduced(2));
    previousPresetBtn.setBounds(controls.removeFromRight(28).reduced(2));
    collapseAllBtn.setBounds(controls.removeFromLeft(88).reduced(2));
    searchStatus.setBounds(controls.reduced(4, 0));
    presetTree.setBounds(area);

}

void LibraryPanel::addFolder(const juce::File& folder)
{
    if (!libraryFolderPaths.contains(folder.getFullPathName()))
        libraryFolderPaths.add(folder.getFullPathName());
    refreshTree();
}

void LibraryPanel::refreshTree()
{
    runSearch(searchBox.getText().trim());
}

void LibraryPanel::rebuildFavoritesSection()
{
    if (favoritesRootItem)
        favoritesRootItem->rebuild();
}

void LibraryPanel::showAddToFavoritesMenu(const juce::File& sfzFile,
                                           const juce::MouseEvent& /*e*/)
{
    const juce::String instrName = soundFontName(sfzFile, sfzFile.getFileNameWithoutExtension());

    juce::PopupMenu folderMenu;
    // Option: add directly to Favorites root (no folder)
    folderMenu.addItem(98, "Favorites (no folder)");
    if (!favorites.folders.empty())
    {
        folderMenu.addSeparator();
        for (int i = 0; i < (int)favorites.folders.size(); ++i)
            folderMenu.addItem(100 + i, favorites.folders[i].name);
    }
    folderMenu.addSeparator();
    folderMenu.addItem(99, "New folder...");

    juce::PopupMenu menu;
    menu.addSubMenu("Add to Favorites", folderMenu);
    if (sfzFile.hasFileExtension("sf2")) menu.addItem(97, "Rename SoundFont...");

    menu.showMenuAsync(
        juce::PopupMenu::Options(),
        [this, sfzFile, instrName](int result)
        {
            if (result == 97)
            {
                renameSoundFont(sfzFile, sfzFile.getFileNameWithoutExtension());
            }
            else if (result == 98)
            {
                // add to root
                if (favorites.addRootEntry(instrName, sfzFile))
                    rebuildFavoritesSection();
                else
                    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                        "Already in Favorites",
                        "\"" + instrName + "\" is already in Favorites.");
            }
            else if (result == 99)
            {
                auto* aw = new juce::AlertWindow("New Favorites Folder",
                    "Enter a name for the new folder:",
                    juce::MessageBoxIconType::NoIcon);
                aw->addTextEditor("name", "", "Folder name:");
                aw->addButton("Create", 1, juce::KeyPress(juce::KeyPress::returnKey));
                aw->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
                aw->enterModalState(false,
                    juce::ModalCallbackFunction::create([this, aw, sfzFile, instrName](int r)
                    {
                        if (r == 1)
                        {
                            juce::String nm = aw->getTextEditorContents("name").trim();
                            if (nm.isNotEmpty())
                            {
                                favorites.addFolder(nm);
                                int idx = favorites.findFolder(nm);
                                if (idx >= 0) favorites.addEntry(idx, instrName, sfzFile);
                                rebuildFavoritesSection();
                            }
                        }
                    }), true);
            }
            else if (result >= 100)
            {
                int idx = result - 100;
                if (favorites.addEntry(idx, instrName, sfzFile))
                    rebuildFavoritesSection();
                else
                    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                        "Already in Favorites",
                        "\"" + instrName + "\" is already in that folder.");
            }
        });
}

void LibraryPanel::metadataChanged()
{
    if (nameSettings)
    {
        nameSettings->setValue("instrumentMetadata", juce::JSON::toString(instrumentMetadata.toJson()));
        if (!nameSettings->saveIfNeeded())
            juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                "Changes not saved", "Ratings and categories changed for this session, but settings could not be saved.");
    }
    updateFilters();
    updateSearch();
    repaint();
}

void LibraryPanel::setInstrumentRating(const LibrarySearch::Result& instrument, int rating)
{
    if (instrument.file == juce::File()) return;
    instrumentMetadata.setRating(instrument.file, instrument.bank, instrument.program, rating);
    metadataChanged();
}

juce::String LibraryPanel::createSoundFontFolder(const juce::String& name)
{
    const auto id = soundFontFolders.add(name);
    if (id.isNotEmpty()) foldersChanged();
    return id;
}

void LibraryPanel::moveSoundFontToFolder(const juce::File& file, const juce::String& folderId)
{
    if (!soundFontFolders.move(file, folderId)) return;
    additionalInstrumentFiles.addIfNotAlreadyThere(file.getFullPathName());
    foldersChanged();
}

void LibraryPanel::removeSoundFontFolder(const juce::String& folderId)
{
    soundFontFolders.remove(folderId);
    foldersChanged();
}

void LibraryPanel::foldersChanged()
{
    if (nameSettings) { saveState(*nameSettings); nameSettings->saveIfNeeded(); }
    refreshTree();
}

void LibraryPanel::showCreateSoundFontFolder(const juce::File& file)
{
    auto* dialog = new juce::AlertWindow("New SoundFont folder", "Choose a folder name:", juce::MessageBoxIconType::NoIcon);
    dialog->addTextEditor("name", {}, "Name:");
    dialog->addButton("Create", 1, juce::KeyPress(juce::KeyPress::returnKey));
    dialog->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
    juce::Component::SafePointer<LibraryPanel> safeThis(this);
    dialog->enterModalState(true, juce::ModalCallbackFunction::create([safeThis, file, dialog](int answer)
    {
        if (!safeThis || answer != 1) return;
        const auto id = safeThis->createSoundFontFolder(dialog->getTextEditorContents("name"));
        if (file != juce::File() && id.isNotEmpty()) safeThis->moveSoundFontToFolder(file, id);
    }), true);
}

void LibraryPanel::showSoundFontFolderMenu(const juce::String& id)
{
    juce::String name;
    for (const auto& folder : soundFontFolders.all()) if (folder.id == id) name = folder.name;
    juce::PopupMenu menu;
    menu.addSectionHeader(name);
    menu.addItem(1, "Rename folder...");
    menu.addItem(2, "Remove folder (keep SoundFonts)");
    juce::PopupMenu ratings;
    int currentRating = 0;
    for (const auto& folder : soundFontFolders.all()) if (folder.id == id) currentRating = folder.rating;
    addStarRatingMenu(ratings, currentRating, libTheme(this));
    menu.addSubMenu("Star rating", ratings);
    juce::Component::SafePointer<LibraryPanel> safeThis(this);
    menu.showMenuAsync(juce::PopupMenu::Options(), [safeThis, id, name](int result)
    {
        if (!safeThis) return;
        if (result >= 10 && result <= 15)
        { safeThis->soundFontFolders.setRating(id, result - 10); safeThis->foldersChanged(); }
        else if (result == 2) safeThis->removeSoundFontFolder(id);
        else if (result == 1)
        {
            auto* dialog = new juce::AlertWindow("Rename folder", "Choose a folder name:", juce::MessageBoxIconType::NoIcon);
            dialog->addTextEditor("name", name, "Name:");
            dialog->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
            dialog->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
            dialog->enterModalState(true, juce::ModalCallbackFunction::create([safeThis, id, dialog](int answer)
            {
                if (safeThis && answer == 1 && safeThis->soundFontFolders.rename(id, dialog->getTextEditorContents("name")))
                    safeThis->foldersChanged();
            }), true);
        }
    });
}

void LibraryPanel::showInstrumentMenu(const LibrarySearch::Result& instrument, bool categoryOnly)
{
    const bool isBank = instrument.file.hasFileExtension("sf2") && instrument.bank < 0;
    const auto info = instrumentInfo(instrument);
    const auto categories = instrumentMetadata.categories();
    juce::PopupMenu menu, categoryItems;
    auto& types = categoryOnly ? menu : categoryItems;
    menu.addSectionHeader(instrument.name);
    {
        types.addItem(20, "Uncategorized", true, info.category.isEmpty());
        for (int i = 0; i < categories.size(); ++i)
            types.addItem(100 + i, categories[i], true, info.category.equalsIgnoreCase(categories[i]));
        types.addSeparator();
        types.addItem(21, "Custom category...");
        if (!categoryOnly)
        {
            menu.addSubMenu("Category", types);
        }
    }
    if (!categoryOnly && !isBank) menu.addItem(30, info.hidden ? "Show instrument" : "Hide instrument");
    if (!categoryOnly && instrument.file != juce::File())
        menu.addItem(1, isBank ? "Rename SoundFont..." : "Rename instrument...");
    const auto folders = soundFontFolders.all();
    if (!categoryOnly && isBank)
    {
        juce::PopupMenu destinations;
        const auto current = soundFontFolders.folderFor(instrument.file);
        destinations.addItem(1001, "No folder", true, current.isEmpty());
        for (size_t i = 0; i < folders.size(); ++i)
            destinations.addItem(10000 + (int) i, folders[i].name, true, current == folders[i].id);
        destinations.addSeparator();
        destinations.addItem(1000, "New folder...");
        menu.addSubMenu("Move SoundFont to folder", destinations);
    }
    const juce::Component::SafePointer<LibraryPanel> safeThis(this);
    menu.showMenuAsync(juce::PopupMenu::Options(), [safeThis, instrument, categories, folders](int result)
    {
        if (!safeThis || result == 0) return;
        if (result == 1000) safeThis->showCreateSoundFontFolder(instrument.file);
        else if (result == 1001) safeThis->moveSoundFontToFolder(instrument.file, {});
        else if (result >= 10000 && result < 10000 + (int) folders.size())
            safeThis->moveSoundFontToFolder(instrument.file, folders[(size_t) (result - 10000)].id);
        else if (result == 30)
        {
            safeThis->setInstrumentHidden(instrument, !safeThis->instrumentInfo(instrument).hidden);
        }
        else if (result == 1)
            safeThis->renameSoundFont(instrument.file, instrument.name, instrument.bank, instrument.program);
        else if (result >= 10 && result <= 15)
            safeThis->setInstrumentRating(instrument, result - 10);
        else if (result == 20 || (result >= 100 && result < 100 + categories.size()))
        {
            safeThis->instrumentMetadata.setCategory(instrument.file, instrument.bank, instrument.program,
                result == 20 ? juce::String() : categories[result - 100]);
            safeThis->metadataChanged();
        }
        else if (result == 21)
        {
            auto* dialog = new juce::AlertWindow("Instrument category", "Enter a category name:", juce::MessageBoxIconType::NoIcon);
            dialog->addTextEditor("category", safeThis->instrumentInfo(instrument).category, "Category:");
            dialog->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
            dialog->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
            dialog->enterModalState(true, juce::ModalCallbackFunction::create([safeThis, instrument, dialog](int answer)
            {
                if (!safeThis || answer != 1) return;
                safeThis->instrumentMetadata.setCategory(instrument.file, instrument.bank, instrument.program,
                    dialog->getTextEditorContents("category"));
                safeThis->metadataChanged();
            }), true);
        }
    });
}

void LibraryPanel::renameSoundFont(const juce::File& file, const juce::String& original,
                                  int bank, int program)
{
    auto* dialog = new juce::AlertWindow(bank < 0 && file.hasFileExtension("sf2") ? "Rename SoundFont" : "Rename Instrument",
        "Choose a custom name for this player. Leave blank to restore the original name.",
        juce::MessageBoxIconType::NoIcon);
    dialog->addTextEditor("name", soundFontName(file, original, bank, program), "Name:");
    dialog->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
    dialog->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
    const juce::Component::SafePointer<LibraryPanel> safeThis(this);
    dialog->enterModalState(true, juce::ModalCallbackFunction::create(
        [safeThis, dialog, file, bank, program](int result)
        {
            if (!safeThis || result != 1) return;
            safeThis->soundFontNames.set(file, bank, program, dialog->getTextEditorContents("name"));
            if (bank < 0)
            {
                const auto name = safeThis->soundFontName(file, file.getFileNameWithoutExtension());
                const auto update = [&](auto& entries)
                {
                    for (auto& entry : entries) if (entry.file == file) entry.name = name;
                };
                update(safeThis->favorites.rootEntries);
                for (auto& folder : safeThis->favorites.folders) update(folder.entries);
            }
            if (auto* settings = safeThis->nameSettings)
            {
                settings->setValue("soundfontNames", juce::JSON::toString(safeThis->soundFontNames.toJson()));
                safeThis->favorites.save(*settings);
                if (!settings->saveIfNeeded())
                    juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                        "Name not saved", "The name was changed for this session, but the settings file could not be saved.");
            }
            safeThis->refreshTree();
            safeThis->repaint();
        }), true);
}

void LibraryPanel::removeSelectedFolder()
{
    juce::PopupMenu menu;
    const auto folders = libraryFolderPaths;
    for (int i = 0; i < folders.size(); ++i)
        menu.addItem(i + 1, juce::File(folders[i]).getFileName());
    juce::Component::SafePointer<LibraryPanel> safeThis(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&removeFolderBtn),
        [safeThis, folders](int result)
        {
            if (!safeThis || result <= 0 || result > folders.size()) return;
            safeThis->libraryFolderPaths.removeString(folders[result - 1]);
            for (int i = safeThis->additionalInstrumentFiles.size(); --i >= 0;)
                if (juce::File(safeThis->additionalInstrumentFiles[i]).isAChildOf(juce::File(folders[result - 1])))
                    safeThis->additionalInstrumentFiles.remove(i);
            safeThis->refreshTree();
        });
}

bool LibraryPanel::navigateInstruments(const juce::KeyPress& key)
{
    if (!key.isKeyCode(juce::KeyPress::upKey) && !key.isKeyCode(juce::KeyPress::downKey)
        && !key.isKeyCode(juce::KeyPress::leftKey) && !key.isKeyCode(juce::KeyPress::rightKey)) return false;
    // Closed groups are excluded, so arrows never reveal or load hidden presets.
    std::vector<const LibrarySearch::Result*> visibleInstruments;
    std::map<juce::String, const LibrarySearch::Result*> byIdentity;
    for (const auto& instrument : browserInstruments)
        if (!instrument.metadata.hidden) byIdentity.emplace(instrumentIdentity(instrument), &instrument);
    std::function<void(juce::TreeViewItem*)> collect = [&](juce::TreeViewItem* parent)
    {
        if (!parent || !parent->isOpen()) return;
        for (int i = 0; i < parent->getNumSubItems(); ++i)
        {
            auto* child = parent->getSubItem(i);
            const auto found = byIdentity.find(child->getUniqueName());
            if (found != byIdentity.end()) visibleInstruments.push_back(found->second);
            else collect(child);
        }
    };
    collect(presetRoot.get());
    if (visibleInstruments.empty()) return true;
    const int direction = (key.isKeyCode(juce::KeyPress::upKey) || key.isKeyCode(juce::KeyPress::leftKey)) ? -1 : 1;
    const auto* selected = presetTree.getSelectedItem(0);
    const auto identity = selected ? selected->getUniqueName() : instrumentIdentity(loadedInstrument);
    int current = -1;
    for (int i = 0; i < (int) visibleInstruments.size(); ++i)
        if (instrumentIdentity(*visibleInstruments[(size_t) i]) == identity) { current = i; break; }
    const int count = (int) visibleInstruments.size();
    const int next = current < 0 ? (direction > 0 ? 0 : count - 1) : juce::jlimit(0, count - 1, current + direction);
    if (next != current) activateBrowserInstrument(*visibleInstruments[(size_t) next]);
    return true;
}

void LibraryPanel::textEditorTextChanged(juce::TextEditor&)
{
    updateSearch();
}

bool LibraryPanel::browsingResults() const
{
    return searchBox.getText().trim().isNotEmpty() || currentFilter().active();
}

InstrumentMetadata::Filter LibraryPanel::currentFilter() const
{
    InstrumentMetadata::Filter filter;
    filter.unratedOnly = ratingFilter.getSelectedId() == 2;
    filter.minimumRating = juce::jmax(0, ratingFilter.getSelectedId() - 2);
    filter.uncategorizedOnly = categoryFilter.getSelectedId() == 2;
    if (categoryFilter.getSelectedId() > 2) filter.category = categoryFilter.getText();
    return filter;
}

void LibraryPanel::updateFilters()
{
    const auto selected = categoryFilter.getText();
    const int oldId = categoryFilter.getSelectedId();
    categoryFilter.clear(juce::dontSendNotification);
    categoryFilter.addItem("All categories", 1);
    categoryFilter.addItem("Uncategorized", 2);
    int id = 3, selectedId = oldId == 2 ? 2 : 1;
    for (const auto& category : instrumentMetadata.categories())
    {
        categoryFilter.addItem(category, id);
        if (oldId > 2 && category.equalsIgnoreCase(selected)) selectedId = id;
        ++id;
    }
    categoryFilter.setSelectedId(selectedId, juce::dontSendNotification);
}

void LibraryPanel::updateSearch()
{
    ++*searchGeneration;
    const auto query = searchBox.getText().trim();
    clearSearchBtn.setVisible(query.isNotEmpty());
    runSearch(query);
    resized();
}

void LibraryPanel::runSearch(const juce::String& query)
{
    const int generation = ++*searchGeneration;
    const auto token = searchGeneration;
    const auto folders = libraryFolderPaths;
    const auto names = soundFontNames;
    const auto metadata = instrumentMetadata;
    const auto filter = currentFilter();
    const auto extraFiles = additionalInstrumentFiles;
    const juce::Component::SafePointer<LibraryPanel> safeThis(this);

    searchStatus.setText(query.isEmpty() ? "Loading instruments..." : "Searching instruments...", juce::dontSendNotification);
    searchPool.addJob([safeThis, token, generation, folders, query, names, metadata, filter, extraFiles]
    {
        const auto cancelled = [&] { return token->load() != generation; };
        auto results = LibrarySearch::find(folders, query, cancelled, names, metadata, filter, true, extraFiles);
        if (cancelled()) return;
        juce::MessageManager::callAsync([safeThis, token, generation, results = std::move(results)]() mutable
        {
            if (!safeThis || token->load() != generation) return;
            safeThis->rebuildInstrumentBrowser(std::move(results));
        });
    });
}

void LibraryPanel::handleFileActivated(const juce::File& file, bool isZip)
{
    if (isZip)
    {
        fileChooser = std::make_unique<juce::FileChooser>(
            "Extract ZIP to folder",
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory));

        auto zipPtr = std::make_shared<juce::ZipFile>(file);
        fileChooser->launchAsync(
            juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
            [this, zipPtr](const juce::FileChooser& fc)
            {
                auto dest = fc.getResult();
        if (!dest.isDirectory()) return;

        // 1. Crear ventana de progreso
        // Usamos un puntero raw para el progressBar para que viva mientras la ventana esté abierta
        auto progressWindow = std::make_unique<juce::AlertWindow>("Extracting...",
            "Please wait...",
            juce::AlertWindow::NoIcon);

        double progress = 0.0;
        // CORRECTO: ProgressBar solo acepta la variable de referencia
        auto progressBar = std::make_unique<juce::ProgressBar>(progress);

        progressWindow->addCustomComponent(progressBar.get());
        progressWindow->enterModalState(false, nullptr, false);

        bool hasFilesInRoot = false;
        juce::Array<juce::File> extractedTopLevelFolders;
        int totalEntries = (int)zipPtr->getNumEntries();

        // 2. Bucle de extracción
        for (int i = 0; i < totalEntries; ++i)
        {
            auto* entry = zipPtr->getEntry(i);
            if (entry != nullptr)
            {
                juce::String path = entry->filename;
                int firstSlash = path.indexOfChar('/');
                if (firstSlash < 0) firstSlash = path.indexOfChar('\\');

                if (firstSlash < 0) {
                    if (!path.endsWith("/") && !path.endsWith("\\")) hasFilesInRoot = true;
                }
                else {
                    juce::String topLevel = path.substring(0, firstSlash);
                    juce::File f = dest.getChildFile(topLevel);
                    if (!extractedTopLevelFolders.contains(f)) extractedTopLevelFolders.add(f);
                }
            }

            zipPtr->uncompressEntry(i, dest, true);

            // 3. Actualizar progreso
            progress = (double)(i + 1) / (double)totalEntries;
            progressBar->repaint();

            // Forzar el procesado de eventos de UI para mover la barra
            juce::MessageManager::getInstance()->runDispatchLoopUntil(1);
        }

        progressWindow->exitModalState(0);
        progressWindow = nullptr; // Cerramos ventana

        // 4. Lógica final
        if (hasFilesInRoot) {
            addFolder(dest);
        }
        else {
            for (auto& folder : extractedTopLevelFolders) addFolder(folder);
            if (extractedTopLevelFolders.isEmpty()) addFolder(dest);
        }

        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "ZIP Extracted", "Done!");
            });
    }
    else
    {
        listeners.call([&file](Listener& l) { l.libraryFileRequested(file); });
    }
}

// ── JSON export / import ──────────────────────────────────────────────────────
juce::var LibraryPanel::buildConfigJson() const
{
    auto* root = new juce::DynamicObject();
    root->setProperty("version", 1);

    // save current theme index
    if (auto* lf = dynamic_cast<const DSKLookAndFeel*>(&getLookAndFeel()))
        root->setProperty("uiTheme", lf->getThemeIndex());

    juce::Array<juce::var> libFolders;
    for (auto& p : libraryFolderPaths) libFolders.add(p);
    root->setProperty("library_folders", libFolders);
    juce::Array<juce::var> files;
    for (const auto& file : additionalInstrumentFiles) files.add(file);
    root->setProperty("instrument_files", files);

    // favorites
    auto* favsObj = new juce::DynamicObject();

    juce::Array<juce::var> rootArr;
    for (auto& e : favorites.rootEntries)
    {
        auto* eo = new juce::DynamicObject();
        eo->setProperty("name", e.name);
        eo->setProperty("path", e.file.getFullPathName());
        rootArr.add(juce::var(eo));
    }
    favsObj->setProperty("root_entries", rootArr);

    juce::Array<juce::var> foldersArr;
    for (auto& f : favorites.folders)
    {
        auto* fo = new juce::DynamicObject();
        fo->setProperty("name", f.name);
        juce::Array<juce::var> entriesArr;
        for (auto& e : f.entries)
        {
            auto* eo = new juce::DynamicObject();
            eo->setProperty("name", e.name);
            eo->setProperty("path", e.file.getFullPathName());
            entriesArr.add(juce::var(eo));
        }
        fo->setProperty("entries", entriesArr);
        foldersArr.add(juce::var(fo));
    }
    favsObj->setProperty("folders", foldersArr);
    root->setProperty("favorites", juce::var(favsObj));
    root->setProperty("soundfont_names", soundFontNames.toJson());
    root->setProperty("soundfont_folders", soundFontFolders.toJson());
    root->setProperty("instrument_metadata", instrumentMetadata.toJson());

    return juce::var(root);
}

int LibraryPanel::applyConfigJson(const juce::var& parsed)
{
    auto* obj = parsed.getDynamicObject();
    if (!obj) return -1;

    if (obj->hasProperty("instrument_metadata"))
    {
        instrumentMetadata.fromJson(obj->getProperty("instrument_metadata"));
        updateFilters();
    }
    if (obj->hasProperty("soundfont_folders")) soundFontFolders.fromJson(obj->getProperty("soundfont_folders"));
    if (obj->hasProperty("soundfont_names"))
        soundFontNames.fromJson(obj->getProperty("soundfont_names"));

    int themeIdx = -1;
    if (obj->hasProperty("uiTheme"))
        themeIdx = (int)obj->getProperty("uiTheme");

    if (auto* arr = obj->getProperty("library_folders").getArray())
    {
        libraryFolderPaths.clear();
        for (auto& v : *arr) libraryFolderPaths.add(v.toString());
    }

    if (auto* files = obj->getProperty("instrument_files").getArray())
    {
        additionalInstrumentFiles.clear();
        for (const auto& file : *files) additionalInstrumentFiles.add(file.toString());
    }

    auto favsVar = obj->getProperty("favorites");
    if (auto* favsObj = favsVar.getDynamicObject())
    {
        favorites.rootEntries.clear();
        if (auto* arr = favsObj->getProperty("root_entries").getArray())
        {
            for (auto& v : *arr)
                if (auto* eo = v.getDynamicObject())
                {
                    FavoritesManager::Entry e;
                    e.name = eo->getProperty("name").toString();
                    e.file = juce::File(eo->getProperty("path").toString());
                    if (e.name.isNotEmpty() && e.file != juce::File())
                        favorites.rootEntries.push_back(e);
                }
        }

        favorites.folders.clear();
        if (auto* arr = favsObj->getProperty("folders").getArray())
        {
            for (auto& v : *arr)
                if (auto* fo = v.getDynamicObject())
                {
                    FavoritesManager::Folder f;
                    f.name = fo->getProperty("name").toString();
                    if (auto* entries = fo->getProperty("entries").getArray())
                        for (auto& ev : *entries)
                            if (auto* eo = ev.getDynamicObject())
                            {
                                FavoritesManager::Entry e;
                                e.name = eo->getProperty("name").toString();
                                e.file = juce::File(eo->getProperty("path").toString());
                                if (e.name.isNotEmpty() && e.file != juce::File())
                                    f.entries.push_back(e);
                            }
                    favorites.folders.push_back(f);
                }
        }
    }

    refreshTree();
    updateSearch();
    return themeIdx;
}

void LibraryPanel::startExport()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Export Config",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
            .getChildFile("dsk_sfz_config.json"),
        "*.json");

    fileChooser->launchAsync(
        juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting,
        [this](const juce::FileChooser& fc)
        {
            auto dest = fc.getResult();
            if (dest == juce::File()) return;
            if (!dest.hasFileExtension(".json"))
                dest = dest.withFileExtension(".json");
            dest.replaceWithText(juce::JSON::toString(buildConfigJson(), true));
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                "Config Exported",
                "Config saved to:\n" + dest.getFullPathName());
        });
}

void LibraryPanel::startImport()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Import Config",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
        "*.json");

    fileChooser->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc)
        {
            auto src = fc.getResult();
            if (!src.existsAsFile()) return;
            auto parsed = juce::JSON::parse(src.loadFileAsString());
            if (parsed.getDynamicObject() == nullptr)
            {
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                    "Import Failed",
                    "Could not parse the config file.");
            }
            else
            {
                int themeIdx = applyConfigJson(parsed);
                if (themeIdx >= 0 && onThemeChanged)
                    onThemeChanged(themeIdx);
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                    "Config Imported",
                    "Instruments and settings restored from:\n" + src.getFullPathName());
            }
        });
}

void LibraryPanel::saveState(juce::PropertiesFile& props)
{
    props.setValue("instrumentMetadata", juce::JSON::toString(instrumentMetadata.toJson()));
    props.setValue("soundfontFolders", juce::JSON::toString(soundFontFolders.toJson()));
    props.setValue("soundfontNames", juce::JSON::toString(soundFontNames.toJson()));
    props.setValue("libraryFolders", libraryFolderPaths.joinIntoString("|"));
    juce::Array<juce::var> files;
    for (const auto& file : additionalInstrumentFiles) files.add(file);
    props.setValue("instrumentFiles", juce::JSON::toString(juce::var(files)));
    favorites.save(props);
    if (auto xml = presetTree.getOpennessState(true))
        props.setValue("instrumentTreeOpenState", xml->toString());
}

void LibraryPanel::loadState(juce::PropertiesFile& props)
{
    nameSettings = &props;
    instrumentMetadata.fromJson(juce::JSON::parse(props.getValue("instrumentMetadata")));
    updateFilters();
    soundFontNames.fromJson(juce::JSON::parse(props.getValue("soundfontNames")));
    soundFontFolders.fromJson(juce::JSON::parse(props.getValue("soundfontFolders")));
    juce::String saved = props.getValue("libraryFolders", "");
    if (saved.isNotEmpty())
    {
        libraryFolderPaths.clear();
        libraryFolderPaths.addTokens(saved, "|", "");
    }
    favorites.load(props);
    const auto files = juce::JSON::parse(props.getValue("instrumentFiles"));
    if (auto* array = files.getArray())
        for (const auto& file : *array) additionalInstrumentFiles.addIfNotAlreadyThere(file.toString());
    savedTreeOpenState = props.getValue("instrumentTreeOpenState", "");
    refreshTree();
}
