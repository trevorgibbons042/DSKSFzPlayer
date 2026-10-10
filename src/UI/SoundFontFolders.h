#pragma once
#include <JuceHeader.h>

// Browser collections keep source paths stable, preserving aliases and ratings.
class SoundFontFolders
{
public:
    struct Folder { juce::String id, name; juce::StringArray files; int rating = 0; };
    const std::vector<Folder>& all() const { return folders; }
    juce::String add(const juce::String& name)
    {
        const auto trimmed = name.trim();
        if (trimmed.isEmpty()) return {};
        for (const auto& folder : folders) if (folder.name.equalsIgnoreCase(trimmed)) return folder.id;
        const auto id = juce::Uuid().toString();
        folders.push_back({ id, trimmed, {} });
        return id;
    }
    bool move(const juce::File& file, const juce::String& id)
    {
        if (!file.hasFileExtension("sf2")) return false;
        if (id.isNotEmpty() && std::none_of(folders.begin(), folders.end(), [&](const auto& f) { return f.id == id; })) return false;
        const auto path = file.getFullPathName();
        for (auto& folder : folders)
        {
            folder.files.removeString(path);
            if (folder.id == id) folder.files.add(path);
        }
        return true;
    }
    juce::String folderFor(const juce::File& file) const
    {
        for (const auto& folder : folders) if (folder.files.contains(file.getFullPathName())) return folder.id;
        return {};
    }
    bool rename(const juce::String& id, const juce::String& name)
    {
        if (name.trim().isEmpty()) return false;
        for (const auto& folder : folders)
            if (folder.id != id && folder.name.equalsIgnoreCase(name.trim())) return false;
        for (auto& folder : folders) if (folder.id == id) { folder.name = name.trim(); return true; }
        return false;
    }
    void setRating(const juce::String& id, int rating)
    { for (auto& folder : folders) if (folder.id == id) folder.rating = juce::jlimit(0, 5, rating); }
    void remove(const juce::String& id)
    { folders.erase(std::remove_if(folders.begin(), folders.end(), [&](const auto& f) { return f.id == id; }), folders.end()); }
    juce::var toJson() const
    {
        juce::Array<juce::var> array;
        for (const auto& folder : folders)
        {
            auto* object = new juce::DynamicObject();
            object->setProperty("rating", folder.rating);
            object->setProperty("id", folder.id); object->setProperty("name", folder.name);
            juce::Array<juce::var> paths; for (const auto& file : folder.files) paths.add(file);
            object->setProperty("files", paths); array.add(juce::var(object));
        }
        return array;
    }
    void fromJson(const juce::var& json)
    {
        folders.clear();
        if (const auto* array = json.getArray())
            for (const auto& value : *array)
            {
                const auto id = value["id"].toString(), name = value["name"].toString().trim();
                if (id.isEmpty() || name.isEmpty() || std::any_of(folders.begin(), folders.end(), [&](const auto& f) { return f.id == id; })) continue;
                folders.push_back({ id, name, {}, juce::jlimit(0, 5, (int) value["rating"]) });
                if (const auto* paths = value["files"].getArray())
                    for (const auto& path : *paths) move(juce::File(path.toString()), id);
            }
    }
private:
    std::vector<Folder> folders;
};
