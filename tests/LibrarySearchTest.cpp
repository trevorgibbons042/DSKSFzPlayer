#include "UI/LibrarySearch.h"
#include <iostream>
#include "UI/SoundFontFolders.h"
#include <stdexcept>

static void require(bool ok, const char* message)
{
    if (!ok) throw std::runtime_error(message);
}

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
    const auto root = juce::File::getCurrentWorkingDirectory()
        .getNonexistentChildFile("dsk-library-search-test", "", false);
    struct Cleanup { juce::File folder; ~Cleanup() { folder.deleteRecursively(); } } cleanup { root };
    require(root.getChildFile("nested").createDirectory().wasOk(), "Cannot create fixtures");
    writeFont(root.getChildFile("Orchestra.sf2"), "Acoustic Bass", 5, 32);
    writeFont(root.getChildFile("nested/Synth.SF2"), "Synth BASS", 128, 38);
    writeFont(root.getChildFile("Piano.sf2"), "Grand Piano", 0, 0);
    root.getChildFile("broken.sf2").replaceWithText("RIFFbroken");
    root.getChildFile("Bass.sfz").replaceWithText("<region>");
    const juce::StringArray folders { root.getFullPathName(), root.getChildFile("nested").getFullPathName() };
    auto results = LibrarySearch::find(folders, "bAsS");
    require(results.size() == 3, "Search must find presets across banks plus SFZ, without duplicates");
    int presets = 0;
    for (const auto& result : results)
    {
        if (result.bank < 0) continue;
        ++presets;
        require(result.location.contains(result.file.getFileName()), "Result must identify its SoundFont");
        require((result.bank == 5 && result.program == 32) || (result.bank == 128 && result.program == 38),
                "Result must preserve bank/program for exact preset activation");
    }
    require(presets == 2, "Missing matching SoundFont instruments");
    const auto all = LibrarySearch::find(folders, "", [] { return false; }, {}, {}, {}, true);
    require(all.size() == 4, "Unified browser must show all presets and SFZ files without bank-file rows");
    const auto bankMatches = LibrarySearch::find(folders, "Orchestra", [] { return false; }, {}, {}, {}, true);
    require(bankMatches.size() == 1 && bankMatches[0].bank == 5 && bankMatches[0].group == "Orchestra",
            "Searching a bank must retain its preset group");
    require(LibrarySearch::find(folders, "EOP").empty(), "Terminal record must not be searchable");
    require(LibrarySearch::find(folders, "unmatched").empty(), "Unexpected result");
    require(LibrarySearch::find(folders, "").empty(), "Empty query must not scan");
    require(LibrarySearch::find(folders, "bass", [] { return true; }).empty(), "Cancellation ignored");
    const auto headers = SF2PresetCatalog::read(root.getChildFile("Orchestra.sf2"));
    require(headers.size() == 1 && headers[0].name == "Acoustic Bass", "Preset headers parsed incorrectly");
    // A declared RIFF size beyond EOF must be rejected safely.
    auto truncated = root.getChildFile("Truncated.sf2");
    truncated.replaceWithText("RIFF\xff\xff\xff\x7fsfbk");
    require(SF2PresetCatalog::read(truncated).empty(), "Truncated font accepted");
    SoundFontNames names;
    const auto orchestra = root.getChildFile("Orchestra.sf2");
    const auto synth = root.getChildFile("nested/Synth.SF2");
    const auto originalSize = orchestra.getSize();
    names.set(orchestra, -1, -1, "My Orchestra");
    names.set(orchestra, 5, 32, "  Warm Pluck  ");
    require(names.get(orchestra, "Acoustic Bass", 5, 32) == "Warm Pluck", "Rename did not trim whitespace");
    require(names.get(orchestra, "Other", 6, 32) == "Other", "Rename leaked into another bank");
    require(names.get(synth, "Synth BASS", 5, 32) == "Synth BASS", "Rename leaked into another file");
    SoundFontNames restored;
    restored.fromJson(juce::JSON::parse(juce::JSON::toString(names.toJson())));
    require(restored.get(orchestra, "original", 5, 32) == "Warm Pluck", "Custom name failed to persist");
    const auto renamed = LibrarySearch::find(folders, "pluck", [] { return false; }, restored);
    require(renamed.size() == 1 && renamed[0].bank == 5 && renamed[0].program == 32,
            "Search must find renamed instruments and preserve their identity");
    require(renamed[0].location.contains("My Orchestra"), "Search location lost renamed bank");
    require(LibrarySearch::find(folders, "My Orchestra", [] { return false; }, restored).size() == 1,
            "Search must find renamed SoundFont banks");
    require(LibrarySearch::find(folders, "Acoustic Bass", [] { return false; }, restored).size() == 1,
            "Original instrument names must remain searchable");
    restored.set(orchestra, 5, 32, "   ");
    require(restored.get(orchestra, "Acoustic Bass", 5, 32) == "Acoustic Bass", "Blank must restore original name");
    require(orchestra.getSize() == originalSize && SF2PresetCatalog::read(orchestra)[0].name == "Acoustic Bass",
            "Display names must not change source files");
    InstrumentMetadata metadata;
    metadata.setRating(orchestra, 5, 32, 5);
    metadata.setCategory(orchestra, 5, 32, "Bass");
    metadata.setRating(synth, 128, 38, 3);
    metadata.setCategory(synth, 128, 38, "Lead Synth");
    const auto sfz = root.getChildFile("Bass.sfz");
    metadata.setRating(sfz, -1, -1, 1);
    metadata.setCategory(sfz, -1, -1, "  Vintage Keys  ");
    require(metadata.get(orchestra, 5, 32).rating == 5, "Rating not set");
    require(metadata.get(orchestra, 6, 32).rating == 0 && metadata.get(orchestra, 5, 33).category.isEmpty(),
            "Metadata leaked between SoundFont presets");
    require(metadata.get(synth, 5, 32).rating == 0, "Metadata leaked between files");
    require(metadata.categories().contains("Vintage Keys"), "Custom categories not available for filtering");
    InstrumentMetadata imported;
    imported.fromJson(juce::JSON::parse(juce::JSON::toString(metadata.toJson())));
    require(imported.get(orchestra, 5, 32).rating == 5 && imported.get(sfz).category == "Vintage Keys",
            "Rating/category export and reload failed");
    imported.setHidden(orchestra, 5, 32, true);
    imported.setRating(orchestra, -1, -1, 4);
    InstrumentMetadata restoredMetadata;
    restoredMetadata.fromJson(imported.toJson());
    require(restoredMetadata.get(orchestra, 5, 32).hidden && !restoredMetadata.get(synth, 128, 38).hidden, "Hidden state must round trip per instrument");
    require(restoredMetadata.get(orchestra).rating == 4 && restoredMetadata.get(orchestra, 5, 32).rating == 5, "Bank and preset ratings must remain independent");
    SoundFontFolders ratedFolders;
    const auto folderId = ratedFolders.add("Keys");
    ratedFolders.setRating(folderId, 3);
    ratedFolders.move(orchestra, folderId);
    SoundFontFolders restoredFolders;
    restoredFolders.fromJson(ratedFolders.toJson());
    require(restoredFolders.all()[0].rating == 3 && restoredFolders.folderFor(orchestra) == folderId, "Folder rating export must preserve membership");
    imported.setRating(orchestra, -1, -1, 0);
    InstrumentMetadata::Filter filter;
    filter.minimumRating = 3;
    auto filtered = LibrarySearch::find(folders, "", [] { return false; }, names, imported, filter);
    require(filtered.size() == 2 && filtered[0].metadata.rating == 5 && filtered[1].metadata.rating == 3,
            "Minimum stars must work without a query and sort by rating");
    filter.category = "lead synth";
    filtered = LibrarySearch::find(folders, "bass", [] { return false; }, names, imported, filter);
    require(filtered.size() == 1 && filtered[0].file == synth && filtered[0].bank == 128,
            "Category, rating and query must combine, preserving exact preset identity");
    filter.category = "Bass";
    filtered = LibrarySearch::find(folders, "pluck", [] { return false; }, names, imported, filter);
    require(filtered.size() == 1 && filtered[0].file == orchestra, "Renames must retain metadata and filtering");
    filter = {};
    filter.unratedOnly = true;
    filtered = LibrarySearch::find(folders, "", [] { return false; }, names, imported, filter);
    require(filtered.size() == 1 && filtered[0].name == "Grand Piano", "Unrated filter included rated instruments or banks");
    filter = {};
    filter.uncategorizedOnly = true;
    require(LibrarySearch::find(folders, "", [] { return false; }, names, imported, filter).size() == 1,
            "Uncategorized filter included categorized instruments");
    imported.setRating(orchestra, 5, 32, 0);
    require(imported.get(orchestra, 5, 32).category == "Bass", "Clearing stars must preserve category");
    imported.setCategory(orchestra, 5, 32, "");
    require(imported.get(orchestra, 5, 32).category.isEmpty(), "Cannot clear category");
    imported.setRating(orchestra, 5, 32, 99);
    require(imported.get(orchestra, 5, 32).rating == 5, "Rating must stay within 0–5");
    std::cout << "Library search: cross-bank names, case, nested folders, deduplication, identity, malformed data, cancellation and rename persistence, ratings, categories and combined filters passed.\n";
}
