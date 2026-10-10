#include <JuceHeader.h>
#include "PluginEditor.h"

// ── Help dialog ───────────────────────────────────────────────────────────────
class HelpDialog : public juce::Component
{
public:
    HelpDialog()
    {
        text.setReadOnly(true);
        text.setMultiLine(true, false);
        text.setScrollbarsShown(true);
        text.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 12.5f, 0));
        text.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xFF0D1828));
        text.setColour(juce::TextEditor::textColourId, juce::Colour(0xFFCCDDEE));
        text.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xFF334466));
        text.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(0xFF334466));

        // Load from embedded binary resource
        const juce::String content = juce::String::fromUTF8(
            reinterpret_cast<const char*>(BinaryData::manual_md),
            BinaryData::manual_mdSize);
        text.setText(content, juce::dontSendNotification);
        text.moveCaretToTop(false);

        closeBtn.setButtonText("Close");
        closeBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF0F3460));
        closeBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFF00D4FF));
        closeBtn.onClick = [this]()
        {
            if (auto* dw = findParentComponentOfClass<juce::DialogWindow>())
                dw->closeButtonPressed();
        };

        addAndMakeVisible(text);
        addAndMakeVisible(closeBtn);
        setSize(720, 580);
    }

    void resized() override
    {
        auto b = getLocalBounds().reduced(6);
        closeBtn.setBounds(b.removeFromBottom(28).removeFromRight(90));
        b.removeFromBottom(6);
        text.setBounds(b);
    }

private:
    juce::TextEditor text;
    juce::TextButton closeBtn;
};

// ── Layout constants ──────────────────────────────────────────────────────────
static constexpr int LIB_W = 260;
static constexpr int HDR_H = 48;
static constexpr int KSZ = 42;
static constexpr int KSLOT = 52;
static constexpr int KNOB_Y = 18;
static constexpr int ROW_H = KNOB_Y + KSZ + 16 + 10;
static constexpr int FTR_H = 30;
static constexpr int KB_H = 56;
static constexpr int VDIV_H = 6; // Altura del tirador horizontal
static constexpr int DEF_H = HDR_H + 4 * ROW_H + FTR_H + VDIV_H + KB_H;
// Alto de arranque por defecto: un poco más alto que el mínimo compacto (DEF_H)
// para dar más espacio vertical a la Librería al abrir el plugin por primera vez.
static constexpr int DEF_STARTUP_H = DEF_H + 90;

static int colX(int rx, int col) { return rx + 8 + col * KSLOT; }

// ══════════════════════════════════════════════════════════════════════════════
// ADSRDisplay
// ══════════════════════════════════════════════════════════════════════════════
void ADSRDisplay::paint(juce::Graphics& g)
{
    auto* dskLF = dynamic_cast<const DSKLookAndFeel*>(&getLookAndFeel());
    const juce::Colour accent = dskLF ? dskLF->getTheme().accent : juce::Colour(0xFF00D4FF);
    const juce::Colour bgCol = dskLF ? dskLF->getTheme().lcdBg : juce::Colour(0xFF0D1828);
    const juce::Colour bdCol = dskLF ? dskLF->getTheme().border : juce::Colour(0xFF334466);

    auto b = getLocalBounds().toFloat().reduced(2.0f);
    g.setColour(bgCol);
    g.fillRoundedRectangle(b, 3.0f);
    g.setColour(bdCol);
    g.drawRoundedRectangle(b, 3.0f, 1.0f);

    float total = attack + decay + 0.25f + release;
    if (total < 0.001f) return;

    float xA = attack / total;
    float xD = xA + decay / total;
    float xS = xD + 0.25f / total;
    float yS = juce::jlimit(0.0f, 1.0f, sustain / 100.0f);

    float pw = b.getWidth() - 4.0f;
    float ph = b.getHeight() - 4.0f;
    float ox = b.getX() + 2.0f;
    float oy = b.getY() + 2.0f;

    auto xp = [&](float t) { return ox + t * pw; };
    auto yp = [&](float v) { return oy + ph * (1.0f - v); };

    juce::Path curve;
    curve.startNewSubPath(xp(0.f), yp(0.f));
    curve.lineTo(xp(xA), yp(1.f));
    curve.lineTo(xp(xD), yp(yS));
    curve.lineTo(xp(xS), yp(yS));
    curve.lineTo(xp(1.f), yp(0.f));

    juce::Path fill = curve;
    fill.lineTo(xp(1.f), yp(0.f));
    fill.lineTo(xp(0.f), yp(0.f));
    fill.closeSubPath();
    g.setColour(accent.withAlpha(0.18f));
    g.fillPath(fill);
    g.setColour(accent);
    g.strokePath(curve, juce::PathStrokeType(1.5f));
}

// ══════════════════════════════════════════════════════════════════════════════
// DSKSFzEditor
// ══════════════════════════════════════════════════════════════════════════════
DSKSFzEditor::DSKSFzEditor(DSKSFzProcessor& p)
    : AudioProcessorEditor(p),
    proc(p),
    titleLink("DSK SFz player", juce::URL("https://www.dskmusic.com")),
    siteLink("www.dskmusic.com", juce::URL("https://www.dskmusic.com")),
    keyboard(p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel(&lf);

    pluginHeight = DEF_STARTUP_H;
    expandedVisualHeight = 90;

    // Restore saved theme before buildUI so colours are applied immediately
    if (auto* s = proc.getAppProperties().getUserSettings())
    {
        lf.setTheme(s->getIntValue("uiTheme", 0));
        guiScale = juce::jlimit(0.5f, 2.0f, (float) s->getDoubleValue("guiScale", 1.0));
        libWidth = juce::jlimit(120, 600, s->getIntValue("libWidth", LIB_W));
        pluginWidth = juce::jlimit(710, 4000, s->getIntValue("pluginWidth", libWidth + 780));
        pluginHeight = juce::jlimit(DEF_H, 1200, s->getIntValue("pluginHeight", DEF_STARTUP_H));
        visualHeight = juce::jlimit(0, 1200, s->getIntValue("visualHeight", juce::jmax(0, pluginHeight - DEF_H)));
        expandedVisualHeight = juce::jlimit(1, 1200, s->getIntValue("expandedVisualHeight", 90));
        juce::StringArray gifs;
        const auto savedGifs = juce::JSON::parse(s->getValue("visualGifs"));
        if (const auto* paths = savedGifs.getArray()) for (const auto& path : *paths) gifs.add(path.toString());
        visualizer.restoreGifs(gifs, s->getValue("visualGif"));
        visualizer.setStyle(s->getIntValue("vizStyle", (int)AudioVisualizer::Style::Spectrum));
        libraryPanel.loadState(*s);
    }

    buildUI();

    libraryPanel.addListener(this);
    libraryPanel.onPresetRequested = [this](int index)
    {
        if (proc.isLoadingInstrument.load()) return;
        if (proc.isSF2Loaded()) proc.selectSF2Preset(index);
        else if (proc.isDSLoaded()) proc.selectDSPreset(index);
        timerCallback();
    };
    libraryPanel.onSoundFontPresetRequested = [this](const juce::File& file, int bank, int program)
    {
        loadLibraryInstrument(file, bank, program);
    };
    libraryPanel.onBankRenameRequested = [this]
    {
        if (proc.isLoadingInstrument.load() || !proc.isSF2Loaded()) return;
        const auto file = proc.getCurrentSFZFile();
        libraryPanel.renameSoundFont(file, file.getFileNameWithoutExtension());
    };
    libraryPanel.onPresetMenuRequested = [this](int index)
    {
        if (proc.isLoadingInstrument.load()) return;
        if (proc.isDSLoaded())
        {
            const auto& files = proc.getDSPresetList();
            if (index >= 0 && index < (int) files.size())
                libraryPanel.showInstrumentMenu({ files[(size_t) index].getFileNameWithoutExtension(), {}, files[(size_t) index] });
            return;
        }
        if (!proc.isSF2Loaded()) return;
        const auto& presets = proc.getSF2Presets();
        if (index < 0 || index >= (int) presets.size()) return;
        const auto& preset = presets[(size_t) index];
        libraryPanel.showInstrumentMenu({ preset.name, {}, proc.getCurrentSFZFile(), preset.bank, preset.presetNumber });
    };
    libraryPanel.onThemeChanged = [this](int idx)
    {
        lf.setTheme(idx);
        sendLookAndFeelChange();
        repaint();
        if (auto* s = proc.getAppProperties().getUserSettings())
            s->setValue("uiTheme", idx);
    };
    libraryPanel.onSFZPreviewRequested = [this](const juce::File& f)
    {
        if (f == proc.getCurrentSFZFile())
            triggerPreviewNote();
        else
            loadInstrumentForPreview(f);
    };

    setSize(pluginWidth, pluginHeight);

    // Habilitar redimensión/maximizado SOLO en Standalone
    if (proc.wrapperType == juce::AudioProcessor::wrapperType_Standalone)
    {
        setResizable(true, true);

        // Pedimos al hilo principal que espere a que el editor se acople a la ventana...
        juce::MessageManager::callAsync([this]()
            {
                // ...buscamos la ventana principal contenedora (DocumentWindow)
                if (auto* dw = findParentComponentOfClass<juce::DocumentWindow>())
                {
                    // ...y le forzamos a mostrar todos los botones (Minimizar, Maximizar y Cerrar)
                    dw->setTitleBarButtonsRequired(juce::DocumentWindow::allButtons, false);
                }
            });
    }
    else
    {
        setResizable(false, false);
    }

    applyWindowConstraints();
    setGuiScale(guiScale);

    startTimerHz(12);
}

