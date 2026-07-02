// Punto de entrada propio de la aplicación Standalone (solo se compila para
// ese formato — ver JucePlugin_Build_Standalone más abajo). Sustituye a la
// juce::StandaloneFilterApp por defecto (ver CMakeLists.txt,
// JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP) únicamente para poder interceptar
// el botón de cerrar ventana y avisar si hay una grabación MP3 sin guardar.
#include <juce_core/system/juce_TargetPlatform.h>

#if JucePlugin_Build_Standalone

#include <JuceHeader.h>
#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>
#include "../PluginProcessor.h"

namespace
{
    constexpr const char* kRecordFolderKey = "recordDestFolder";
}

class DSKStandaloneWindow : public juce::StandaloneFilterWindow
{
public:
    using juce::StandaloneFilterWindow::StandaloneFilterWindow;

    void closeButtonPressed() override
    {
        if (auto* proc = dynamic_cast<DSKSFzProcessor*>(getAudioProcessor()))
        {
            if (proc->getRecorder().hasUnsavedRecording())
            {
                auto* box = new juce::AlertWindow("Unsaved recording",
                    "You have an unsaved audio recording. Closing now will lose it.",
                    juce::AlertWindow::WarningIcon);
                box->addButton("Save and quit", 1, juce::KeyPress(juce::KeyPress::returnKey));
                box->addButton("Quit without saving", 2);
                box->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

                box->enterModalState(true, juce::ModalCallbackFunction::create([this, proc](int result)
                {
                    if (result == 0)
                        return; // Cancelar: no cerrar la aplicación

                    if (result == 1)
                    {
                        juce::File folder = juce::File::getSpecialLocation(juce::File::userDesktopDirectory);
                        if (auto* s = proc->getAppProperties().getUserSettings())
                        {
                            auto saved = s->getValue(kRecordFolderKey, juce::String());
                            if (saved.isNotEmpty() && juce::File(saved).isDirectory())
                                folder = juce::File(saved);
                        }
                        proc->getRecorder().saveToFile(Mp3Recorder::suggestFileName(folder));
                    }

                    finishClosing();
                }), true);

                return;
            }
        }

        finishClosing();
    }

private:
    void finishClosing()
    {
        pluginHolder->savePluginState();
        juce::JUCEApplicationBase::quit();
    }
};

class DSKStandaloneApp : public juce::JUCEApplication
{
public:
    DSKStandaloneApp()
    {
        juce::PropertiesFile::Options options;
        options.applicationName     = getApplicationName();
        options.filenameSuffix      = ".settings";
        options.osxLibrarySubFolder = "Application Support";
       #if JUCE_LINUX || JUCE_BSD
        options.folderName          = "~/.config";
       #else
        options.folderName          = "";
       #endif
        appProperties.setStorageParameters(options);
    }

    const juce::String getApplicationName() override    { return juce::CharPointer_UTF8(JucePlugin_Name); }
    const juce::String getApplicationVersion() override { return JucePlugin_VersionString; }
    bool moreThanOneInstanceAllowed() override           { return true; }
    void anotherInstanceStarted(const juce::String&) override {}

    void initialise(const juce::String&) override
    {
        mainWindow.reset(new DSKStandaloneWindow(getApplicationName(),
            juce::LookAndFeel::getDefaultLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId),
            appProperties.getUserSettings(), false, {}, nullptr, {}));
        mainWindow->setVisible(true);
    }

    void shutdown() override
    {
        mainWindow = nullptr;
        appProperties.saveIfNeeded();
    }

    // Cierre solicitado por el sistema (Cmd+Q, apagado, etc.): no bloqueamos
    // este camino con el aviso de guardado — solo el botón de cerrar ventana
    // (closeButtonPressed) lo hace, para no interferir con el apagado del SO.
    void systemRequestedQuit() override
    {
        if (mainWindow != nullptr)
            mainWindow->pluginHolder->savePluginState();
        quit();
    }

private:
    juce::ApplicationProperties appProperties;
    std::unique_ptr<DSKStandaloneWindow> mainWindow;
};

juce::JUCEApplicationBase* juce_CreateApplication() { return new DSKStandaloneApp(); }

#endif // JucePlugin_Build_Standalone
