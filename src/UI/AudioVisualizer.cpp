#include "AudioVisualizer.h"
#include <algorithm>
#include <cmath>

namespace
{
    constexpr int   kNumStars     = 90;
    constexpr int   kNumBubbles   = 34;
    constexpr int   kNumBands     = 28;
    constexpr int   kNumParticles = 46;
    constexpr int   kNumOrbitDots = 24; // 3 anillos x 8
    constexpr int   kWaveHistLen  = 72;
    constexpr int   kMaxPulses    = 8;
    constexpr float kIdleBase     = 0.16f; // energía mínima en reposo (standby suave)
    constexpr float kTimerHz      = 30.0f;

    constexpr char kMatrixCharset[] = "01$#%&@+-/*<>=ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    constexpr int  kMatrixCharsetLen = (int)(sizeof(kMatrixCharset) - 1);
}

AudioVisualizer::AudioVisualizer()
{
    setInterceptsMouseClicks(false, true);

    customizeBtn.setTooltip("Customize the visualizer animation");
    customizeBtn.onClick = [this] { showStyleMenu(); };
    addChildComponent(eqDisplay);
    eqDisplay.setInterceptsMouseClicks(true, true);
    addAndMakeVisible(customizeBtn);

    ensureSeeded();
    startTimerHz((int)kTimerHz);
}

AudioVisualizer::~AudioVisualizer()
{
    stopTimer();
    ++*gifGeneration;
    gifPool.removeAllJobs(true, -1);
}

void AudioVisualizer::ensureSeeded()
{
    if (seeded)
        return;
    seeded = true;

    stars.resize((size_t)kNumStars);
    for (auto& s : stars)
    {
        s.ang       = rng.nextFloat() * juce::MathConstants<float>::twoPi;
        s.r         = rng.nextFloat();
        s.speed     = 0.15f + rng.nextFloat() * 0.35f;
        s.twinklePh = rng.nextFloat() * juce::MathConstants<float>::twoPi;
    }

    bubbles.resize((size_t)kNumBubbles);
    for (auto& b : bubbles)
    {
        b.x        = rng.nextFloat();
        b.y        = rng.nextFloat();
        b.radius   = 0.018f + rng.nextFloat() * 0.05f;
        b.speed    = 0.05f + rng.nextFloat() * 0.12f;
        b.wobblePh = rng.nextFloat() * juce::MathConstants<float>::twoPi;
    }

    bands.resize((size_t)kNumBands);
    for (size_t i = 0; i < bands.size(); ++i)
    {
        const float n = (float)i / (float)(bands.size() - 1);
        // curva en forma de campana para que los graves/agudos parezcan más bajos
        bands[i].weight = 0.35f + 0.65f * std::sin(n * juce::MathConstants<float>::pi);
        bands[i].value  = 0.0f;
    }

    particles.resize((size_t)kNumParticles);
    for (auto& p : particles)
    {
        p.x     = rng.nextFloat();
        p.y     = rng.nextFloat();
        p.vx    = (rng.nextFloat() - 0.5f) * 0.05f;
        p.vy    = (rng.nextFloat() - 0.5f) * 0.05f;
        p.huePh = rng.nextFloat() * juce::MathConstants<float>::twoPi;
    }

    orbitDots.resize((size_t)kNumOrbitDots);
    for (size_t i = 0; i < orbitDots.size(); ++i)
    {
        orbitDots[i].ring       = (float)(i % 3);
        orbitDots[i].baseAngle  = ((float)(i / 3) / 8.0f) * juce::MathConstants<float>::twoPi;
        orbitDots[i].speed      = 0.12f + 0.06f * orbitDots[i].ring;
    }

    waveHistory.assign((size_t)kWaveHistLen, 0.0f);
}

void AudioVisualizer::resized()
{
    auto r = getLocalBounds();
    customizeBtn.setBounds(r.getRight() - 76, 4, 72, juce::jmin(22, r.getHeight() - 4));
    eqDisplay.setBounds(r);
    rebuildMatrixColumns();
}