DSKSFzEditor::~DSKSFzEditor()
{
    ++loadGeneration;
    libraryLoadPool.removeAllJobs(true, -1);
    stopTimer();
    libraryPanel.removeListener(this);
    if (auto* s = proc.getAppProperties().getUserSettings())
    {
        s->setValue("uiTheme", lf.getThemeIndex());
        s->setValue("guiScale", guiScale);
        s->setValue("libWidth", libWidth);
        s->setValue("pluginWidth", getWidth());
        s->setValue("pluginHeight", pluginHeight);
        juce::Array<juce::var> gifPaths;
        for (const auto& path : visualizer.getGifPaths()) gifPaths.add(path);
        s->setValue("visualGifs", juce::JSON::toString(juce::var(gifPaths)));
        s->setValue("visualGif", visualizer.getGifPath());
        s->setValue("visualHeight", visualHeight);
        s->setValue("expandedVisualHeight", expandedVisualHeight);
        s->setValue("vizStyle", visualizer.getStyle());
        libraryPanel.saveState(*s);
        s->saveIfNeeded();
    }
    setLookAndFeel(nullptr);
}

// ── LookAndFeel changed ───────────────────────────────────────────────────────
void DSKSFzEditor::lookAndFeelChanged()
{
    const auto& t = lf.getTheme();

    instrNameLabel.setColour(juce::Label::textColourId, t.lcdText);
    bankNameLabel.setColour(juce::Label::textColourId, t.subtext);
    statusLabel.setColour(juce::Label::textColourId, t.subtext);
    titleLink.setColour(juce::HyperlinkButton::textColourId, t.accent);
    siteLink.setColour(juce::HyperlinkButton::textColourId, t.subtext);

    openBtn.setColour(juce::TextButton::buttonColourId, t.sectionBg);
    openBtn.setColour(juce::TextButton::textColourOffId, t.accent);
    optionsBtn.setColour(juce::TextButton::buttonColourId, t.sectionBg);
    optionsBtn.setColour(juce::TextButton::textColourOffId, t.subtext);
    for (auto& button : bankRatingButtons)
        button.setColour(juce::TextButton::textColourOffId, t.favGold);
    for (auto& button : ratingButtons)
    {
        button.setColour(juce::TextButton::buttonColourId, t.sectionBg);
        button.setColour(juce::TextButton::textColourOffId, t.favGold);
    }
    categoryButton.setColour(juce::TextButton::buttonColourId, t.sectionBg);
    categoryButton.setColour(juce::TextButton::textColourOffId, t.accent);
    rrResetBtn.setColour(juce::TextButton::buttonColourId, t.sectionBg);
    rrResetBtn.setColour(juce::TextButton::textColourOffId, t.subtext);
    voicesKnob.setColour(juce::Slider::textBoxTextColourId, t.text);

    const juce::Colour wKey = t.isDark ? t.sectionBg.brighter(0.22f) : juce::Colour(0xFFF8F8F8);
    const juce::Colour bKey = t.isDark ? t.editorBg.darker(0.65f) : juce::Colour(0xFF1A1A2A);
    keyboard.setColour(juce::MidiKeyboardComponent::whiteNoteColourId, wKey);
    keyboard.setColour(juce::MidiKeyboardComponent::blackNoteColourId, bKey);
    keyboard.setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, t.accent.withAlpha(0.2f));
    keyboard.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId, t.accent.withAlpha(0.5f));
    keyboard.setColour(juce::MidiKeyboardComponent::keySeparatorLineColourId, t.border);
    keyboard.setColour(juce::MidiKeyboardComponent::textLabelColourId, t.isDark ? t.text : juce::Colours::black);
    keyboard.setColour(juce::MidiKeyboardComponent::shadowColourId, juce::Colours::black.withAlpha(t.isDark ? 0.35f : 0.2f));

    const juce::Colour fxBtnBg = t.isDark ? juce::Colour(0xFF1A2438) : t.sectionBg;
    const juce::Colour fxBtnBgOn = t.sectionBg.brighter(0.2f);
    for (auto* b : { &driveOnBtn, &chorusOnBtn, &phaserOnBtn, &flangerOnBtn, &delayOnBtn, &reverbOnBtn, &eqOnBtn })
    {
        b->setColour(juce::TextButton::buttonColourId, fxBtnBg);
        b->setColour(juce::TextButton::buttonOnColourId, fxBtnBgOn);
        b->setColour(juce::TextButton::textColourOffId, t.subtext);
        b->setColour(juce::TextButton::textColourOnId, t.accent);
    }

    // update knob label colours
    for (auto& kl : kLabels)
        kl.lbl->setColour(juce::Label::textColourId, t.text.withAlpha(0.75f));

    if (recorderBar != nullptr)
        recorderBar->applyTheme(t);

    visualizer.setThemeColours(t.isDark, t.editorBg, t.accent, t.accent2, t.sectionBg);

    repaint();
}

// ── Helpers ───────────────────────────────────────────────────────────────────
void DSKSFzEditor::mk(juce::Slider& s)
{
    s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    s.setPopupDisplayEnabled(true, true, this);
    addAndMakeVisible(s);
}

void DSKSFzEditor::addKL(juce::Slider& s, const juce::String& text)
{
    auto lbl = std::make_unique<juce::Label>();
    lbl->setText(text, juce::dontSendNotification);
    lbl->setFont(juce::Font(11.0f, juce::Font::bold));
    lbl->setColour(juce::Label::textColourId, juce::Colour(0xFFAABBCC));
    lbl->setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    lbl->setJustificationType(juce::Justification::centred);
    lbl->setInterceptsMouseClicks(false, false);
    addAndMakeVisible(*lbl);
    kLabels.push_back({ &s, std::move(lbl) });
}

void DSKSFzEditor::mkFXBtn(juce::TextButton& b, const juce::String& name)
{
    b.setName(name);
    b.setButtonText(name + " OFF");
    b.onStateChange = [&b, name]()
    {
        b.setButtonText(name + (b.getToggleState() ? " ON" : " OFF"));
    };
    b.setClickingTogglesState(true);
    b.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF1A2438));
    b.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xFF00556E));
    b.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFAABBCC));
    b.setColour(juce::TextButton::textColourOnId, juce::Colour(0xFF00D4FF));
    b.setTooltip(name + ": click to enable / disable");
    addAndMakeVisible(b);
}

void DSKSFzEditor::sa(juce::Slider& s, const juce::String& id)
{
    if (auto* parameter = proc.params.getParameter(id)) s.setName(parameter->getName(128));
    sAttach.push_back(std::make_unique<SA>(proc.params, id, s));
}

void DSKSFzEditor::ba(juce::TextButton& b, const juce::String& id)
{
    bAttach.push_back(std::make_unique<BA>(proc.params, id, b));
}

void DSKSFzEditor::ca(juce::ComboBox& c, const juce::String& id)
{
    cAttach.push_back(std::make_unique<CA>(proc.params, id, c));
}

// ── Funciones de detección para tiradores ─────────────────────────────────────
bool DSKSFzEditor::isNearDivider(int x) const
{
    return x >= libWidth && x <= libWidth + 5;
}

bool DSKSFzEditor::isNearHDivider(int y) const
{
    return visualizer.isVisible() && y >= visualizer.getY() - FTR_H - VDIV_H && y < visualizer.getY() - FTR_H;
}

