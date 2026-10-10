#pragma once
#include <JuceHeader.h>
#include <vector>

// Reads only SF2 preset headers; skips sample data rather than loading a synth.
namespace SF2PresetCatalog
{
struct Preset { juce::String name; int bank; int program; };

inline std::vector<Preset> read(const juce::File& file,
                               const std::function<bool()>& cancelled = [] { return false; })
{
    std::vector<Preset> presets;
    auto stream = file.createInputStream();
    if (!stream || stream->getTotalLength() < 12) return presets;
    auto tag = [&]() { char bytes[4]{}; return stream->read(bytes, 4) == 4
        ? juce::String::fromUTF8(bytes, 4) : juce::String(); };
    if (tag() != "RIFF") return presets;
    const auto end = juce::int64(static_cast<juce::uint32>(stream->readInt())) + 8;
    if (end > stream->getTotalLength() || end < 12 || tag() != "sfbk") return presets;
    while (!cancelled() && stream->getPosition() + 8 <= end)
    {
        const auto id = tag();
        const auto size = juce::int64(static_cast<juce::uint32>(stream->readInt()));
        const auto start = stream->getPosition();
        if (size > end - start) return {};
        if (id == "LIST" && size >= 4 && tag() == "pdta")
        {
            const auto listEnd = start + size;
            while (!cancelled() && stream->getPosition() + 8 <= listEnd)
            {
                const auto child = tag();
                const auto length = juce::int64(static_cast<juce::uint32>(stream->readInt()));
                const auto data = stream->getPosition();
                if (length > listEnd - data) return {};
                if (child == "phdr")
                {
                    // 38-byte records, followed by the required terminal record.
                    if (length < 38 || length % 38 != 0) return {};
                    for (juce::int64 i = 0; i < length / 38 - 1; ++i)
                    {
                        if (cancelled()) return {};
                        char name[21]{};
                        if (stream->read(name, 20) != 20) return {};
                        const int program = static_cast<juce::uint16>(stream->readShort());
                        const int bank = static_cast<juce::uint16>(stream->readShort());
                        presets.push_back({ juce::String::fromUTF8(name), bank, program });
                        if (!stream->setPosition(data + (i + 1) * 38)) return {};
                    }
                    return presets;
                }
                if (!stream->setPosition(data + length + (length & 1))) return {};
            }
        }
        if (!stream->setPosition(start + size + (size & 1))) return {};
    }
    return presets;
}
}