void AudioVisualizer::rebuildMatrixColumns()
{
    const int newCount = juce::jmax(1, (int)((float)getWidth() / matrixCharPxW));
    if ((int)matrixCols.size() == newCount)
        return;
    matrixCols.resize((size_t)newCount);
    for (int i = 0; i < newCount; ++i)
        resetMatrixColumn(i);
}

void AudioVisualizer::resetMatrixColumn(int index)
{
    auto& c = matrixCols[(size_t)index];
    c.headY = -rng.nextFloat() * 1.4f;
    c.speed = 0.18f + rng.nextFloat() * 0.34f;
    c.len   = 4 + rng.nextInt(6); // 4..9 caracteres de estela
    juce::String g;
    for (int i = 0; i < c.len; ++i)
        g += kMatrixCharset[rng.nextInt(kMatrixCharsetLen)];
    c.glyphs   = g;
    c.nextSwap = t + rng.nextDouble() * 0.3;
}

void AudioVisualizer::pushLevel(float levelL, float levelR)
{
    targetLevel = juce::jlimit(0.0f, 1.5f, juce::jmax(levelL, levelR));
}

void AudioVisualizer::setThemeColours(bool isDark, juce::Colour bg, juce::Colour accent, juce::Colour accent2, juce::Colour sectionBg)
{
    darkTheme = isDark;
    bgColour  = bg;
    accentA   = accent;
    accentB   = accent2;

    // Mismo fondo que usa el botón de carpeta del grabador, para que destaque más
    // que el fondo casi transparente que tenía antes.
    customizeBtn.setColour(juce::TextButton::buttonColourId, sectionBg);
    customizeBtn.setColour(juce::TextButton::buttonOnColourId, accent.withAlpha(0.35f));
    customizeBtn.setColour(juce::TextButton::textColourOffId, accent.withAlpha(0.85f));
    customizeBtn.setColour(juce::TextButton::textColourOnId, accent);
    repaint();
}

void AudioVisualizer::setStyle(int styleIndex)
{
    style = (Style)juce::jlimit(0, (int)Style::NumStyles - 1, styleIndex);
    eqDisplay.setVisible(style == Style::EQ);
    customizeBtn.toFront(false);
    repaint();
}

juce::String AudioVisualizer::getStyleName(int styleIndex)
{
    switch ((Style)styleIndex)
    {
        case Style::Starfield: return "Starfield";
        case Style::Bubbles:   return "Bubbles";
        case Style::Spectrum:  return "Spectrum Bars";
        case Style::Rings:     return "Pulse Rings";
        case Style::Plasma:    return "Plasma Glow";
        case Style::Particles: return "Fireflies";
        case Style::Orbit:     return "Orbit";
        case Style::Waveform:  return "Waveform";
        case Style::Matrix:    return "Matrix Rain";
        case Style::EQ:        return "EQ response";
        case Style::GIF:       return "GIF animation";
        default:                return "Style";
    }
}

void AudioVisualizer::restoreGifs(const juce::StringArray& paths, const juce::String& selected)
{
    gifPaths.clear();
    for (const auto& path : paths) if (juce::File(path).existsAsFile()) gifPaths.addIfNotAlreadyThere(path);
    if (juce::File(selected).existsAsFile()) loadGif(juce::File(selected), false);
}

void AudioVisualizer::loadGif(const juce::File& file, bool showErrors)
{
    const auto token = gifGeneration;
    const int generation = ++*token;
    juce::Component::SafePointer<AudioVisualizer> safeThis(this);
    gifPool.addJob([safeThis, token, generation, file, showErrors]
    {
        auto decoded = AnimatedGif::read(file, [token, generation] { return token->load() != generation; });
        if (token->load() != generation) return;
        juce::MessageManager::callAsync([safeThis, token, generation, file, showErrors, decoded = std::move(decoded)]() mutable
        {
            if (!safeThis || token->load() != generation) return;
            if (decoded.frames.empty())
            {
                if (showErrors) juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "GIF could not be loaded", decoded.error);
                return;
            }
            safeThis->gif = std::move(decoded);
            safeThis->gifFrame = 0; safeThis->gifElapsedMs = 0;
            safeThis->lastGifTick = juce::Time::getMillisecondCounterHiRes();
            safeThis->gifPath = file.getFullPathName();
            safeThis->gifPaths.addIfNotAlreadyThere(safeThis->gifPath);
            if (showErrors) safeThis->setStyle((int) Style::GIF);
            if (safeThis->onGifLibraryChanged) safeThis->onGifLibraryChanged();
            if (showErrors && safeThis->onStyleChanged) safeThis->onStyleChanged((int) Style::GIF);
        });
    });
}