// ── buildUI ───────────────────────────────────────────────────────────────────
void DSKSFzEditor::buildUI()
{
    titleLink.setFont(juce::Font(15.0f, juce::Font::bold), false);
    titleLink.setColour(juce::HyperlinkButton::textColourId, juce::Colour(0xFF00D4FF));
    titleLink.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(titleLink);

    siteLink.setFont(juce::Font(10.0f), false);
    siteLink.setColour(juce::HyperlinkButton::textColourId, juce::Colour(0xFF4488AA));
    siteLink.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(siteLink);

    instrNameLabel.setFont(juce::Font(16.0f, juce::Font::bold)); // Aumentamos la fuente de 13 a 16
    instrNameLabel.setColour(juce::Label::textColourId, juce::Colour(0xFF00E6CC));
    instrNameLabel.setJustificationType(juce::Justification::centredLeft);
    instrNameLabel.setText("No instrument loaded", juce::dontSendNotification);
    addAndMakeVisible(instrNameLabel);
    bankNameLabel.setFont(juce::Font(12.0f));
    bankNameLabel.setJustificationType(juce::Justification::centredLeft);
    bankNameLabel.setTooltip("Click to rename SoundFont");
    bankNameLabel.onClick = [this]
    {
        if (!proc.isLoadingInstrument.load() && proc.isSF2Loaded())
        {
            const auto file = proc.getCurrentSFZFile();
            libraryPanel.renameSoundFont(file, file.getFileNameWithoutExtension());
        }
    };
    addAndMakeVisible(bankNameLabel);

    statusLabel.setFont(juce::Font(10.0f));
    statusLabel.setColour(juce::Label::textColourId, juce::Colour(0xFF7799BB));
    statusLabel.setJustificationType(juce::Justification::centred);
    statusLabel.setText("Ready", juce::dontSendNotification);
    addAndMakeVisible(statusLabel);

    for (size_t i = 0; i < ratingButtons.size(); ++i)
    {
        auto& button = ratingButtons[i];
        button.setButtonText(juce::String::fromUTF8("☆"));
        button.setTooltip("Rate " + juce::String((int) i + 1) + " stars; click the same rating to clear");
        button.onClick = [this, i]
        {
            const auto instrument = currentLibraryInstrument();
            if (instrument.file == juce::File()) return;
            const int rating = (int) i + 1;
            libraryPanel.setInstrumentRating(instrument,
                libraryPanel.instrumentInfo(instrument).rating == rating ? 0 : rating);
            timerCallback();
        };
        addAndMakeVisible(button);
    }
    for (size_t i = 0; i < bankRatingButtons.size(); ++i)
    {
        auto& button = bankRatingButtons[i];
        button.setComponentID("bank-rating");
        button.setTooltip("Rate SoundFont " + juce::String((int) i + 1) + " stars; click again to clear");
        button.onClick = [this, i]
        {
            if (proc.isLoadingInstrument.load() || !proc.isSF2Loaded()) return;
            const LibrarySearch::Result bank { {}, {}, proc.getCurrentSFZFile() };
            const int rating = (int) i + 1;
            libraryPanel.setInstrumentRating(bank, libraryPanel.instrumentInfo(bank).rating == rating ? 0 : rating);
            timerCallback();
        };
        addAndMakeVisible(button);
    }
    categoryButton.setTooltip("Set instrument category");
    categoryButton.onClick = [this]
    {
        const auto instrument = currentLibraryInstrument();
        if (instrument.file != juce::File()) libraryPanel.showCategoryMenu(instrument);
    };
    addAndMakeVisible(categoryButton);

    openBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF0F3460));
    openBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFF00D4FF));
    openBtn.setTooltip("Open an SFZ, SF2 or Decent Sampler file, or a ZIP instrument pack");
    openBtn.onClick = [this]()
    {
        fileChooser = std::make_unique<juce::FileChooser>(
            "Open SFZ/SF2/Decent Sampler instrument", proc.getCurrentSFZFile().getParentDirectory(),
            "*.sfz;*.SFZ;*.sf2;*.SF2;*.dspreset;*.DSPRESET;*.dslibrary;*.DSLIBRARY;*.zip;*.ZIP");
        fileChooser->launchAsync(
            juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this](const juce::FileChooser& fc)
            {
                auto f = fc.getResult();
        if (!f.existsAsFile()) return;
        if (f.hasFileExtension(".zip")) libraryZipRequested(f);
        else                            libraryFileRequested(f);
            });
    };
    addAndMakeVisible(openBtn);

    optionsBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF0F3460));
    optionsBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFF8899AA));
    optionsBtn.setTooltip("Plugin options");
    optionsBtn.onClick = [this]()
    {
        // ── Theme submenu ─────────────────────────────────────────────────────
        const char* themeNames[DSKLookAndFeel::NUM_THEMES] = {
            "Deep Navy", "Midnight", "Forest", "Deep Purple", "Light Gray", "Warm Cream"
        };
        juce::PopupMenu themeMenu;
        for (int i = 0; i < DSKLookAndFeel::NUM_THEMES; ++i)
        {
            bool isCurrent = (lf.getThemeIndex() == i);
            juce::String label = juce::String(themeNames[i]);
            if (i < 4) label += " (dark)"; else label += " (light)";
            themeMenu.addItem(50 + i, label, true, isCurrent);
        }

        juce::PopupMenu menu;
        menu.addSectionHeader("DSK SFz player");
        if (standaloneOption)
        {
            menu.addItem(2001, "Audio / MIDI settings...");
            menu.addItem(2002, "Save plugin state...");
            menu.addItem(2003, "Load plugin state...");
            menu.addItem(2004, "Reset plugin state");
            menu.addSeparator();
        }
        menu.addItem(1, "All notes off");
        menu.addItem(2, "Reset round-robin counters");
        menu.addSeparator();
        menu.addItem(3, "Reset all parameters");
        menu.addSeparator();
        menu.addItem(6, "Edit Instrument...", canEditCurrentInstrument());
        menu.addSeparator();
        menu.addItem(10, "Export config...");
        menu.addItem(11, "Import config...");
        menu.addSeparator();
        menu.addSubMenu("Theme", themeMenu);
        juce::PopupMenu scaleMenu;
        for (const int percent : { 50, 75, 90, 100, 110, 125, 150, 175, 200 })
            scaleMenu.addItem(1000 + percent, juce::String(percent) + "%",
                (float) percent / 100.0f <= maximumGuiScale(),
                juce::roundToInt(guiScale * 100.0f) == percent);
        menu.addSubMenu("GUI Scale", scaleMenu);
        menu.addSeparator();
        menu.addItem(5, "Help...");
        menu.addItem(4, "About...");

        menu.showMenuAsync(
            juce::PopupMenu::Options()
            .withParentComponent(this)
            .withTargetComponent(&optionsBtn),
            [this](int r)
            {
                if (r >= 2001 && r <= 2004 && standaloneOption) { auto action = standaloneOption; action(r - 2000); }
                else if (r >= 1050 && r <= 1200)
                {
                    setGuiScale((float) (r - 1000) / 100.0f);
                }
                else if (r == 1)
                {
                    proc.allNotesOff();
                }
                else if (r == 2)
                {
                    proc.resetRoundRobin();
                }
                else if (r == 3)
                {
                    juce::AlertWindow::showAsync(
                        juce::MessageBoxOptions()
                        .withIconType(juce::MessageBoxIconType::QuestionIcon)
                        .withTitle("Reset All Parameters")
                        .withMessage("Are you sure you want to reset all parameters and interface dimensions to their default values?")
                        .withButton("Reset")
                        .withButton("Cancel"),
                        [this](int result)
                        {
                            if (result == 1) // Si el usuario hace clic en "Reset"
                            {
                                // 1. Restaurar todos los parámetros de audio
                                for (auto* p : proc.getParameters())
                                    p->setValueNotifyingHost(p->getDefaultValue());

                                // 2. Restaurar las dimensiones por defecto de la interfaz
                                libWidth = LIB_W;
                                pluginHeight = DEF_STARTUP_H;
                                setSize(pluginWidth, pluginHeight);
                                applyWindowConstraints();
                                setGuiScale(1.0f);
                            }
                        });
                }
                else if (r == 6)
                {
                    openInstrumentEditor();
                }
                else if (r == 10)
                {
                    libraryPanel.startExport();
                }
                else if (r == 11)
                {
                    libraryPanel.startImport();
                }
                else if (r >= 50 && r < 50 + DSKLookAndFeel::NUM_THEMES)
                {
                    lf.setTheme(r - 50);
                    sendLookAndFeelChange();
                    repaint();
                    if (auto* s = proc.getAppProperties().getUserSettings())
                        s->setValue("uiTheme", r - 50);
                }
                else if (r == 5) // Help
                {
                    juce::DialogWindow::LaunchOptions opts;
                    opts.dialogTitle = "DSK SFz Player  —  Help / Ayuda";
                    opts.content.setOwned(new HelpDialog());
                    opts.resizable = true;
                    opts.useNativeTitleBar = true;
                    opts.componentToCentreAround = this;
                    opts.launchAsync();
                }
                else if (r == 4)
                {
                    auto* dlg = new juce::AlertWindow(
                        "DSK SFz player",
                        "Version 1.0  |  DSK Music\n\n"
                        "64-voice polyphonic SFZ/SF2 sampler.\n"
                        "Supports WAV, FLAC, and OGG formats.\n\n"
                        "Drop SFZ, SF2, folder, or ZIP onto the plugin window to load.\n\n"
                        "Click \"Visit Website\" to open www.dskmusic.com",
                        juce::AlertWindow::InfoIcon, this);
                    dlg->addButton("Visit Website", 1);
                    dlg->addButton("Close", 0);
                    dlg->enterModalState(true,
                        juce::ModalCallbackFunction::create([](int res)
                            {
                                if (res == 1)
                                juce::URL("https://www.dskmusic.com").launchInDefaultBrowser();
                            }), true);
                }
            });
    };
    addAndMakeVisible(optionsBtn);

    addAndMakeVisible(libraryPanel);

    // ── AMP section ───────────────────────────────────────────────────────────
    masterVolKnob.setRange(-60.0, 6.0, 0.1);
    masterVolKnob.setTooltip("Master Volume (-60 to +6 dB)");
    mk(masterVolKnob); addKL(masterVolKnob, "Volume");
    sa(masterVolKnob, "masterVolume");

    masterPanKnob.setRange(-100.0, 100.0, 1.0);
    masterPanKnob.setComponentID("bipolar");
    masterPanKnob.setTooltip("Master Pan (-100 = left, +100 = right)");
    mk(masterPanKnob); addKL(masterPanKnob, "Pan");
    sa(masterPanKnob, "masterPan");

    pitchTransKnob.setRange(-24.0, 24.0, 1.0);
    pitchTransKnob.setComponentID("bipolar");
    pitchTransKnob.setTooltip("Transpose in semitones");
    mk(pitchTransKnob); addKL(pitchTransKnob, "Semit.");
    sa(pitchTransKnob, "transpose");

    pitchFineKnob.setRange(-100.0, 100.0, 0.5);
    pitchFineKnob.setComponentID("bipolar");
    pitchFineKnob.setTooltip("Fine tune in cents");
    mk(pitchFineKnob); addKL(pitchFineKnob, "Cents");
    sa(pitchFineKnob, "fineTune");
    glideKnob.setRange(0.0, 2000.0, 1.0);
    glideKnob.setTooltip("Glide between notes in milliseconds. 0 = off. Use Voices = 1 for solo slides.");
    mk(glideKnob); addKL(glideKnob, "Glide ms");
    sa(glideKnob, "glideTime");

    ampAKnob.setRange(0.001, 10.0, 0.001);
    ampAKnob.setTooltip("Amp Attack (seconds)");
    mk(ampAKnob); addKL(ampAKnob, "A");
    sa(ampAKnob, "ampAttack");

    ampDKnob.setRange(0.001, 10.0, 0.001);
    ampDKnob.setTooltip("Amp Decay (seconds)");
    mk(ampDKnob); addKL(ampDKnob, "D");
    sa(ampDKnob, "ampDecay");

    ampSKnob.setRange(0.0, 100.0, 0.1);
    ampSKnob.setTooltip("Amp Sustain (%)");
    mk(ampSKnob); addKL(ampSKnob, "S");
    sa(ampSKnob, "ampSustain");

    ampRKnob.setRange(0.001, 10.0, 0.001);
    ampRKnob.setTooltip("Amp Release (seconds)");
    mk(ampRKnob); addKL(ampRKnob, "R");
    sa(ampRKnob, "ampRelease");

    addAndMakeVisible(adsrDisplay);

    // ── FILTER section ────────────────────────────────────────────────────────
    filtTypeBox.addItem("LP", 1); filtTypeBox.addItem("HP", 2);
    filtTypeBox.addItem("BP", 3); filtTypeBox.addItem("Notch", 4);
    filtTypeBox.setTooltip("Filter type: Low-Pass / High-Pass / Band-Pass / Notch");
    addAndMakeVisible(filtTypeBox);
    ca(filtTypeBox, "filterType");

    filtCutKnob.setRange(20.0, 20000.0, 1.0);
    filtCutKnob.setSkewFactorFromMidPoint(1200.0);
    filtCutKnob.setTooltip("Cutoff frequency (Hz)");
    mk(filtCutKnob); addKL(filtCutKnob, "Cutoff");
    sa(filtCutKnob, "filterCutoff");

    filtResKnob.setRange(0.1, 20.0, 0.01);
    filtResKnob.setTooltip("Resonance / Q");
    mk(filtResKnob); addKL(filtResKnob, "Res");
    sa(filtResKnob, "filterRes");

    filtEnvKnob.setRange(-1.0, 1.0, 0.001);
    filtEnvKnob.setComponentID("bipolar");
    filtEnvKnob.setTooltip("Filter envelope amount");
    mk(filtEnvKnob); addKL(filtEnvKnob, "Env");
    sa(filtEnvKnob, "filterEnvAmt");

    filtAKnob.setRange(0.001, 10.0, 0.001); mk(filtAKnob); addKL(filtAKnob, "A");
    filtDKnob.setRange(0.001, 10.0, 0.001); mk(filtDKnob); addKL(filtDKnob, "D");
    filtSKnob.setRange(0.0, 100.0, 0.1);   mk(filtSKnob); addKL(filtSKnob, "S");
    filtRKnob.setRange(0.001, 10.0, 0.001); mk(filtRKnob); addKL(filtRKnob, "R");
    filtAKnob.setTooltip("Filter env Attack");  sa(filtAKnob, "filterAttack");
    filtDKnob.setTooltip("Filter env Decay");   sa(filtDKnob, "filterDecay");
    filtSKnob.setTooltip("Filter env Sustain"); sa(filtSKnob, "filterSustain");
    filtRKnob.setTooltip("Filter env Release"); sa(filtRKnob, "filterRelease");

    // ── MOD section ───────────────────────────────────────────────────────────
    modWheelBox.addItem("Wheel: Off", 1);
    modWheelBox.addItem("Wheel: LFO 1", 2);
    modWheelBox.addItem("Wheel: LFO 2", 3);
    modWheelBox.addItem("Wheel: Both", 4);
    modWheelBox.setTooltip("MIDI mod wheel (CC1) controls LFO depth from zero up to the Depth knob setting.");
    addAndMakeVisible(modWheelBox);
    ca(modWheelBox, "modWheelTarget");
    auto buildLFO = [&](juce::ComboBox& shp, juce::ComboBox& tgt,
        juce::Slider& rate, juce::Slider& amt,
        const juce::String& shpId, const juce::String& tgtId,
        const juce::String& rateId, const juce::String& amtId)
    {
        for (auto* c : { &shp, &tgt }) addAndMakeVisible(*c);
        shp.addItem("Sine", 1); shp.addItem("Tri", 2);
        shp.addItem("Saw", 3); shp.addItem("Sq", 4); shp.addItem("S&H", 5);
        tgt.addItem("Pitch", 1); tgt.addItem("Cutoff", 2);
        tgt.addItem("Pan", 3);  tgt.addItem("Vol", 4);
        shp.setTooltip("LFO waveform"); tgt.setTooltip("LFO modulation target");
        rate.setRange(0.1, 20.0, 0.01); rate.setTooltip("LFO rate (Hz)");
        amt.setRange(0.0, 1.0, 0.001); amt.setTooltip("LFO depth");
        mk(rate); addKL(rate, "Rate");
        mk(amt);  addKL(amt, "Depth");
        ca(shp, shpId); ca(tgt, tgtId);
        sa(rate, rateId); sa(amt, amtId);
    };
    buildLFO(lfo1ShapeBox, lfo1TgtBox, lfo1RateKnob, lfo1AmtKnob,
        "lfo1Shape", "lfo1Target", "lfo1Rate", "lfo1Amount");
    buildLFO(lfo2ShapeBox, lfo2TgtBox, lfo2RateKnob, lfo2AmtKnob,
        "lfo2Shape", "lfo2Target", "lfo2Rate", "lfo2Amount");

    // ── FX section ────────────────────────────────────────────────────────────
    mkFXBtn(driveOnBtn, "DRIVE");
    mkFXBtn(chorusOnBtn, "CHORUS");
    mkFXBtn(phaserOnBtn, "PHASER");
    mkFXBtn(flangerOnBtn, "FLANGER");
    ba(phaserOnBtn, "phaserOn"); ba(flangerOnBtn, "flangerOn");
    mkFXBtn(delayOnBtn, "DELAY");
    mkFXBtn(reverbOnBtn, "REVERB");
    mkFXBtn(eqOnBtn, "EQ");
    ba(driveOnBtn, "driveOn"); ba(chorusOnBtn, "chorusOn");
    ba(delayOnBtn, "delayOn"); ba(reverbOnBtn, "reverbOn");
    ba(eqOnBtn, "eqOn");

    driveAmtKnob.setRange(0.0, 1.0, 0.001); mk(driveAmtKnob); addKL(driveAmtKnob, "Drive");
    driveMixKnob.setRange(0.0, 1.0, 0.001); mk(driveMixKnob); addKL(driveMixKnob, "Mix");
    sa(driveAmtKnob, "driveAmount"); sa(driveMixKnob, "driveMix");

    choRateKnob.setRange(0.1, 10.0, 0.01);  mk(choRateKnob);  addKL(choRateKnob, "Rate");
    choDepthKnob.setRange(0.0, 1.0, 0.001); mk(choDepthKnob); addKL(choDepthKnob, "Depth");
    choMixKnob.setRange(0.0, 1.0, 0.001);   mk(choMixKnob);   addKL(choMixKnob, "Mix");
    sa(choRateKnob, "chorusRate"); sa(choDepthKnob, "chorusDepth"); sa(choMixKnob, "chorusMix");

    phaRateKnob.setRange(0.1, 10.0, 0.01); mk(phaRateKnob); addKL(phaRateKnob, "Rate");
    sa(phaRateKnob, "phaserRate");
    phaDepthKnob.setRange(0.0, 1.0, 0.001); mk(phaDepthKnob); addKL(phaDepthKnob, "Depth");
    sa(phaDepthKnob, "phaserDepth");
    phaMixKnob.setRange(0.0, 1.0, 0.001); mk(phaMixKnob); addKL(phaMixKnob, "Mix");
    sa(phaMixKnob, "phaserMix");
    flgRateKnob.setRange(0.1, 10.0, 0.01); mk(flgRateKnob); addKL(flgRateKnob, "Rate");
    sa(flgRateKnob, "flangerRate");
    flgDepthKnob.setRange(0.0, 1.0, 0.001); mk(flgDepthKnob); addKL(flgDepthKnob, "Depth");
    sa(flgDepthKnob, "flangerDepth");
    flgMixKnob.setRange(0.0, 1.0, 0.001); mk(flgMixKnob); addKL(flgMixKnob, "Mix");
    sa(flgMixKnob, "flangerMix");

    delTimeKnob.setRange(1.0, 2000.0, 1.0); mk(delTimeKnob); addKL(delTimeKnob, "Time");
    delFbkKnob.setRange(0.0, 0.95, 0.001);  mk(delFbkKnob);  addKL(delFbkKnob, "Fbk");
    delMixKnob.setRange(0.0, 1.0, 0.001);   mk(delMixKnob);  addKL(delMixKnob, "Mix");
    sa(delTimeKnob, "delayTime"); sa(delFbkKnob, "delayFeedback"); sa(delMixKnob, "delayMix");

    revSizeKnob.setRange(0.0, 1.0, 0.001); mk(revSizeKnob); addKL(revSizeKnob, "Size");
    revDampKnob.setRange(0.0, 1.0, 0.001); mk(revDampKnob); addKL(revDampKnob, "Damp");
    revMixKnob.setRange(0.0, 1.0, 0.001);  mk(revMixKnob);  addKL(revMixKnob, "Mix");
    sa(revSizeKnob, "reverbSize"); sa(revDampKnob, "reverbDamping"); sa(revMixKnob, "reverbMix");

    // Enable an effect when its controls are adjusted by the user. Using the
    // gesture callback leaves preset restoration and host automation alone.
    const auto enableOnAdjust = [](juce::TextButton& button,
                                   std::initializer_list<juce::Slider*> knobs)
    {
        for (auto* knob : knobs)
            knob->onDragStart = [&button]()
            {
                button.setToggleState(true, juce::sendNotification);
            };
    };
    enableOnAdjust(chorusOnBtn, { &choRateKnob, &choDepthKnob, &choMixKnob });
    enableOnAdjust(phaserOnBtn, { &phaRateKnob, &phaDepthKnob, &phaMixKnob });
    enableOnAdjust(flangerOnBtn, { &flgRateKnob, &flgDepthKnob, &flgMixKnob });
    enableOnAdjust(delayOnBtn, { &delTimeKnob, &delFbkKnob, &delMixKnob });
    enableOnAdjust(reverbOnBtn, { &revSizeKnob, &revDampKnob, &revMixKnob });

    eqLowKnob.setRange(-12.0, 12.0, 0.1);  eqLowKnob.setComponentID("bipolar");
    eqMidKnob.setRange(-12.0, 12.0, 0.1);  eqMidKnob.setComponentID("bipolar");
    eqHighKnob.setRange(-12.0, 12.0, 0.1); eqHighKnob.setComponentID("bipolar");
    mk(eqLowKnob); addKL(eqLowKnob, "Low");
    mk(eqMidKnob); addKL(eqMidKnob, "Mid");
    mk(eqHighKnob); addKL(eqHighKnob, "High");
    enableOnAdjust(eqOnBtn, { &eqLowKnob, &eqMidKnob, &eqHighKnob });
    sa(eqLowKnob, "eqLow"); sa(eqMidKnob, "eqMid"); sa(eqHighKnob, "eqHigh");

    // ── Footer ────────────────────────────────────────────────────────────────
    voicesKnob.setRange(1, 64, 1);
    voicesKnob.setSliderStyle(juce::Slider::LinearHorizontal);
    voicesKnob.setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 20);
    voicesKnob.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xFFCCDDEE));
    voicesKnob.setTooltip("Max polyphony (1–64 voices). Type a whole number and press Enter; right-click for numeric entry.");
    addAndMakeVisible(voicesKnob);
    sa(voicesKnob, "maxVoices");

    rrResetBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF0F3460));
    rrResetBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFF8899AA));
    rrResetBtn.setTooltip("Reset round-robin counters");
    rrResetBtn.onClick = [this]() { proc.resetRoundRobin(); };
    addAndMakeVisible(rrResetBtn);

    // Grabador MP3 — solo en Standalone, oculto por completo en VST3
    if (proc.wrapperType == juce::AudioProcessor::wrapperType_Standalone)
    {
        recorderBar = std::make_unique<RecorderBar>(proc);
        addAndMakeVisible(*recorderBar);
    }

    // ── Visualizer ────────────────────────────────────────────────────────────
    addAndMakeVisible(visualizer);
    visualizer.getEQDisplay().onBandGesture = [this](int band, bool starting)
    {
        const juce::String names[] = { "Low", "Mid", "High" };
        const juce::String gainIds[] = { "eqLow", "eqMid", "eqHigh" };
        for (const auto& id : { "eq" + names[band] + "Freq", gainIds[band], "eq" + names[band] + "Width" })
            if (auto* parameter = proc.params.getParameter(id))
            { if (starting) parameter->beginChangeGesture(); else parameter->endChangeGesture(); }
    };
    visualizer.getEQDisplay().onBandEdit = [this](int band, float frequency, float gain, float width)
    {
        const juce::String names[] = { "Low", "Mid", "High" };
        const auto set = [this](const juce::String& id, float value)
        {
            if (auto* parameter = proc.params.getParameter(id))
                parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
        };
        set("eq" + names[band] + "Freq", frequency);
        set("eq" + names[band], gain);
        set("eq" + names[band] + "Width", width);
        if (auto* on = proc.params.getParameter("eqOn"))
            if (on->getValue() < 0.5f)
            { on->beginChangeGesture(); on->setValueNotifyingHost(1.0f); on->endChangeGesture(); }
    };
    visualizer.onStyleChanged = [this](int style)
    {
        if (visualHeight == 0 || (style == (int) AudioVisualizer::Style::EQ && visualHeight < 64))
            setVoicesVisualHeight(style == (int) AudioVisualizer::Style::EQ ? 90 : 64);
        if (auto* s = proc.getAppProperties().getUserSettings())
        { s->setValue("vizStyle", visualizer.getStyle()); s->saveIfNeeded(); }
    };
    visualizer.onGifLibraryChanged = [this]
    {
        if (auto* s = proc.getAppProperties().getUserSettings())
        {
            juce::Array<juce::var> paths;
            for (const auto& path : visualizer.getGifPaths()) paths.add(path);
            s->setValue("visualGifs", juce::JSON::toString(juce::var(paths)));
            s->setValue("visualGif", visualizer.getGifPath());
            s->saveIfNeeded();
        }
    };

    // ── Piano keyboard ────────────────────────────────────────────────────────
    keyboard.setColour(juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour(0xFFD8E8F0));
    keyboard.setColour(juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour(0xFF0D1828));
    keyboard.setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, juce::Colour(0x3300D4FF));
    keyboard.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId, juce::Colour(0x7700D4FF));
    keyboard.setColour(juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour(0xFF334466));
    keyboard.setAvailableRange(21, 108);
    keyboard.setLowestVisibleKey(36);
    keyboard.setScrollButtonsVisible(false);
    addAndMakeVisible(keyboard);

    setWantsKeyboardFocus(true);

    // Refresh the complete component tree after attaching children: library
    // controls may still hold colours from their standalone construction.
    sendLookAndFeelChange();
}

