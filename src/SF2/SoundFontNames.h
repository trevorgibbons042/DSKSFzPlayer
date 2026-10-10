#pragma once
#include <JuceHeader.h>
#include <map>

// Display names are keyed by file and MIDI bank/program, never by list order.
class SoundFontNames
{
public:
    juce::String get(const juce::File& file, const juce::String& original,
                     int bank = -1, int program = -1) const
    {
        const auto it = names.find(key(file, bank, program));
        return it == names.end() ? original : it->second;
    }

    void set(const juce::File& file, int bank, int program, const juce::String& name)
    {
        const auto id = key(file, bank, program);
        const auto trimmed = name.trim();
        if (trimmed.isEmpty()) names.erase(id);
        else names[id] = trimmed;
    }

    juce::var toJson() const
    {
        juce::Array<juce::var> entries;
        for (const auto& entry : names)
        {
            auto* object = new juce::DynamicObject();
            object->setProperty("key", entry.first);
            object->setProperty("name", entry.second);
            entries.add(juce::var(object));
        }
        return entries;
    }

    void fromJson(const juce::var& json)
    {
        names.clear();
        if (const auto* entries = json.getArray())
            for (const auto& entry : *entries)
            {
                const auto id = entry["key"].toString();
                const auto name = entry["name"].toString().trim();
                if (id.isNotEmpty() && name.isNotEmpty()) names[id] = name;
            }
    }

private:
    std::map<juce::String, juce::String> names;
    static juce::String key(const juce::File& file, int bank, int program)
    {
        auto path = file.getFullPathName();
       #if JUCE_WINDOWS
        path = path.toLowerCase();
       #endif
        return juce::String(bank) + ":" + juce::String(program) + ":" + path;
    }
};