void AudioVisualizer::chooseGif()
{
    gifChooser = std::make_unique<juce::FileChooser>("Add a GIF to Visuals", juce::File(gifPath), "*.gif");
    juce::Component::SafePointer<AudioVisualizer> safeThis(this);
    gifChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [safeThis](const juce::FileChooser& chooser)
        { if (safeThis && chooser.getResult().existsAsFile()) safeThis->loadGif(chooser.getResult()); });
}

void AudioVisualizer::showStyleMenu()
{
    juce::PopupMenu menu, gifs;
    for (int i = 0; i < (int)Style::NumStyles; ++i)
        menu.addItem(i + 1, getStyleName(i), i != (int) Style::GIF || !gif.frames.empty(), i == (int)style);
    menu.addSeparator();
    menu.addItem(1001, "Add GIF...");
    for (int i = 0; i < gifPaths.size(); ++i)
        gifs.addItem(2000 + i, juce::File(gifPaths[i]).getFileName(), true, gifPaths[i] == gifPath);
    if (gifPaths.size() > 0) menu.addSubMenu("My GIFs", gifs);
    menu.addItem(1002, "Remove current GIF", gifPath.isNotEmpty());
    juce::Component::SafePointer<AudioVisualizer> safeThis(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(customizeBtn),
        [safeThis](int result)
        {
            if (!safeThis || result <= 0) return;
            if (result == 1001) { safeThis->chooseGif(); return; }
            if (result >= 2000 && result < 2000 + safeThis->gifPaths.size())
            { safeThis->loadGif(juce::File(safeThis->gifPaths[result - 2000])); return; }
            if (result == 1002)
            {
                ++*safeThis->gifGeneration;
                safeThis->gifPaths.removeString(safeThis->gifPath);
                safeThis->gifPath.clear(); safeThis->gif.frames.clear();
                if (safeThis->onGifLibraryChanged) safeThis->onGifLibraryChanged();
                result = (int) (safeThis->style == Style::GIF ? Style::Spectrum : safeThis->style) + 1;
            }
            const int index = result - 1;
            safeThis->setStyle(index);
            if (safeThis->onStyleChanged) safeThis->onStyleChanged(index);
        });
}