// ── Paint ─────────────────────────────────────────────────────────────────────
void DSKSFzEditor::secBg(juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title)
{
    const auto& t = lf.getTheme();
    g.setColour(t.sectionBg);
    g.fillRoundedRectangle(r.toFloat(), 3.0f);
    g.setColour(t.sectionBdr);
    g.drawRoundedRectangle(r.toFloat(), 3.0f, 1.0f);
    if (title.isNotEmpty())
    {
        g.setColour(t.accent);
        g.setFont(juce::Font(9.0f, juce::Font::bold));
        g.drawText(title, r.getX() + 5, r.getY() + 2, 80, 14, juce::Justification::centredLeft);
    }
}

void DSKSFzEditor::drawMeter(juce::Graphics& g, int x, int y, int w, int h,
    float lvl, const char* ch)
{
    const auto& t = lf.getTheme();
    g.setColour(t.headerBg);
    g.fillRect(x, y, w, h);

    float l = juce::jlimit(0.0f, 1.0f, lvl);
    int   gH = (int)(juce::jmin(l, 0.75f) / 0.75f * h * 0.75f);
    int   yH = (int)(juce::jmax(0.0f, l - 0.75f) / 0.15f * h * 0.15f);
    int   rH = (int)(juce::jmax(0.0f, l - 0.90f) / 0.10f * h * 0.10f);

    g.setColour(juce::Colour(0xFF00BB44));
    g.fillRect(x, y + h - gH, w, gH);
    g.setColour(juce::Colour(0xFFCCBB00));
    g.fillRect(x, y + (int)(h * 0.25f) - yH, w, yH);
    g.setColour(juce::Colour(0xFFCC2200));
    g.fillRect(x, y + (int)(h * 0.10f) - rH, w, rH);

    g.setColour(t.border);
    g.drawRect(x, y, w, h, 1);
    g.setColour(t.subtext);
    g.setFont(juce::Font(8.5f));
    g.drawText(ch, x, y - 12, w, 12, juce::Justification::centred);
}

