#pragma once
#include "../SF2/SF2PresetCatalog.h"
#include <set>
#include "InstrumentMetadata.h"
#include "../SF2/SoundFontNames.h"

namespace LibrarySearch
{
struct Result
{
    juce::String name, location;
    juce::File file;
    int bank = -1, program = -1;
    InstrumentMetadata::Entry metadata {};
    juce::String group;
};

inline std::vector<Result> find(const juce::StringArray& folders, const juce::String& query,
                               const std::function<bool()>& cancelled = [] { return false; },
                               const SoundFontNames& names = {},
                               const InstrumentMetadata& metadata = {},
                               const InstrumentMetadata::Filter& filter = {},
                               bool instrumentsOnly = false,
                               const juce::StringArray& additionalFiles = {})
{
    if (query.trim().isEmpty() && !filter.active() && !instrumentsOnly) return {};
    std::vector<Result> results;
    const auto add = [&](Result result)
    {
        result.metadata = metadata.get(result.file, result.bank, result.program);
        if (filter.matches(result.metadata)) results.push_back(std::move(result));
    };
    std::set<juce::String> visited;
    std::vector<juce::File> files;
    for (const auto& path : folders)
    {
        if (cancelled()) return {};
        for (const auto& entry : juce::RangedDirectoryIterator(juce::File(path), true, "*", juce::File::findFiles))
        {
            if (cancelled()) return {};
            files.push_back(entry.getFile());
        }
    }
    for (const auto& path : additionalFiles) files.emplace_back(path);
    for (const auto& file : files)
    {
        if (cancelled()) return {};
        if (!file.hasFileExtension("sfz;sf2;dspreset;dslibrary") ||
            !visited.insert(file.getFullPathName()).second || !file.existsAsFile()) continue;
        const auto bankName = names.get(file, file.getFileNameWithoutExtension());
        const bool bankMatch = query.isEmpty() || bankName.containsIgnoreCase(query)
            || file.getFileNameWithoutExtension().containsIgnoreCase(query);
        if (!file.hasFileExtension("sf2"))
        {
            if (bankMatch || (!instrumentsOnly && file.getFullPathName().containsIgnoreCase(query)))
            {
                Result result { bankName, file.getParentDirectory().getFullPathName(), file };
                result.group = file.getParentDirectory().getFileName();
                add(std::move(result));
            }
        }
        else
        {
            if (!instrumentsOnly && !filter.active()
                && (bankMatch || file.getFullPathName().containsIgnoreCase(query)))
                add({ bankName, file.getParentDirectory().getFullPathName(), file });
            for (const auto& preset : SF2PresetCatalog::read(file, cancelled))
            {
                const auto presetName = names.get(file, preset.name, preset.bank, preset.program);
                if ((instrumentsOnly && bankMatch) || query.isEmpty()
                    || preset.name.containsIgnoreCase(query) || presetName.containsIgnoreCase(query))
                {
                    Result result { presetName,
                        bankName + " / " + file.getFileName() + " / " + juce::String(preset.bank) + ":"
                            + juce::String(preset.program) + " / " + file.getParentDirectory().getFullPathName(),
                        file, preset.bank, preset.program };
                    result.group = bankName;
                    add(std::move(result));
                }
            }
        }
    }
    if (cancelled()) return {};
    std::stable_sort(results.begin(), results.end(), [](const auto& a, const auto& b)
    {
        if (a.metadata.rating != b.metadata.rating) return a.metadata.rating > b.metadata.rating;
        const int compare = a.name.compareIgnoreCase(b.name);
        return compare != 0 ? compare < 0 : a.location.compareIgnoreCase(b.location) < 0;
    });
    return results;
}
}