void AudioVisualizer::timerCallback()
{
    const double now = juce::Time::getMillisecondCounterHiRes();
    if (style == Style::GIF && !gif.frames.empty())
    {
        gifElapsedMs += lastGifTick > 0 ? juce::jmin(1000.0, now - lastGifTick) : 0.0;
        while (gifElapsedMs >= gif.frames[gifFrame].durationMs)
        {
            gifElapsedMs -= gif.frames[gifFrame].durationMs;
            gifFrame = (gifFrame + 1) % gif.frames.size();
        }
    }
    lastGifTick = now;
    const double dt = 1.0 / kTimerHz;
    t += dt;

    // Ataque rápido, caída suave — para que reaccione con viveza pero sin parpadeos
    const float coeff = targetLevel > smoothedLevel ? 0.45f : 0.10f;
    smoothedLevel += (targetLevel - smoothedLevel) * coeff;

    const bool transient = (targetLevel - lastLevelForTransient) > 0.22f;
    lastLevelForTransient = targetLevel;

    const float energy = kIdleBase + smoothedLevel * (1.0f - kIdleBase);

    switch (style)
    {
        case Style::Starfield:
        {
            for (auto& s : stars)
            {
                s.r += (float)dt * s.speed * (0.35f + energy * 1.4f);
                if (s.r > 1.05f)
                {
                    s.r   = 0.0f;
                    s.ang = rng.nextFloat() * juce::MathConstants<float>::twoPi;
                    s.speed = 0.15f + rng.nextFloat() * 0.35f;
                }
            }
            break;
        }
        case Style::Bubbles:
        {
            for (auto& b : bubbles)
            {
                b.y -= (float)dt * b.speed * (0.3f + energy * 1.6f);
                if (b.y < -0.08f)
                {
                    b.y      = 1.05f;
                    b.x      = rng.nextFloat();
                    b.radius = 0.018f + rng.nextFloat() * 0.05f;
                    b.speed  = 0.05f + rng.nextFloat() * 0.12f;
                }
            }
            break;
        }
        case Style::Spectrum:
        {
            for (size_t i = 0; i < bands.size(); ++i)
            {
                const float jitter = 0.75f + 0.25f * std::sin((float)t * 2.3f + (float)i * 0.7f);
                const float target = smoothedLevel * bands[i].weight * jitter;
                const float rise   = target > bands[i].value ? 0.5f : 0.10f;
                bands[i].value += (target - bands[i].value) * rise;
            }
            break;
        }
        case Style::Rings:
        {
            static double lastIdleSpawn = 0.0;
            if (transient || (t - lastIdleSpawn) > 1.7)
            {
                lastIdleSpawn = t;
                if ((int)pulses.size() < kMaxPulses)
                    pulses.push_back({ 0.0f, 1.0f });
            }
            for (auto& p : pulses)
            {
                p.radius += (float)dt * (0.22f + smoothedLevel * 0.7f);
                p.alpha   = juce::jlimit(0.0f, 1.0f, 1.0f - p.radius);
            }
            pulses.erase(std::remove_if(pulses.begin(), pulses.end(),
                [](const Pulse& p) { return p.radius > 1.15f; }), pulses.end());
            break;
        }
        case Style::Particles:
        {
            for (auto& p : particles)
            {
                p.x += p.vx * (float)dt * (0.5f + energy * 1.6f);
                p.y += p.vy * (float)dt * (0.5f + energy * 1.6f);
                if (p.x < -0.05f) p.x = 1.05f;
                if (p.x >  1.05f) p.x = -0.05f;
                if (p.y < -0.05f) p.y = 1.05f;
                if (p.y >  1.05f) p.y = -0.05f;
            }
            break;
        }
        case Style::Waveform:
        {
            static double lastPush = 0.0;
            if (t - lastPush > 0.08) // ~12 muestras/seg, coincide con el meter del editor
            {
                lastPush = t;
                waveHistory.erase(waveHistory.begin());
                waveHistory.push_back(smoothedLevel);
            }
            break;
        }
        case Style::Matrix:
        {
            if (matrixCols.empty())
                rebuildMatrixColumns();

            const float h = (float)getHeight();
            for (size_t i = 0; i < matrixCols.size(); ++i)
            {
                auto& c = matrixCols[i];
                c.headY += (float)dt * c.speed * (0.4f + energy * 1.3f);

                const float lenNorm = h > 0.0f ? (float)c.len * matrixCharPxH / h : 0.3f;
                if (c.headY - lenNorm > 1.15f)
                {
                    resetMatrixColumn((int)i);
                }
                else if (t > c.nextSwap)
                {
                    c.nextSwap = t + 0.09 + rng.nextDouble() * 0.22;
                    if (c.glyphs.isNotEmpty())
                    {
                        const int idx = rng.nextInt(c.glyphs.length());
                        c.glyphs = c.glyphs.replaceSection(idx, 1,
                            juce::String::charToString(kMatrixCharset[rng.nextInt(kMatrixCharsetLen)]));
                    }
                }
            }
            break;
        }
        case Style::Plasma:
        case Style::Orbit:
        default:
            break;
    }

    repaint();
}