void DSKSFzEditor::paint(juce::Graphics& g)
{
    const auto& t = lf.getTheme();

    g.fillAll(t.editorBg);

    g.saveState();
    const float controlsScale = juce::jmin(1.0f, (float) (getWidth() - libWidth - VDIV_H) / 780.0f);
    g.addTransform(juce::AffineTransform::scale(controlsScale)
        .translated(libWidth * (1.0f - controlsScale) + VDIV_H, 0.0f));
    const int W = libWidth + juce::jmax(780, getWidth() - libWidth - VDIV_H);
    const int rx = libWidth;
    const int rw = juce::jmax(780, getWidth() - libWidth - VDIV_H); // Fijo visualmente a 780px para no desparramar controles

    // Header bar (Extendemos el fondo hasta W para ocultar el hueco)
    g.setColour(t.headerBg);
    g.fillRect(rx, 0, W - rx, HDR_H);
    g.setColour(t.divider);
    g.drawLine((float)rx, (float)HDR_H, (float)W, (float)HDR_H, 1.5f);

    

    const int S0 = HDR_H;
    const int S1 = S0 + ROW_H;
    const int S2 = S1 + ROW_H;
    const float footerScale = juce::jmin(1.0f, (float) (getWidth() - libWidth - VDIV_H) / 780.0f);
    const int SF = juce::roundToInt((getHeight() - KB_H - juce::jmin(visualHeight, juce::jmax(0, getHeight() - DEF_H)) - FTR_H) / footerScale);

    auto secR = [&](int sy) { return juce::Rectangle<int>(rx + 3, sy + 2, rw - 6, ROW_H - 4); };
    secBg(g, secR(S0), "AMP");

    {
        auto r = secR(S1);
        secBg(g, r, "");
        const int divX = rx + rw * 54 / 100;
        g.setColour(t.sectionBdr);
        g.drawLine((float)divX, (float)(r.getY() + 4), (float)divX, (float)(r.getBottom() - 4), 1.0f);
        g.setColour(t.accent);
        g.setFont(juce::Font(10.5f, juce::Font::bold));
        g.drawText("FILTER", r.getX() + 5, r.getY() + 2, 80, 14, juce::Justification::centredLeft);
        g.drawText("MOD", divX + 6, r.getY() + 2, 80, 14, juce::Justification::centredLeft);
    }

    secBg(g, secR(S2), "");
    secBg(g, secR(S2 + ROW_H), "");

    // Footer strip (Extendemos el fondo hasta W)
    g.setColour(t.footerBg);
    g.fillRect(rx, SF, W - rx, FTR_H);
    g.setColour(t.text.withAlpha(0.75f));
    g.setFont(juce::Font(11.0f, juce::Font::bold));
    g.drawText("Voices:", rx + 10, SF + 7, 56, 16, juce::Justification::centredLeft);

    // Meters
    const int mW = 10, mGap = 4;
    const int mX = rx + rw - 2 * (mW + mGap) - 4;
    const int mY = S0 + KNOB_Y;
    const int mH = KSZ + 16;
    drawMeter(g, mX, mY, mW, mH, meterL.load(), "L");
    drawMeter(g, mX + mW + mGap, mY, mW, mH, meterR.load(), "R");
    g.restoreState();
    const int H = getHeight();
    g.setColour(t.divider);
    g.fillRect(libWidth, 0, VDIV_H, H - KB_H);
    const int divY = visualizer.getY() - FTR_H - VDIV_H;
    if (visualizer.isVisible())
    {
        g.setColour(t.sectionBg);
        g.fillRect(libWidth + VDIV_H, divY, getWidth() - libWidth - VDIV_H, VDIV_H);
        g.setColour(t.accent.withAlpha(0.6f));
        const int centre = (libWidth + getWidth()) / 2;
        g.drawLine((float) centre - 20, (float) divY + 3, (float) centre + 20, (float) divY + 3, 2.0f);
    }

}

