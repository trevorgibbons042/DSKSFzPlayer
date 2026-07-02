#include "DecentSamplerParser.h"

namespace
{
    // "-6dB" / "-6 dB" / "-6" → -6.0f
    float parseDb(const juce::String& s, float def)
    {
        if (s.isEmpty()) return def;
        return s.upToFirstOccurrenceOf("dB", false, true).trim().getFloatValue();
    }

    double attrD(const juce::XmlElement* group, const juce::XmlElement* groups,
                 const char* name, double def)
    {
        if (group  != nullptr && group->hasAttribute(name))  return group->getDoubleAttribute(name, def);
        if (groups != nullptr && groups->hasAttribute(name)) return groups->getDoubleAttribute(name, def);
        return def;
    }
}

bool DecentSamplerParser::parse(const juce::File& dspresetFile, std::vector<SFZRegion>& outRegions)
{
    warnings.clear();
    outRegions.clear();

    auto xml = juce::XmlDocument::parse(dspresetFile);
    if (!xml)
    {
        warnings.add("Could not parse XML: " + dspresetFile.getFullPathName());
        return false;
    }

    auto* groupsEl = xml->getChildByName("groups");
    if (!groupsEl)
    {
        warnings.add("No <groups> element found in " + dspresetFile.getFullPathName());
        return false;
    }

    int groupIndex = 0;
    for (auto* groupEl : groupsEl->getChildIterator())
    {
        if (!groupEl->hasTagName("group")) continue;
        ++groupIndex;

        const double gAttack  = attrD(groupEl, groupsEl, "attack",  0.001);
        const double gDecay   = attrD(groupEl, groupsEl, "decay",   0.0);
        const double gSustain = attrD(groupEl, groupsEl, "sustain", 1.0);
        const double gRelease = attrD(groupEl, groupsEl, "release", 0.05);

        const double gTuning = attrD(groupEl, groupsEl, "tuning", 0.0);
        const double gPan    = attrD(groupEl, groupsEl, "pan",    0.0);
        const float  gVolDb  = parseDb(groupEl->getStringAttribute("volume",
                                        groupsEl->getStringAttribute("volume")), 0.0f);

        const juce::String groupName = groupEl->getStringAttribute("name", "group" + juce::String(groupIndex));

        // Bucket de round-robin: clave = rango de nota/velocidad dentro del grupo.
        // seqLength/seqPosition se asignan por orden de aparición en el XML.
        std::map<std::tuple<int, int, int, int>, std::vector<size_t>> rrBuckets;

        for (auto* sampleEl : groupEl->getChildIterator())
        {
            if (!sampleEl->hasTagName("sample")) continue;

            juce::String path = sampleEl->getStringAttribute("path");
            if (path.isEmpty()) continue;
            path = path.replaceCharacter('\\', '/');

            SFZRegion region;
            region.samplePath = path;
            region.group      = groupName;

            region.pitchKeycenter = sampleEl->getIntAttribute("rootNote", 60);
            region.lokey = sampleEl->getIntAttribute("loNote", 0);
            region.hikey = sampleEl->getIntAttribute("hiNote", 127);
            region.lovel = sampleEl->getIntAttribute("loVel", 0);
            region.hivel = sampleEl->getIntAttribute("hiVel", 127);

            const double sTuning = sampleEl->getDoubleAttribute("tuning", 0.0);
            region.tune = (float)((gTuning + sTuning) * 100.0); // semitonos -> centesimas

            const double sPan = sampleEl->getDoubleAttribute("pan", 0.0);
            region.pan = juce::jlimit(-100.0f, 100.0f, (float)(gPan + sPan));

            const float sVolDb = parseDb(sampleEl->getStringAttribute("volume"), 0.0f);
            region.volume = gVolDb + sVolDb;

            if (sampleEl->hasAttribute("start")) region.offset = sampleEl->getIntAttribute("start", 0);
            if (sampleEl->hasAttribute("end"))   region.end    = sampleEl->getIntAttribute("end", -1);

            const bool loopEnabled = sampleEl->getBoolAttribute("loopEnabled", false);
            if (sampleEl->hasAttribute("loopStart") || sampleEl->hasAttribute("loopEnd"))
            {
                region.loopStart        = sampleEl->getIntAttribute("loopStart", 0);
                region.loopEnd          = sampleEl->getIntAttribute("loopEnd", -1);
                region.loopMode         = loopEnabled ? SFZLoopMode::Continuous : SFZLoopMode::NoLoop;
                region.loopModeExplicit = true;
            }

            region.ampAttack  = (float)gAttack;
            region.ampDecay   = (float)gDecay;
            region.ampSustain = juce::jlimit(0.0f, 100.0f, (float)(gSustain * 100.0));
            region.ampRelease = (float)gRelease;

            outRegions.push_back(region);
            const size_t idx = outRegions.size() - 1;

            const juce::String seqMode = sampleEl->getStringAttribute("seqMode",
                                          groupEl->getStringAttribute("seqMode"));
            if (seqMode.isNotEmpty())
            {
                auto key = std::make_tuple(region.lokey, region.hikey, region.lovel, region.hivel);
                rrBuckets[key].push_back(idx);
            }
        }

        for (auto& bucket : rrBuckets)
        {
            const auto& indices = bucket.second;
            const int len = (int)indices.size();
            if (len <= 1) continue;
            for (int i = 0; i < len; ++i)
            {
                outRegions[indices[(size_t)i]].seqLength   = len;
                outRegions[indices[(size_t)i]].seqPosition = i + 1;
            }
        }
    }

    if (outRegions.empty())
    {
        warnings.add("No <sample> regions found in " + dspresetFile.getFullPathName());
        return false;
    }

    return true;
}