void AudioVisualizer::paint(juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    if (r.getWidth() < 4.0f || r.getHeight() < 4.0f)
        return;

    g.setColour(bgColour);
    g.fillRect(r);

    switch (style)
    {
        case Style::Starfield: drawStarfield(g, r); break;
        case Style::Bubbles:   drawBubbles(g, r);   break;
        case Style::Spectrum:  drawSpectrum(g, r);  break;
        case Style::Rings:     drawRings(g, r);     break;
        case Style::Plasma:    drawPlasma(g, r);    break;
        case Style::Particles: drawParticles(g, r); break;
        case Style::Orbit:     drawOrbit(g, r);      break;
        case Style::Waveform:  drawWaveform(g, r);  break;
        case Style::Matrix:    drawMatrix(g, r);    break;
        case Style::GIF:
            if (!gif.frames.empty())
                g.drawImageWithin(gif.frames[gifFrame].image, 0, 0, getWidth(), getHeight(), juce::RectanglePlacement::centred);
            else
            {
                g.setColour(accentA);
                g.drawText("Use Visuals to add a GIF", getLocalBounds(), juce::Justification::centred);
            }
            break;
        default: break;
    }
}

void AudioVisualizer::drawStarfield(juce::Graphics& g, juce::Rectangle<float> r)
{
    const auto cx = r.getCentreX();
    const auto cy = r.getCentreY();
    const float maxR = 0.5f * std::sqrt(r.getWidth() * r.getWidth() + r.getHeight() * r.getHeight()) * 0.55f;

    for (auto& s : stars)
    {
        const float px = cx + std::cos(s.ang) * s.r * maxR;
        const float py = cy + std::sin(s.ang) * s.r * maxR;
        const float twinkle = 0.75f + 0.25f * std::sin((float)t * 3.0f + s.twinklePh);
        const float alpha = juce::jlimit(0.0f, 1.0f, s.r * twinkle);
        const float size = 0.8f + s.r * 2.6f;

        g.setColour((s.speed > 0.32f ? accentB : accentA).withAlpha(alpha));
        g.fillEllipse(px - size * 0.5f, py - size * 0.5f, size, size);
    }
}

void AudioVisualizer::drawBubbles(juce::Graphics& g, juce::Rectangle<float> r)
{
    for (auto& b : bubbles)
    {
        const float wobble = std::sin((float)t * 1.3f + b.wobblePh) * 0.012f;
        const float px = r.getX() + (b.x + wobble) * r.getWidth();
        const float py = r.getY() + b.y * r.getHeight();
        const float rad = b.radius * r.getHeight();
        const float alpha = juce::jlimit(0.0f, 0.55f, 0.18f + smoothedLevel * 0.5f);

        g.setColour(accentA.withAlpha(alpha * 0.5f));
        g.fillEllipse(px - rad, py - rad, rad * 2.0f, rad * 2.0f);
        g.setColour(accentA.withAlpha(alpha));
        g.drawEllipse(px - rad, py - rad, rad * 2.0f, rad * 2.0f, 1.0f);
    }
}

void AudioVisualizer::drawSpectrum(juce::Graphics& g, juce::Rectangle<float> r)
{
    const int n = (int)bands.size();
    const float slot = r.getWidth() / (float)n;
    const float gap = juce::jmin(3.0f, slot * 0.25f);

    for (int i = 0; i < n; ++i)
    {
        const float v = juce::jlimit(0.0f, 1.0f, bands[(size_t)i].value);
        const float bh = juce::jmax(2.0f, v * r.getHeight() * 0.92f);
        const float bx = r.getX() + (float)i * slot + gap * 0.5f;
        const float bw = juce::jmax(1.0f, slot - gap);
        const float by = r.getBottom() - bh;

        g.setColour(accentA.interpolatedWith(accentB, v).withAlpha(0.85f));
        g.fillRoundedRectangle(bx, by, bw, bh, juce::jmin(2.0f, bw * 0.4f));
    }
}