// ── overlay visual sobre TODOS los componentes ────────────────────────────────
void DSKSFzEditor::paintOverChildren(juce::Graphics& g)
{
    if (isDragOver)
    {
        const auto& t = lf.getTheme();

        // Oscurecemos toda la interfaz, incluyendo la librería y el teclado
        g.setColour(t.accent.withAlpha(0.20f));
        g.fillAll();

        // Borde exterior
        g.setColour(t.accent);
        g.drawRect(getLocalBounds(), 3);

        // Fondo semi-transparente para asegurar que el texto se lea perfecto
        auto textBounds = getLocalBounds().withSizeKeepingCentre(360, 40);
        g.setColour(juce::Colour(0xD9000000));
        g.fillRoundedRectangle(textBounds.toFloat(), 6.0f);

        g.setColour(t.accent);
        g.setFont(juce::Font(18.0f, juce::Font::bold));
        g.drawText("Drop an instrument, GIF, folder, or ZIP",
            getLocalBounds(), juce::Justification::centred);
    }
}

// ── resized ───────────────────────────────────────────────────────────────────
void DSKSFzEditor::resized()
{
    // Keep the split independent of host/OS window dimensions.
    libWidth = juce::jlimit(120, juce::jmax(120, juce::jmin(600, getWidth() - 590)), libWidth);
    pluginWidth = getWidth();
    pluginHeight = getHeight();

    const int W = getWidth();
    const int H = getHeight();
    const int rx = libWidth;
    const int rw = juce::jmax(780, getWidth() - libWidth - VDIV_H); // Fijo a 780px para que los knobs se queden agrupados

    // Los controles de la derecha conservan su posición Y anclada respecto a la parte superior
    const int S0 = HDR_H;
    const int S1 = S0 + ROW_H;
    const int S2 = S1 + ROW_H;
    const float footerScale = juce::jmin(1.0f, (float) (getWidth() - libWidth - VDIV_H) / 780.0f);
    const int SF = juce::roundToInt((getHeight() - KB_H - juce::jmin(visualHeight, juce::jmax(0, getHeight() - DEF_H)) - FTR_H) / footerScale);

    // La librería ocupa todo el alto hasta el tirador horizontal
    libraryPanel.setBounds(0, 0, libWidth, H - KB_H);
    keyboard.setBounds(0, H - KB_H, W, KB_H);
    keyboard.setKeyWidth(juce::jmax(10.0f, (float)W / 52.0f));

    const int hdrLabelX = rx + 8;
    titleLink.setBounds(hdrLabelX, 4, 160, 24);
    siteLink.setBounds(hdrLabelX, 28, 120, 16);
    const int openBtnW = 160; // ancho aumentado para que el texto largo no se vea diminuto
    const int openBtnX = rx + rw - 94 - openBtnW;
    openBtn.setBounds(openBtnX, 3, openBtnW, 21);
    optionsBtn.setBounds(rx + rw - 90, 3, 82, 21);

    // Botón de favoritos: posición fija justo antes del botón Open.
    const int favBtnW = 100;
    const int favBtnX = openBtnX - 10 - favBtnW;
    for (size_t i = 0; i < ratingButtons.size(); ++i)
    {
        bankRatingButtons[i].setBounds(favBtnX + (int) i * 20, 2, 20, 20);
        ratingButtons[i].setBounds(favBtnX + (int) i * 20, 25, 20, 20);
    }
    categoryButton.setBounds(openBtnX, 27, openBtnW, 18);

    // Nombre del preset / estado: centrados entre el final del texto "DSK SFz player"
    // (izquierda) y el botón de favoritos (derecha) — referencia pedida por el usuario.
    const juce::Font titleFontForWidth(15.0f, juce::Font::bold);
    const int titleTextEndX = hdrLabelX + (int)titleFontForWidth.getStringWidth("DSK SFz player");
    const int lcdX = titleTextEndX + 14;
    const int lcdW = juce::jmax(60, (favBtnX - 10) - lcdX);
    bankNameLabel.setBounds(lcdX, 2, lcdW, 20);
    instrNameLabel.setBounds(lcdX, 25, lcdW, 20);
    statusLabel.setBounds(rx + rw - 90, 28, 82, 16);

    auto K = [&](juce::Slider& s, int col, int secY)
    { s.setBounds(colX(rx, col), secY + KNOB_Y, KSZ, KSZ); };

    K(masterVolKnob, 0, S0); K(masterPanKnob, 1, S0);
    K(pitchTransKnob, 2, S0); K(pitchFineKnob, 3, S0); K(glideKnob, 4, S0);
    K(ampAKnob, 5, S0); K(ampDKnob, 6, S0);
    K(ampSKnob, 7, S0); K(ampRKnob, 8, S0);

    const int mW = 10, mGap = 4;
    const int mX = rx + rw - 2 * (mW + mGap) - 4;
    const int adsrX = colX(rx, 9) + 4;
    adsrDisplay.setBounds(adsrX, S0 + KNOB_Y, mX - adsrX - 4, KSZ);

    {
        const int filterZoneW = rw * 54 / 100;
        const int divX = rx + filterZoneW;
        const int KSM = 36, SLOT = 42, comboW = 74;
        const int comboY = S1 + KNOB_Y + (KSM - 20) / 2;
        int fx0 = rx + 8;
        filtTypeBox.setBounds(fx0, comboY, comboW, 20);
        int kx = fx0 + comboW + 6;
        auto KF = [&](juce::Slider& s) { s.setBounds(kx, S1 + KNOB_Y, KSM, KSM); kx += SLOT; };
        KF(filtCutKnob); KF(filtResKnob); KF(filtEnvKnob);
        kx += 4;
        KF(filtAKnob); KF(filtDKnob); KF(filtSKnob); KF(filtRKnob);

        modWheelBox.setBounds(divX + 54, S1 + 1, 126, 16);
        const int modZoneW = rw - filterZoneW;
        const int lfoComboW = juce::jmin(72, (modZoneW / 2 - 8 - 2 * KSLOT));
        const int lfoComboH = 18;
        int lfo1X = divX + 8;
        lfo1ShapeBox.setBounds(lfo1X, S1 + KNOB_Y, lfoComboW, lfoComboH);
        lfo1TgtBox.setBounds(lfo1X, S1 + KNOB_Y + 22, lfoComboW, lfoComboH);
        lfo1RateKnob.setBounds(lfo1X + lfoComboW + 4, S1 + KNOB_Y, KSZ, KSZ);
        lfo1AmtKnob.setBounds(lfo1X + lfoComboW + 4 + KSLOT, S1 + KNOB_Y, KSZ, KSZ);

        int lfo2X = divX + modZoneW / 2 + 4;
        lfo2ShapeBox.setBounds(lfo2X, S1 + KNOB_Y, lfoComboW, lfoComboH);
        lfo2TgtBox.setBounds(lfo2X, S1 + KNOB_Y + 22, lfoComboW, lfoComboH);
        lfo2RateKnob.setBounds(lfo2X + lfoComboW + 4, S1 + KNOB_Y, KSZ, KSZ);
        lfo2AmtKnob.setBounds(lfo2X + lfoComboW + 4 + KSLOT, S1 + KNOB_Y, KSZ, KSZ);
    }

    struct FXSlot { juce::TextButton* btn; std::vector<juce::Slider*> knobs; };
    FXSlot fxSlots[] = {
        { &driveOnBtn,  { &driveAmtKnob, &driveMixKnob } },
        { &chorusOnBtn, { &choRateKnob, &choDepthKnob, &choMixKnob } },
        { &phaserOnBtn, { &phaRateKnob, &phaDepthKnob, &phaMixKnob } },
        { &flangerOnBtn, { &flgRateKnob, &flgDepthKnob, &flgMixKnob } },
        { &delayOnBtn,  { &delTimeKnob, &delFbkKnob, &delMixKnob } },
        { &reverbOnBtn, { &revSizeKnob, &revDampKnob, &revMixKnob } },
        { &eqOnBtn,     { &eqLowKnob, &eqMidKnob, &eqHighKnob } },
    };
    for (int i = 0; i < 7; ++i)
    {
        const int fxSlot = (rw - 6) / (i < 4 ? 4 : 3);
        const int rowY = S2 + (i < 4 ? 0 : ROW_H);
        const int x0 = rx + 3 + (i < 4 ? i : i - 4) * fxSlot;
        fxSlots[i].btn->setBounds(x0 + 2, rowY + 4, fxSlot - 4, 14);
        const int nk = (int)fxSlots[i].knobs.size();
        const int avail = fxSlot - 4;
        const int kw = juce::jmin(KSZ, (avail - (nk - 1) * 2) / nk);
        const int kx0 = x0 + 2 + (avail - nk * kw - (nk - 1) * 2) / 2;
        for (int j = 0; j < nk; ++j)
            fxSlots[i].knobs[j]->setBounds(kx0 + j * (kw + 2), rowY + KNOB_Y + 2, kw, kw);
    }

    voicesKnob.setBounds(rx + 66, SF + 5, 130, 20);
    rrResetBtn.setBounds(rx + rw - 80, SF + 4, 74, 22);
    if (recorderBar != nullptr)
        recorderBar->setBounds(rx + 210, SF + 4, (rx + rw - 86) - (rx + 210), 22);

    // Only an explicit visual-size change alters its height. Window resizing
    // moves the visual down with the keyboard without stretching it vertically.
    const int available = juce::jmax(0, H - DEF_H);
    const int actualHeight = juce::jmin(visualHeight, available);
    visualizer.setBounds(rx + VDIV_H, H - KB_H - actualHeight, W - rx - VDIV_H, actualHeight);
    visualizer.setVisible(actualHeight > 0);

    for (auto& kl : kLabels)
        kl.lbl->setBounds(kl.s->getX(), kl.s->getBottom() + 2, kl.s->getWidth(), 14);
    const float controlsScale = juce::jmin(1.0f, (float) (W - rx - VDIV_H) / 780.0f);
    const auto transform = juce::AffineTransform::scale(controlsScale)
        .translated(rx * (1.0f - controlsScale) + VDIV_H, 0.0f);
    for (auto* child : getChildren())
        if (child != &libraryPanel && child != &keyboard && child != &visualizer
            && dynamic_cast<juce::ResizableCornerComponent*>(child) == nullptr
            && dynamic_cast<juce::ResizableBorderComponent*>(child) == nullptr)
            child->setTransform(transform);
    repaint();
}

