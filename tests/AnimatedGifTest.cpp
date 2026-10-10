#include "UI/AnimatedGif.h"
#include "UI/AudioVisualizer.h"
#include "GifFixture.h"
#include <iostream>
#include <stdexcept>
static void require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
int main()
{
    juce::ScopedJuceInitialiser_GUI gui;
    const auto root = juce::File::getCurrentWorkingDirectory().getNonexistentChildFile("gif-fixtures", "", false);
    struct Cleanup { juce::File folder; ~Cleanup() { folder.deleteRecursively(); } } cleanup { root };
    root.createDirectory();
    const auto file = root.getChildFile("Animation.gif");
    writeGifFixture(file);
    const auto gif = AnimatedGif::read(file);
    require(gif.error.isEmpty() && gif.frames.size() == 5, "Must decode every GIF frame");
    require(gif.frames[0].durationMs == 40 && gif.frames[1].durationMs == 70, "GIF frame timings must be retained");
    require(gif.frames[0].image.getPixelAt(0,0) == juce::Colours::red && gif.frames[0].image.getPixelAt(1,0) == juce::Colours::red, "First frame incorrect");
    require(gif.frames[1].image.getPixelAt(0,0) == juce::Colours::red && gif.frames[1].image.getPixelAt(1,0) == juce::Colours::lime, "Partial frame must composite over previous frame");
    require(gif.frames[2].image.getPixelAt(0,0) == juce::Colours::blue && gif.frames[2].image.getPixelAt(1,0) == juce::Colours::red, "Restore-previous disposal is broken");
    require(gif.frames[3].image.getPixelAt(0,0) == juce::Colours::black && gif.frames[3].image.getPixelAt(1,0) == juce::Colours::lime, "Restore-background disposal is broken");
    require(gif.frames[4].image.getPixelAt(1,0) == juce::Colours::lime, "Transparency must preserve previous pixels");
    const auto broken = root.getChildFile("Broken.gif"); broken.replaceWithText("GIF89a");
    require(!AnimatedGif::read(broken).error.isEmpty(), "Malformed GIF must fail cleanly");
    require(AnimatedGif::read(file, [] { return true; }).frames.empty(), "GIF cancellation ignored");
    AudioVisualizer visual;
    visual.setSize(300, 90);
    visual.loadGif(file, false);
    for (int i = 0; i < 100 && visual.getGifFrameCount() == 0; ++i)
        juce::MessageManager::getInstance()->runDispatchLoopUntil(20);
    require(visual.getGifFrameCount() == 5 && visual.getGifPaths().size() == 1, "Background GIF loading or library registration failed");
    visual.setStyle((int) AudioVisualizer::Style::GIF);
    bool advanced = false;
    for (int i = 0; i < 20 && !advanced; ++i)
    {
        juce::MessageManager::getInstance()->runDispatchLoopUntil(20);
        advanced = visual.getGifFrameIndex() != 0;
    }
    require(advanced, "GIF playback must advance, not display a static first frame");
    AudioVisualizer restored;
    restored.setStyle((int) AudioVisualizer::Style::EQ);
    restored.restoreGifs(visual.getGifPaths(), visual.getGifPath());
    for (int i = 0; i < 100 && restored.getGifFrameCount() == 0; ++i)
        juce::MessageManager::getInstance()->runDispatchLoopUntil(20);
    require(restored.getGifFrameCount() == 5 && restored.getStyle() == (int) AudioVisualizer::Style::EQ,
            "Restoring GIFs must preserve the selected visual style");
    std::cout << "GIF frames, timing, partial updates, disposal, transparency, cancellation, playback and restoration passed.\n";
}
