#include "RecorderBar.h"
#include "../PluginProcessor.h"
#include "DSKLookAndFeel.h"
#include <cmath>

static constexpr const char* kRecordFolderKey = "recordDestFolder";

// ── IconButton::paintButton ─────────────────────────────────────────────────
void RecorderBar::IconButton::paintButton(juce::Graphics& g, bool highlighted, bool down)
{
    const DSKTheme fallback;
    const DSKTheme& t = theme != nullptr ? *theme : fallback;

    auto b = getLocalBounds().toFloat();

    juce::Colour bg = t.sectionBg;
    if (down)             bg = bg.brighter(0.18f);
    else if (highlighted) bg = bg.brighter(0.09f);

    g.setColour(bg);
    g.fillRoundedRectangle(b, 4.0f);

    juce::Colour border = active ? t.accent : t.border;
    if (icon == Icon::Record && active)
        border = t.accent2;
    g.setColour(isEnabled() ? border : border.withAlpha(0.35f));
    g.drawRoundedRectangle(b.reduced(0.5f), 4.0f, 1.2f);

    juce::Colour iconCol = t.text;
    if (icon == Icon::Record) iconCol = active ? t.accent2 : t.text;
    else if (active)          iconCol = t.accent;
    if (! isEnabled())
        iconCol = iconCol.withAlpha(0.35f);

    g.setColour(iconCol);
    auto ic = b.reduced(b.getWidth() * 0.30f);

    switch (icon)
    {
        case Icon::Record:
            g.fillEllipse(ic);
            break;

        case Icon::Stop:
            g.fillRoundedRectangle(ic, 1.5f);
            break;

        case Icon::Play:
        {
            juce::Path p;
            p.addTriangle(ic.getX(), ic.getY(), ic.getX(), ic.getBottom(), ic.getRight(), ic.getCentreY());
            g.fillPath(p);
            break;
        }

        case Icon::Folder:
        {
            juce::Path p;
            const float tabW = ic.getWidth() * 0.55f;
            const float tabH = ic.getHeight() * 0.22f;
            p.addRoundedRectangle(ic.getX(), ic.getY() + tabH, ic.getWidth(), ic.getHeight() - tabH, 1.2f);
            p.addRoundedRectangle(ic.getX(), ic.getY(), tabW, tabH * 1.5f, 1.0f);
            g.fillPath(p);
            break;
        }

        case Icon::Save:
        {
            // Flecha hacia una bandeja: icono universal de "guardar/descargar".
            juce::Path p;
            const float stemW = ic.getWidth() * 0.22f;
            const float cx    = ic.getCentreX();
            const float arrowTopY = ic.getY();
            const float arrowTipY = ic.getBottom() - ic.getHeight() * 0.30f;
            p.addRoundedRectangle(cx - stemW * 0.5f, arrowTopY, stemW, arrowTipY - arrowTopY, stemW * 0.4f);
            p.addTriangle(ic.getX(), arrowTipY - ic.getHeight() * 0.10f,
                          ic.getRight(), arrowTipY - ic.getHeight() * 0.10f,
                          cx, ic.getBottom() - ic.getHeight() * 0.02f);
            juce::Path tray;
            tray.addRoundedRectangle(ic.getX(), ic.getBottom() - ic.getHeight() * 0.14f,
                                      ic.getWidth(), ic.getHeight() * 0.14f, 1.0f);
            p.addPath(tray);
            g.fillPath(p);
            break;
        }
    }
}