// ── timerCallback ─────────────────────────────────────────────────────────────
void DSKSFzEditor::timerCallback()
{
    // Parameter attachments can change toggle state without button callbacks
    // when restoring a preset or receiving host automation.
    for (auto* button : { &driveOnBtn, &chorusOnBtn, &phaserOnBtn, &flangerOnBtn, &delayOnBtn, &reverbOnBtn, &eqOnBtn })
        button->setButtonText(button->getName() + (button->getToggleState() ? " ON" : " OFF"));

    const auto instrument = currentLibraryInstrument();
    const auto info = libraryPanel.instrumentInfo(instrument);
    for (size_t i = 0; i < ratingButtons.size(); ++i)
    {
        ratingButtons[i].setEnabled(instrument.file != juce::File());
        ratingButtons[i].setFilled((int) i < info.rating);
    }
    categoryButton.setEnabled(instrument.file != juce::File());
    categoryButton.setButtonText(info.category.isEmpty() ? "Category..." : info.category);
    if (proc.isLoadingInstrument.load()) return;
    auto sfz = proc.getCurrentSFZFile();
    libraryPanel.setLoadedInstrument(instrument);
    const bool sf2Mode = proc.isSF2Loaded();

    const bool hasBank = sf2Mode;
    bankNameLabel.setVisible(hasBank);
    bankNameLabel.setText("SoundFont: " + libraryPanel.soundFontName(sfz, sfz.getFileNameWithoutExtension()), juce::dontSendNotification);
    const LibrarySearch::Result bank { {}, {}, sfz };
    const int bankRating = hasBank ? libraryPanel.instrumentInfo(bank).rating : 0;
    for (size_t i = 0; i < bankRatingButtons.size(); ++i)
    {
        bankRatingButtons[i].setVisible(hasBank);
        bankRatingButtons[i].setFilled((int) i < bankRating);
    }
    const auto name = instrument.file != juce::File() ? instrument.name : "No instrument loaded";
    instrNameLabel.setText(name, juce::dontSendNotification);
    instrNameLabel.setTooltip(instrument.file != juce::File() ? "Click to rename instrument" : "");
    instrNameLabel.onClick = instrument.file != juce::File() ? std::function<void()>([this]
    {
        const auto current = currentLibraryInstrument();
        if (current.file != juce::File())
            libraryPanel.renameSoundFont(current.file, current.name, current.bank, current.program);
    }) : nullptr;

    statusLabel.setText(
        proc.isLoadingInstrument.load() ? "Loading..." : "Ready",
        juce::dontSendNotification);

    auto gf = [this](const char* id) -> float
    {
        auto* p = proc.params.getRawParameterValue(id);
        return p ? p->load() : 0.0f;
    };
    int spectrumStart1, spectrumSize1, spectrumStart2, spectrumSize2;
    proc.spectrumFifo.prepareToRead(proc.spectrumFifo.getNumReady(), spectrumStart1, spectrumSize1, spectrumStart2, spectrumSize2);
    for (int i = 0; i < spectrumSize1; ++i) visualizer.pushSpectrumSample(proc.spectrumSamples[(size_t) (spectrumStart1 + i)]);
    for (int i = 0; i < spectrumSize2; ++i) visualizer.pushSpectrumSample(proc.spectrumSamples[(size_t) (spectrumStart2 + i)]);
    proc.spectrumFifo.finishedRead(spectrumSize1 + spectrumSize2);
    EQBandSettings eqBands { { gf("eqLowFreq"), gf("eqMidFreq"), gf("eqHighFreq") },
        { gf("eqLowWidth"), gf("eqMidWidth"), gf("eqHighWidth") } };
    visualizer.setEQResponse(proc.getSampleRate(), gf("eqLow"), gf("eqMid"), gf("eqHigh"), gf("eqOn") > 0.5f, eqBands);
    adsrDisplay.attack = gf("ampAttack");
    adsrDisplay.decay = gf("ampDecay");
    adsrDisplay.sustain = gf("ampSustain");
    adsrDisplay.release = gf("ampRelease");
    adsrDisplay.repaint();

    float pl, pr;
    if (proc.getCurrentFormat() == InstrumentFormat::SF2)
    {
        auto& syn = proc.getSF2Synth();
        pl = syn.meterLevelL.exchange(0.0f);
        pr = syn.meterLevelR.exchange(0.0f);
    }
    else
    {
        auto& syn = proc.getSynth();
        pl = syn.meterLevelL.exchange(0.0f);
        pr = syn.meterLevelR.exchange(0.0f);
    }
    meterL.store(juce::jmax(meterL.load() * 0.88f, pl));
    meterR.store(juce::jmax(meterR.load() * 0.88f, pr));
    visualizer.pushLevel(meterL.load(), meterR.load());

    if (previewCountdown > 0)
        if (--previewCountdown == 0)
            proc.keyboardState.noteOff(1, 60, 0.0f);

    repaint();
}

// ── PC keyboard → MIDI note forwarding ───────────────────────────────────────
bool DSKSFzEditor::keyPressed(const juce::KeyPress& key)
{
    if (libraryPanel.navigateInstruments(key)) return true;
    return keyboard.keyPressed(key);
}

bool DSKSFzEditor::keyStateChanged(bool isKeyDown)
{
    return keyboard.keyStateChanged(isKeyDown);
}

// ── Library callbacks ─────────────────────────────────────────────────────────
LibrarySearch::Result DSKSFzEditor::currentLibraryInstrument() const
{
    if (proc.isLoadingInstrument.load()) return {};
    auto file = proc.getCurrentSFZFile();
    if (!file.existsAsFile()) return {};
    if (proc.isSF2Loaded())
    {
        for (const auto& preset : proc.getSF2Presets())
            if (preset.index == proc.getSF2CurrentPresetIndex())
                return { libraryPanel.soundFontName(file, preset.name, preset.bank, preset.presetNumber), {}, file,
                         preset.bank, preset.presetNumber };
        return {};
    }
    if (proc.isDSLoaded())
    {
        const auto& files = proc.getDSPresetList();
        const int selected = proc.getDSCurrentPresetIndex();
        if (selected >= 0 && selected < (int) files.size()) file = files[(size_t) selected];
    }
    return { libraryPanel.soundFontName(file, file.getFileNameWithoutExtension()), {}, file };
}

void DSKSFzEditor::libraryFileRequested(const juce::File& file)
{
    loadLibraryInstrument(file);
}

void DSKSFzEditor::loadLibraryInstrument(const juce::File& file, int bank, int program)
{
    statusLabel.setText("Loading...", juce::dontSendNotification);
    const int currentGen = ++loadGeneration;
    const juce::Component::SafePointer<DSKSFzEditor> safeThis(this);
    libraryLoadPool.addJob([this, safeThis, file, bank, program, currentGen]()
    {
        if (currentGen != loadGeneration.load()) return;
        // Stepping through search hits in the same SoundFont only changes the
        // preset; keep the bank's sample data loaded.
        bool loaded = bank >= 0 && !proc.isLoadingInstrument.load() && proc.isSF2Loaded()
            && proc.getCurrentSFZFile() == file;
        if (!loaded) loaded = proc.loadInstrument(file);
        if (currentGen != loadGeneration.load()) return;
        if (loaded && bank >= 0)
        {
            loaded = false;
            for (const auto& preset : proc.getSF2Presets())
                if (preset.bank == bank && preset.presetNumber == program)
                {
                    loaded = proc.selectSF2Preset(preset.index);
                    break;
                }
        }
        juce::MessageManager::callAsync([safeThis, currentGen, loaded]()
        {
            if (safeThis && currentGen == safeThis->loadGeneration.load())
            {
                safeThis->onInstrumentLoaded();
                safeThis->statusLabel.setText(loaded ? "Ready" : "Could not load instrument", juce::dontSendNotification);
            }
        });
    });
}

void DSKSFzEditor::libraryZipRequested(const juce::File& zipFile)
{
    libraryPanel.handleFileActivated(zipFile, true);
}

// ── Drag and Drop ─────────────────────────────────────────────────────────────
bool DSKSFzEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (auto& f : files)
    {
        juce::File file(f);
        if (file.hasFileExtension("sfz;sf2;dspreset;dslibrary;gif") || file.hasFileExtension(".zip") || file.isDirectory())
            return true;
    }
    return false;
}

void DSKSFzEditor::fileDragEnter(const juce::StringArray&, int, int)
{
    isDragOver = true;
    repaint();
}

void DSKSFzEditor::fileDragExit(const juce::StringArray&)
{
    isDragOver = false;
    repaint();
}

void DSKSFzEditor::filesDropped(const juce::StringArray& files, int, int)
{
    isDragOver = false;
    repaint();

    bool sfzLoaded = false;
    bool zipLoaded = false;
    bool gifLoaded = false;

    for (auto& f : files)
    {
        juce::File file(f);
        if (file.isDirectory())
        {
            libraryPanel.addFolderDirect(file);
        }
        else if (!gifLoaded && file.hasFileExtension("gif"))
        {
            visualizer.loadGif(file);
            gifLoaded = true;
        }
        else if (!sfzLoaded && file.hasFileExtension("sfz;sf2;dspreset;dslibrary"))
        {
            libraryFileRequested(file);
            sfzLoaded = true;
        }
        else if (!zipLoaded && file.hasFileExtension(".zip"))
        {
            libraryZipRequested(file);
            zipLoaded = true;
        }
    }
}

// ── Divider drag (browser width and independent visual height) ─────────────────
void DSKSFzEditor::mouseMove(const juce::MouseEvent& e)
{
    bool nearV = isNearDivider(e.x) && e.y < getHeight() - KB_H - VDIV_H;
    bool nearH = e.x >= libWidth + VDIV_H && isNearHDivider(e.y);

    if (nearV)       setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
    else if (nearH)  setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
    else             setMouseCursor(juce::MouseCursor::NormalCursor);
}

