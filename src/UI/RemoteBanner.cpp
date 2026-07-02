#include "RemoteBanner.h"

namespace RemoteBanner
{
    static const char* kBannerConfigUrl = "https://www.dskmusic.com/downloads/ads/dsksfzplayer/banner.json";
    static const char* kTrackUrl        = "https://www.dskmusic.com/downloads/ads/dsksfzplayer/track.php";

    // Debe coincidir exactamente con TRACK_SECRET en .servidor/public/config.php;
    // si se rota uno hay que rotar el otro a la vez.
    static const char* kTrackSecret = "754627f0b8d92257dde112bff1a5148eea967c277c682730";

    Result fetchBlocking()
    {
        Result result;

        auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                           .withConnectionTimeoutMs(4000);

        std::unique_ptr<juce::InputStream> jsonStream(juce::URL(kBannerConfigUrl).createInputStream(options));
        if (jsonStream == nullptr)
            return result;

        const juce::String jsonText = jsonStream->readEntireStreamAsString();
        auto parsed = juce::JSON::parse(jsonText);
        if (!parsed.isObject())
            return result;

        const juce::String imageUrl = parsed.getProperty("image", juce::String()).toString().trim();
        const juce::String destUrl  = parsed.getProperty("url",   juce::String()).toString().trim();
        if (imageUrl.isEmpty() || destUrl.isEmpty())
            return result;

        std::unique_ptr<juce::InputStream> imgStream(juce::URL(imageUrl).createInputStream(options));
        if (imgStream == nullptr)
            return result;

        juce::MemoryBlock imgData;
        imgStream->readIntoMemoryBlock(imgData);
        if (imgData.getSize() == 0)
            return result;

        juce::Image img = juce::ImageFileFormat::loadFrom(imgData.getData(), imgData.getSize());
        if (!img.isValid())
            return result;

        result.ok    = true;
        result.image = img;
        result.url   = destUrl;
        return result;
    }

    // Identificador anónimo persistente por instalación (no ligado a cuenta ni
    // dato personal alguno): permite al dashboard contar usuarios únicos reales,
    // más fiable que agrupar por IP (que cambia con conexiones dinámicas/móviles).
    // Se genera una vez y se guarda en un fichero de texto junto a los ajustes
    // de la app; si no se puede leer/escribir, simplemente se envía vacío.
    static juce::String getOrCreateInstallId()
    {
        auto dir  = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                        .getChildFile("DSK SFz player");
        auto file = dir.getChildFile("install_id.txt");

        auto existing = file.loadFileAsString().trim();
        if (existing.isNotEmpty())
            return existing;

        auto newId = juce::Uuid().toString();
        if (dir.createDirectory())
            file.replaceWithText(newId);
        return newId;
    }

    // Construye y envía la petición GET "fire-and-forget" común a clicks e
    // impresiones. Todo (incluida la lectura/escritura del install id) ocurre
    // en el hilo lanzado aquí, nunca en el hilo llamante.
    static void sendTrackingEvent(const juce::String& event, const juce::String& destinationUrl)
    {
        juce::Thread::launch([event, destinationUrl]
        {
            auto url = juce::URL(kTrackUrl)
                           .withParameter("key",   kTrackSecret)
                           .withParameter("event", event)
                           .withParameter("os",    juce::SystemStats::getOperatingSystemName())
                           .withParameter("ver",   ProjectInfo::versionString)
                           .withParameter("lang",  juce::SystemStats::getUserLanguage())
                           .withParameter("daw",   juce::PluginHostType().getHostDescription())
                           .withParameter("fmt",   juce::AudioProcessor::getWrapperTypeDescription(
                                                        juce::PluginHostType::getPluginLoadedAs()))
                           .withParameter("iid",   getOrCreateInstallId());

            if (destinationUrl.isNotEmpty())
                url = url.withParameter("url", destinationUrl);

            auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                               .withConnectionTimeoutMs(3000);

            // Solo se necesita completar la petición GET; no importa la respuesta.
            std::unique_ptr<juce::InputStream> stream(url.createInputStream(options));
            if (stream != nullptr)
                stream->readEntireStreamAsString();
        });
    }

    void trackClickAsync(const juce::String& destinationUrl)
    {
        sendTrackingEvent("click", destinationUrl);
    }

    void trackImpressionAsync()
    {
        sendTrackingEvent("impression", {});
    }
}