// ── RecorderBar ──────────────────────────────────────────────────────────────
RecorderBar::RecorderBar(DSKSFzProcessor& processor) : proc(processor)
{
    for (auto* b : { &recordBtn, &playBtn, &stopBtn, &saveBtn, &folderBtn })
        addAndMakeVisible(b);

    recordBtn.setTooltip("Record to MP3");
    playBtn.setTooltip("Play recording");
    stopBtn.setTooltip("Stop");
    saveBtn.setTooltip("Save recording to MP3");
    folderBtn.setTooltip("Destination folder");

    recordBtn.onClick = [this] { recordClicked(); };
    playBtn.onClick   = [this] { playClicked();  };
    stopBtn.onClick   = [this] { stopClicked();  };
    saveBtn.onClick   = [this] { saveClicked();  };
    folderBtn.onClick = [this] { chooseFolderClicked(); };

    timeLabel.setJustificationType(juce::Justification::centredLeft);
    timeLabel.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 13.5f, juce::Font::bold));
    timeLabel.setText("00:00", juce::dontSendNotification);
    addAndMakeVisible(timeLabel);

    if (auto* s = proc.getAppProperties().getUserSettings())
    {
        auto saved = s->getValue(kRecordFolderKey, juce::String());
        if (saved.isNotEmpty() && juce::File(saved).isDirectory())
            destinationFolder = juce::File(saved);
    }
    if (! destinationFolder.isDirectory())
        destinationFolder = juce::File::getSpecialLocation(juce::File::userDesktopDirectory);
    folderBtn.setTooltip("Destination folder: " + destinationFolder.getFullPathName());

    updateStates();
    startTimerHz(15);
}

RecorderBar::~RecorderBar()
{
    stopTimer();
}

void RecorderBar::applyTheme(const DSKTheme& t)
{
    theme = &t;
    for (auto* b : { &recordBtn, &playBtn, &stopBtn, &saveBtn, &folderBtn })
        b->setTheme(&t);
    timeLabel.setColour(juce::Label::textColourId, t.text);
    repaint();
}

void RecorderBar::paint(juce::Graphics& g)
{
    if (proc.getRecorder().isRecording())
    {
        const float alpha = 0.12f + 0.16f * (0.5f + 0.5f * std::sin(pulsePhase));
        auto glow = recordBtn.getBounds().toFloat().expanded(4.0f);
        g.setColour((theme != nullptr ? theme->accent2 : juce::Colour(0xFFE94560)).withAlpha(alpha));
        g.fillEllipse(glow);
    }
}

void RecorderBar::resized()
{
    auto b = getLocalBounds();
    const int btnSz = b.getHeight();

    recordBtn.setBounds(b.removeFromLeft(btnSz));
    b.removeFromLeft(4);
    playBtn.setBounds(b.removeFromLeft(btnSz));
    b.removeFromLeft(4);
    stopBtn.setBounds(b.removeFromLeft(btnSz));
    b.removeFromLeft(10);

    folderBtn.setBounds(b.removeFromRight(btnSz));
    b.removeFromRight(4);
    saveBtn.setBounds(b.removeFromRight(btnSz));
    b.removeFromRight(6);

    timeLabel.setBounds(b);
}

void RecorderBar::timerCallback()
{
    auto& rec = proc.getRecorder();
    const bool isRec  = rec.isRecording();
    const bool isPlay = rec.isPlaying();

    if (isRec)
    {
        pulsePhase += 0.22f;
        timeLabel.setColour(juce::Label::textColourId,
                             theme != nullptr ? theme->accent2 : juce::Colour(0xFFE94560));
        timeLabel.setText(formatTime(rec.getRecordedSeconds()), juce::dontSendNotification);
        recordBtn.repaint();
    }
    else if (isPlay)
    {
        timeLabel.setColour(juce::Label::textColourId, theme != nullptr ? theme->accent : juce::Colour(0xFF00D4FF));
        timeLabel.setText(formatTime(rec.getPlaybackPositionSeconds()) + " / " + formatTime(rec.getRecordedSeconds()),
                           juce::dontSendNotification);
    }
    else
    {
        timeLabel.setColour(juce::Label::textColourId, theme != nullptr ? theme->text : juce::Colour(0xFFCCDDEE));
        timeLabel.setText(rec.hasRecording() ? formatTime(rec.getRecordedSeconds()) : juce::String("00:00"),
                           juce::dontSendNotification);
    }

    if (rec.didHitLengthLimit())
    {
        rec.acknowledgeLengthLimit();
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Recording",
            "Maximum recording length reached (" + juce::String(Mp3Recorder::kMaxRecordMinutes)
                + " min). Recording has been stopped. Press Save to export it to MP3.");
    }

    if (isRec != wasRecording || isPlay != wasPlaying)
        updateStates();

    wasRecording = isRec;
    wasPlaying   = isPlay;
}