void DSKSFzEditor::mouseDown(const juce::MouseEvent& e)
{
    bool nearV = isNearDivider(e.x) && e.y < getHeight() - KB_H - VDIV_H;
    bool nearH = e.x >= libWidth + VDIV_H && isNearHDivider(e.y);

    if (!nearV && !nearH) return;

    if (e.getNumberOfClicks() >= 2)
    {
        if (nearV) libWidth = LIB_W;
        if (nearH)
        {
            toggleCompactVoicesArea();
            return;
        }

        resized();
        return;
    }

    if (nearV)
    {
        isDraggingDivider = true;
        libWidthAtDragStart = libWidth;
        dragStartX = e.x;
    }

    if (nearH)
    {
        isDraggingHDivider = true;
        visualHeightAtDragStart = visualHeight;
        dragStartY = e.y;
    }


}

void DSKSFzEditor::mouseDrag(const juce::MouseEvent& e)
{
    if (!isDraggingDivider && !isDraggingHDivider) return;

    if (isDraggingDivider)
        libWidth = juce::jlimit(120, juce::jmax(120, juce::jmin(600, getWidth() - 590)), libWidthAtDragStart + (e.x - dragStartX));
    if (isDraggingHDivider)
        visualHeight = juce::jlimit(0, juce::jmax(0, getHeight() - DEF_H), visualHeightAtDragStart - (e.y - dragStartY));
    resized();
}

void DSKSFzEditor::mouseUp(const juce::MouseEvent&)
{
    if (!isDraggingDivider && !isDraggingHDivider) return;
    isDraggingDivider = false;
    isDraggingHDivider = false;
    if (auto* s = proc.getAppProperties().getUserSettings())
    { s->setValue("libWidth", libWidth); s->setValue("visualHeight", visualHeight); s->saveIfNeeded(); }
}

void DSKSFzEditor::setVoicesVisualHeight(int height)
{
    if (visualHeight > 0) expandedVisualHeight = visualHeight;
    visualHeight = juce::jlimit(0, 1200, height);
    // Make room if needed, but shrinking a visual never resizes the window.
    if (visualHeight > getHeight() - DEF_H)
    {
        if (auto* c = getConstrainer()) c->setSizeLimits(710, DEF_H, 4000, 4000);
        setSize(getWidth(), DEF_H + visualHeight);
        applyWindowConstraints();
    }
    else resized();
    if (auto* settings = proc.getAppProperties().getUserSettings())
    {
        settings->setValue("visualHeight", visualHeight);
        settings->setValue("expandedVisualHeight", expandedVisualHeight);
        settings->setValue("pluginHeight", getHeight());
        settings->saveIfNeeded();
    }
}

void DSKSFzEditor::toggleCompactVoicesArea()
{
    setVoicesVisualHeight(visualHeight > 0 ? 0 : expandedVisualHeight);
}

// ── Window constraints ────────────────────────────────────────────────────────
float DSKSFzEditor::maximumGuiScale() const
{
    const auto& displays = juce::Desktop::getInstance().getDisplays();
    const auto* display = displays.getDisplayForRect(getScreenBounds());
    if (display == nullptr) display = displays.getPrimaryDisplay();
    if (display == nullptr || getWidth() <= 0 || getHeight() <= 0) return 2.0f;
    const auto area = display->userArea.reduced(24);
    return juce::jlimit(0.5f, 2.0f, juce::jmin(
        (float) area.getWidth() / ((float) getWidth() * hostScale),
        (float) (area.getHeight() - 32) / ((float) getHeight() * hostScale)));
}

void DSKSFzEditor::setGuiScale(float scale)
{
    guiScale = juce::jlimit(0.5f, maximumGuiScale(), scale);
    // Keep the user's zoom separate from the DAW's display scaling.
    juce::AudioProcessorEditor::setScaleFactor(hostScale * guiScale);
    if (auto* settings = proc.getAppProperties().getUserSettings())
    {
        settings->setValue("guiScale", guiScale);
        settings->saveIfNeeded();
    }
}

void DSKSFzEditor::setScaleFactor(float newScale)
{
    hostScale = newScale > 0.0f ? newScale : 1.0f;
    juce::AudioProcessorEditor::setScaleFactor(hostScale * guiScale);
}

void DSKSFzEditor::applyWindowConstraints()
{
    if (auto* c = getConstrainer())
    {
        if (proc.wrapperType == juce::AudioProcessor::wrapperType_Standalone)
            c->setSizeLimits(710, DEF_H, 4000, 4000);
        else
            c->setSizeLimits(getWidth(), getHeight(), getWidth(), getHeight());
    }
}

// ── SFZ preview (click [S] badge) ────────────────────────────────────────────
void DSKSFzEditor::triggerPreviewNote()
{
    proc.keyboardState.noteOn(1, 60, 0.8f);
    previewCountdown = 10; // ~830 ms at 12 Hz
}

void DSKSFzEditor::loadInstrumentForPreview(const juce::File& f)
{
    statusLabel.setText("Loading...", juce::dontSendNotification);
    const int currentGen = ++loadGeneration;
    juce::Thread::launch([this, f, currentGen]()
        {
            if (currentGen != loadGeneration.load()) return; // Abortar carga si hay un clic más reciente
    proc.loadInstrument(f);
    juce::MessageManager::callAsync([this, currentGen]()
        {
            if (currentGen == loadGeneration.load())
            {
                statusLabel.setText("Ready", juce::dontSendNotification);
                triggerPreviewNote();
                onInstrumentLoaded();
            }
        });
        });
}

void DSKSFzEditor::onInstrumentLoaded()
{
    timerCallback();
}

// ── Instrument text editor (Options > Edit Instrument...) ───────────────────
// Only plain-text formats can be edited this way: SFZ files and Decent
// Sampler's XML .dspreset (including one extracted from a .dslibrary — the
// currently loaded file always points at a real file on disk, see
// DSKSFzProcessor::getCurrentSFZFile()). SF2 is a binary format and is
// excluded.
bool DSKSFzEditor::canEditCurrentInstrument() const
{
    const auto fmt = proc.getCurrentFormat();
    return proc.getCurrentSFZFile().existsAsFile()
        && (fmt == InstrumentFormat::SFZ || fmt == InstrumentFormat::DecentSampler);
}

void DSKSFzEditor::openInstrumentEditor()
{
    if (instrumentEditorWindow != nullptr)
    {
        instrumentEditorWindow->toFront(true);
        return;
    }

    if (!canEditCurrentInstrument())
        return;

    instrumentEditorFile = proc.getCurrentSFZFile();
    instrumentEditorDirty = false;

    instrumentEditorWindow = std::make_unique<InstrumentEditorWindow>(instrumentEditorFile);

    instrumentEditorWindow->onTextChanged = [this]()
    {
        if (!instrumentEditorDirty)
        {
            instrumentEditorDirty = true;
            if (instrumentEditorWindow != nullptr)
                instrumentEditorWindow->setDirtyIndicator(true);
        }
    };

    instrumentEditorWindow->onSaveClicked = [this]() { saveInstrumentEditorContents(); };

    instrumentEditorWindow->onCloseRequested = [this](std::function<void(bool)> onResolved)
    {
        requestCloseInstrumentEditor(std::move(onResolved));
    };
}

bool DSKSFzEditor::saveInstrumentEditorContents()
{
    if (instrumentEditorWindow == nullptr)
        return false;

    if (!instrumentEditorFile.replaceWithText(instrumentEditorWindow->getEditorText()))
    {
        instrumentEditorWindow->setStatusText("Save failed — check file permissions.");
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
            "Save Failed",
            "Could not write to \"" + instrumentEditorFile.getFullPathName() + "\".");
        return false;
    }

    instrumentEditorDirty = false;
    instrumentEditorWindow->setDirtyIndicator(false);
    instrumentEditorWindow->setStatusText("Saved. Reloading instrument...");

    // Reload the instrument so the edits take effect immediately, the same
    // way any other file load happens (background thread, guarded by
    // loadGeneration so a newer load always wins).
    statusLabel.setText("Loading...", juce::dontSendNotification);
    const int currentGen = ++loadGeneration;
    auto fileToReload = instrumentEditorFile;
    juce::Thread::launch([this, fileToReload, currentGen]()
        {
            if (currentGen != loadGeneration.load()) return;
    proc.loadInstrument(fileToReload);
    juce::MessageManager::callAsync([this, currentGen]()
        {
            if (currentGen == loadGeneration.load())
            {
                statusLabel.setText("Ready", juce::dontSendNotification);
                if (instrumentEditorWindow != nullptr)
                    instrumentEditorWindow->setStatusText("Saved and reloaded.");
                onInstrumentLoaded();
            }
        });
        });

    return true;
}

void DSKSFzEditor::requestCloseInstrumentEditor(std::function<void(bool)> onResolved)
{
    if (instrumentEditorWindow == nullptr)
    {
        if (onResolved) onResolved(true);
        return;
    }

    if (!instrumentEditorDirty)
    {
        instrumentEditorWindow = nullptr;
        if (onResolved) onResolved(true);
        return;
    }

    auto* box = new juce::AlertWindow("Unsaved Changes",
        "The instrument file \"" + instrumentEditorFile.getFileName() + "\" has unsaved changes.\n"
        "Do you want to save them before closing?",
        juce::AlertWindow::WarningIcon);
    box->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
    box->addButton("Discard", 2);
    box->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    box->enterModalState(true, juce::ModalCallbackFunction::create([this, onResolved](int result)
        {
            if (result == 0) // Cancel: keep the editor window open
            {
                if (onResolved) onResolved(false);
                return;
            }

            if (result == 1) // Save
            {
                if (!saveInstrumentEditorContents())
                {
                    // Write failed — keep the window open so nothing is lost.
                    if (onResolved) onResolved(false);
                    return;
                }
            }

            instrumentEditorDirty = false;
            instrumentEditorWindow = nullptr;
            if (onResolved) onResolved(true);
        }), true);
}