void AudioVisualizer::drawRings(juce::Graphics& g, juce::Rectangle<float> r)
{
    const auto cx = r.getCentreX();
    const auto cy = r.getCentreY();
    const float maxR = 0.5f * juce::jmin(r.getWidth(), r.getHeight() * 2.4f);

    for (auto& p : pulses)
    {
        const float rad = p.radius * maxR;
        g.setColour(accentA.withAlpha(p.alpha * 0.7f));
        g.drawEllipse(cx - rad, cy - rad, rad * 2.0f, rad * 2.0f, 1.6f);
    }

    const float coreR = (3.0f + smoothedLevel * 5.0f);
    g.setColour(accentB.withAlpha(0.5f + smoothedLevel * 0.4f));
    g.fillEllipse(cx - coreR, cy - coreR, coreR * 2.0f, coreR * 2.0f);
}

void AudioVisualizer::drawPlasma(juce::Graphics& g, juce::Rectangle<float> r)
{
    const float energy = kIdleBase + smoothedLevel * (1.0f - kIdleBase);
    const float baseRad = juce::jmin(r.getWidth(), r.getHeight() * 3.0f) * (0.28f + energy * 0.22f);

    struct Blob { float speedX, speedY, phase; juce::Colour col; };
    const Blob blobs[3] = {
        { 0.55f, 0.42f, 0.0f,                              accentA },
        { 0.38f, 0.61f, juce::MathConstants<float>::pi * 0.66f, accentB },
        { 0.47f, 0.33f, juce::MathConstants<float>::pi * 1.33f, accentA.interpolatedWith(accentB, 0.5f) },
    };

    for (auto& b : blobs)
    {
        const float ox = std::cos((float)t * b.speedX + b.phase) * r.getWidth() * 0.22f;
        const float oy = std::sin((float)t * b.speedY + b.phase) * r.getHeight() * 0.28f;
        const float cx = r.getCentreX() + ox;
        const float cy = r.getCentreY() + oy;

        juce::ColourGradient grad(b.col.withAlpha(0.30f + energy * 0.25f), cx, cy,
                                   b.col.withAlpha(0.0f), cx + baseRad, cy, true);
        g.setGradientFill(grad);
        g.fillEllipse(cx - baseRad, cy - baseRad, baseRad * 2.0f, baseRad * 2.0f);
    }
}

void AudioVisualizer::drawParticles(juce::Graphics& g, juce::Rectangle<float> r)
{
    const float energy = kIdleBase + smoothedLevel * (1.0f - kIdleBase);

    for (auto& p : particles)
    {
        const float px = r.getX() + p.x * r.getWidth();
        const float py = r.getY() + p.y * r.getHeight();
        const float twinkle = 0.5f + 0.5f * std::sin((float)t * 2.2f + p.huePh);
        const float alpha = juce::jlimit(0.0f, 1.0f, (0.15f + energy * 0.6f) * twinkle);
        const float size = 2.0f + energy * 3.0f;
        const auto col = accentA.interpolatedWith(accentB, 0.5f + 0.5f * std::sin(p.huePh));

        g.setColour(col.withAlpha(alpha * 0.35f));
        g.fillEllipse(px - size, py - size, size * 2.0f, size * 2.0f);
        g.setColour(col.withAlpha(alpha));
        g.fillEllipse(px - size * 0.4f, py - size * 0.4f, size * 0.8f, size * 0.8f);
    }
}

void AudioVisualizer::drawOrbit(juce::Graphics& g, juce::Rectangle<float> r)
{
    const auto cx = r.getCentreX();
    const auto cy = r.getCentreY();
    const float maxR = 0.5f * juce::jmin(r.getWidth(), r.getHeight() * 2.4f);
    const float ringR[3] = { maxR * 0.35f, maxR * 0.62f, maxR * 0.9f };

    for (int ring = 0; ring < 3; ++ring)
    {
        g.setColour((ring % 2 == 0 ? accentA : accentB).withAlpha(0.12f));
        g.drawEllipse(cx - ringR[ring], cy - ringR[ring], ringR[ring] * 2.0f, ringR[ring] * 2.0f, 1.0f);
    }

    for (auto& d : orbitDots)
    {
        const float pulse = 1.0f + 0.10f * smoothedLevel * std::sin((float)t * 4.0f + d.baseAngle);
        const float rad = ringR[(int)d.ring] * pulse;
        const float ang = d.baseAngle + (float)t * d.speed * (0.4f + smoothedLevel * 1.4f);
        const float px = cx + std::cos(ang) * rad;
        const float py = cy + std::sin(ang) * rad;
        const float size = 2.5f + smoothedLevel * 2.5f;

        g.setColour(((int)d.ring % 2 == 0 ? accentA : accentB).withAlpha(0.55f + smoothedLevel * 0.35f));
        g.fillEllipse(px - size, py - size, size * 2.0f, size * 2.0f);
    }
}

