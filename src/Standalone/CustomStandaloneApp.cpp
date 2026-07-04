// Punto de entrada propio de la aplicación Standalone (solo se compila para
// ese formato — ver JucePlugin_Build_Standalone más abajo). Sustituye a la
// juce::StandaloneFilterApp por defecto (ver CMakeLists.txt,
// JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP) para poder interceptar el botón de
// cerrar ventana (avisos de grabación MP3 y de edición de instrumento sin
// guardar) y para cargar el archivo pasado por Windows vía "Open with".
#include <juce_core/system/juce_TargetPlatform.h>

#if JucePlugin_Build_Standalone

#include <JuceHeader.h>
#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>
#include "../PluginProcessor.h"
#include "../PluginEditor.h"

namespace
{
    constexpr const char* kRecordFolderKey = "recordDestFolder";
}

class DSKStandaloneWindow : public juce::StandaloneFilterWindow
{
public:
    using juce::StandaloneFilterWindow::StandaloneFilterWindow;

    // Cierre solicitado por el botón de la ventana: primero comprobamos si
    // hay cambios sin guardar en el editor de instrumento (Options > Edit
    // Instrument...); solo si el usuario decide continuar (guardar o
    // descartar) pasamos a la comprobación existente de grabación MP3 sin
    // guardar.
    void closeButtonPressed() override
    {
        if (auto* editor = dynamic_cast<DSKSFzEditor*>(getAudioProcessor()->getActiveEditor()))
        {
            if (editor->hasUnsavedInstrumentEdits())
            {
                editor->requestCloseInstrumentEditor([this](bool proceed)
                {
                    if (proceed)
                        checkUnsavedRecordingThenClose();
                });
                return;
            }
        }

        checkUnsavedRecordingThenClose();
    }

private:
    void checkUnsavedRecordingThenClose()
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

        loadInstrumentFromCommandLine();
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
    // Windows "Open with" (o doble clic si el usuario fija manualmente la
    // asociación) lanza el standalone pasando la ruta del archivo como
    // argumento de línea de comandos. getCommandLineParameterArray() ya
    // separa correctamente rutas con espacios (usa CommandLineToArgvW en
    // Windows), así que no hace falta parsear el string a mano.
    void loadInstrumentFromCommandLine()
    {
        if (mainWindow == nullptr)
            return;

        for (auto& arg : juce::JUCEApplicationBase::getCommandLineParameterArray())
        {
            juce::File f(arg);
            if (f.existsAsFile() && f.hasFileExtension("sfz;sf2;dspreset;dslibrary"))
            {
                if (auto* proc = dynamic_cast<DSKSFzProcessor*>(mainWindow->getAudioProcessor()))
                    proc->loadInstrument(f);
                break;
            }
        }
    }

    juce::ApplicationProperties appProperties;
    std::unique_ptr<DSKStandaloneWindow> mainWindow;
};

juce::JUCEApplicationBase* juce_CreateApplication() { return new DSKStandaloneApp(); }

#endif // JucePlugin_Build_Standalone