void RecorderBar::updateStates()
{
    auto& rec = proc.getRecorder();
    const bool isRec  = rec.isRecording();
    const bool isPlay = rec.isPlaying();
    const bool hasRec = rec.hasRecording();

    recordBtn.setActive(isRec);
    recordBtn.setTooltip(isRec ? "Stop recording" : "Record to MP3");

    playBtn.setActive(isPlay);
    playBtn.setEnabled(hasRec && ! isRec);

    stopBtn.setEnabled(isRec || isPlay);
    saveBtn.setEnabled(rec.hasUnsavedRecording() && ! isRec);
    folderBtn.setEnabled(! isRec);
}

void RecorderBar::recordClicked()
{
    auto& rec = proc.getRecorder();

    if (rec.isRecording())
    {
        rec.stopRecording();
        updateStates();
        return;
    }

    if (rec.hasUnsavedRecording())
    {
        juce::NativeMessageBox::showOkCancelBox(juce::AlertWindow::WarningIcon, "Recording",
            "There is an unsaved recording. Recording again will discard it.\nContinue and discard it?",
            this, juce::ModalCallbackFunction::create([this](int result)
            {
                if (result != 0)
                    beginNewRecording();
            }));
        return;
    }

    beginNewRecording();
}

void RecorderBar::beginNewRecording()
{
    auto& rec = proc.getRecorder();

    if (rec.isPlaying())
        rec.stopPlayback();

    if (! rec.startRecording())
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Recording",
            "Could not start recording. Make sure the audio device's sample rate is one of the "
            "standard rates "
            "(44100, 48000, 32000, 24000, 22050, 16000, 12000, 11025 or 8000 Hz).");
        return;
    }

    updateStates();
}

void RecorderBar::saveClicked()
{
    auto& rec = proc.getRecorder();
    if (! rec.hasUnsavedRecording())
        return;

    if (! destinationFolder.isDirectory())
        destinationFolder = juce::File::getSpecialLocation(juce::File::userDesktopDirectory);

    const auto file = Mp3Recorder::suggestFileName(destinationFolder);

    if (! rec.saveToFile(file))
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Save recording",
            "Could not save the MP3 file. Make sure the destination folder exists and is writable.");
        return;
    }

    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Save recording",
        "Recording saved to:\n" + file.getFullPathName());

    updateStates();
}

void RecorderBar::playClicked()
{
    auto& rec = proc.getRecorder();
    if (! rec.isRecording() && rec.hasRecording())
        rec.startPlayback();
    updateStates();
}

void RecorderBar::stopClicked()
{
    auto& rec = proc.getRecorder();
    if (rec.isRecording()) rec.stopRecording();
    if (rec.isPlaying())   rec.stopPlayback();
    updateStates();
}

void RecorderBar::chooseFolderClicked()
{
    fileChooser = std::make_unique<juce::FileChooser>("Choose destination folder for recordings",
                                                        destinationFolder);
    const auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories;

    fileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc)
    {
        auto result = fc.getResult();
        if (! result.isDirectory())
            return;

        destinationFolder = result;
        folderBtn.setTooltip("Destination folder: " + destinationFolder.getFullPathName());

        if (auto* s = proc.getAppProperties().getUserSettings())
        {
            s->setValue(kRecordFolderKey, destinationFolder.getFullPathName());
            s->saveIfNeeded();
        }
    });
}

juce::String RecorderBar::formatTime(double seconds)
{
    const int total = (int) seconds;
    const int mm = total / 60;
    const int ss = total % 60;
    return juce::String::formatted("%02d:%02d", mm, ss);
}
