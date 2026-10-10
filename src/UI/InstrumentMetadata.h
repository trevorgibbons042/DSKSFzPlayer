#pragma once
#include <JuceHeader.h>
#include <map>

class InstrumentMetadata
{
public:
    struct Entry { int rating = 0; juce::String category; bool hidden = false; };
    struct Filter
    {
        int minimumRating = 0;
        bool unratedOnly = false;
        juce::String category;
        bool uncategorizedOnly = false;
        bool active() const { return minimumRating > 0 || unratedOnly || category.isNotEmpty() || uncategorizedOnly; }
        bool matches(const Entry& entry) const
        {
            return entry.rating >= minimumRating && (!unratedOnly || entry.rating == 0)
                && (!uncategorizedOnly || entry.category.isEmpty())
                && (category.isEmpty() || entry.category.equalsIgnoreCase(category));
        }
    };

    Entry get(const juce::File& file, int bank = -1, int program = -1) const
    {
        const auto it = entries.find(key(file, bank, program));
        return it == entries.end() ? Entry{} : it->second;
    }
    void setHidden(const juce::File& file, int bank, int program, bool hidden)
    { entries[key(file, bank, program)].hidden = hidden; }
    void setRating(const juce::File& file, int bank, int program, int rating)
    { entries[key(file, bank, program)].rating = juce::jlimit(0, 5, rating); }
    void setCategory(const juce::File& file, int bank, int program, const juce::String& category)
    { entries[key(file, bank, program)].category = category.trim(); }

    juce::StringArray categories() const
    {
        juce::StringArray result { "Drums", "Percussion", "Keys", "Piano", "Organ", "Bass", "Guitar",
            "Strings", "Brass", "Woodwinds", "Lead Synth", "Pad", "Pluck", "Arpeggio", "Vocal", "FX", "Other" };
        for (const auto& item : entries)
            if (item.second.category.isNotEmpty() && !result.contains(item.second.category, true))
                result.add(item.second.category);
        result.sort(true);
        return result;
    }
    static juce::String stars(int rating)
    {
        return juce::String::repeatedString(juce::String::fromUTF8("★"), juce::jlimit(0, 5, rating))
             + juce::String::repeatedString(juce::String::fromUTF8("☆"), 5 - juce::jlimit(0, 5, rating));
    }
    static juce::String details(const Entry& entry)
    {
        juce::String text;
        if (entry.rating > 0) text = stars(entry.rating);
        if (entry.category.isNotEmpty())
            text += (text.isNotEmpty() ? "  " : "") + entry.category;
        return text;
    }
    juce::var toJson() const
    {
        juce::Array<juce::var> array;
        for (const auto& item : entries)
        {
            if (item.second.rating == 0 && item.second.category.isEmpty() && !item.second.hidden) continue;
            auto* object = new juce::DynamicObject();
            object->setProperty("key", item.first);
            object->setProperty("rating", item.second.rating);
            object->setProperty("category", item.second.category);
            object->setProperty("hidden", item.second.hidden);
            array.add(juce::var(object));
        }
        return array;
    }
    void fromJson(const juce::var& value)
    {
        entries.clear();
        if (const auto* array = value.getArray())
            for (const auto& item : *array)
            {
                const auto id = item["key"].toString();
                if (id.isNotEmpty()) entries[id] = { juce::jlimit(0, 5, (int) item["rating"]), item["category"].toString().trim(), (bool) item["hidden"] };
            }
    }
private:
    std::map<juce::String, Entry> entries;
    static juce::String key(const juce::File& file, int bank, int program)
    {
        auto path = file.getFullPathName();
       #if JUCE_WINDOWS
        path = path.toLowerCase();
       #endif
        return juce::String(bank) + ":" + juce::String(program) + ":" + path;
    }
};
