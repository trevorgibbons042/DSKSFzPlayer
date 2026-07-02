#pragma once
#include <JuceHeader.h>
#include "../SFZ/SFZRegion.h"

// ──────────────────────────────────────────────────────────────────────────────
// DecentSamplerParser: lee un archivo .dspreset (XML) y devuelve una lista de
// SFZRegion, reutilizando así el motor SFZSynth ya existente para reproducirlo.
//
// Se parsean <groups>/<group>/<sample> (mapeo, ADSR, tuning, volumen, pan,
// loop, start/end y round-robin). Los bloques <effects> (cadena de FX propia
// del preset) y <ui> (skin personalizado) se ignoran deliberadamente: el
// plugin ya renderiza su propia UI fija e independiente del formato, igual
// que hace con SF2.
// ──────────────────────────────────────────────────────────────────────────────
class DecentSamplerParser
{
public:
    DecentSamplerParser() = default;

    // dspresetFile: ruta al .dspreset. Las rutas de sample dentro del XML se
    // resuelven relativas a su carpeta contenedora (igual que SFZParser).
    bool parse(const juce::File& dspresetFile, std::vector<SFZRegion>& outRegions);

    const juce::StringArray& getWarnings() const { return warnings; }

private:
    juce::StringArray warnings;
};