void AudioVisualizer::drawWaveform(juce::Graphics& g, juce::Rectangle<float> r)
{
    const int n = (int)waveHistory.size();
    if (n < 2)
        return;

    const float midY = r.getCentreY();
    const float ampScale = r.getHeight() * 0.46f;

    juce::Path ribbon;
    ribbon.startNewSubPath(r.getX(), midY);
    for (int i = 0; i < n; ++i)
    {
        const float x = r.getX() + r.getWidth() * (float)i / (float)(n - 1);
        const float idleRipple = 0.03f * std::sin((float)t * 1.6f + (float)i * 0.35f);
        const float amp = juce::jlimit(0.0f, 1.2f, waveHistory[(size_t)i] + idleRipple);
        ribbon.lineTo(x, midY - amp * ampScale);
    }
    for (int i = n - 1; i >= 0; --i)
    {
        const float x = r.getX() + r.getWidth() * (float)i / (float)(n - 1);
        const float idleRipple = 0.03f * std::sin((float)t * 1.6f + (float)i * 0.35f);
        const float amp = juce::jlimit(0.0f, 1.2f, waveHistory[(size_t)i] + idleRipple);
        ribbon.lineTo(x, midY + amp * ampScale);
    }
    ribbon.closeSubPath();

    g.setColour(accentA.withAlpha(0.22f));
    g.fillPath(ribbon);

    juce::Path line;
    for (int i = 0; i < n; ++i)
    {
        const float x = r.getX() + r.getWidth() * (float)i / (float)(n - 1);
        const float idleRipple = 0.03f * std::sin((float)t * 1.6f + (float)i * 0.35f);
        const float amp = juce::jlimit(0.0f, 1.2f, waveHistory[(size_t)i] + idleRipple);
        const float y = midY - amp * ampScale;
        if (i == 0) line.startNewSubPath(x, y); else line.lineTo(x, y);
    }
    g.setColour(accentA);
    g.strokePath(line, juce::PathStrokeType(1.6f));
}

void AudioVisualizer::drawMatrix(juce::Graphics& g, juce::Rectangle<float> r)
{
    if (matrixCols.empty() || r.getHeight() < 4.0f)
        return;

    g.setFont(matrixFont);
    const float rowPx = matrixCharPxH;
    const auto headCol = darkTheme ? accentA.interpolatedWith(juce::Colours::white, 0.55f)
                                    : accentA.darker(0.35f);

    for (size_t ci = 0; ci < matrixCols.size(); ++ci)
    {
        auto& c = matrixCols[ci];
        const float cx = r.getX() + (float)ci * matrixCharPxW + matrixCharPxW * 0.5f;

        for (int j = 0; j < c.len && j < c.glyphs.length(); ++j)
        {
            const float charYNorm = c.headY - (float)j * (rowPx / r.getHeight());
            const float py = r.getY() + charYNorm * r.getHeight();
            if (py < r.getY() - rowPx || py > r.getBottom() + rowPx)
                continue;

            const float fade = 1.0f - (float)j / (float)c.len;
            g.setColour(j == 0 ? headCol.withAlpha(juce::jmax(0.0f, fade))
                                : accentA.withAlpha(fade * fade * 0.85f));
            g.drawText(c.glyphs.substring(j, j + 1),
                       juce::Rectangle<float>(cx - matrixCharPxW * 0.5f, py - rowPx * 0.5f, matrixCharPxW, rowPx),
                       juce::Justification::centred, false);
        }
    }
}
