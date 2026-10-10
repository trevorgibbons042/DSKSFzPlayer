#pragma once
#include <JuceHeader.h>

// JUCE decodes one GIF image. Split the animation into valid single-image GIFs,
// then composite partial frames according to their transparency/disposal flags.
struct AnimatedGif
{
    struct Frame { juce::Image image; int durationMs = 100; };
    std::vector<Frame> frames;
    juce::String error;

    static AnimatedGif read(const juce::File& file, const std::function<bool()>& cancelled = [] { return false; })
    {
        AnimatedGif result;
        const auto fail = [&](const char* message) { result.frames.clear(); result.error = message; return result; };
        if (file.getSize() <= 0 || file.getSize() > 32 * 1024 * 1024) return fail("Choose a GIF smaller than 32 MB.");
        juce::MemoryBlock data;
        if (!file.loadFileAsData(data) || data.getSize() < 13) return fail("The GIF could not be read.");
        const auto* bytes = static_cast<const unsigned char*>(data.getData());
        const auto size = data.getSize();
        if (std::memcmp(bytes, "GIF87a", 6) != 0 && std::memcmp(bytes, "GIF89a", 6) != 0) return fail("This file is not a GIF.");
        const auto word = [&](size_t index) { return (int) bytes[index] | ((int) bytes[index + 1] << 8); };
        const int width = word(6), height = word(8);
        if (width < 1 || height < 1 || width > 2048 || height > 2048) return fail("Choose a GIF up to 2048 pixels wide and tall.");
        const size_t paletteSize = (bytes[10] & 128) ? (size_t) (3 * (1 << ((bytes[10] & 7) + 1))) : 0;
        size_t pos = 13 + paletteSize;
        if (pos > size) return fail("The GIF colour table is incomplete.");
        const auto blocks = [&]() -> bool
        {
            while (pos < size)
            {
                const size_t count = bytes[pos++];
                if (count == 0) return true;
                if (count > size - pos) return false;
                pos += count;
            }
            return false;
        };
        int delay = 100, disposal = 0, transparentIndex = 0;
        bool transparent = false, first = true;
        juce::Image canvas(juce::Image::ARGB, width, height, true);
        const double scale = juce::jmin(1.0, 800.0 / juce::jmax(width, height));
        const int outputWidth = juce::jmax(1, (int) (width * scale));
        const int outputHeight = juce::jmax(1, (int) (height * scale));
        while (pos < size)
        {
            if (cancelled()) return fail("GIF loading cancelled.");
            const auto marker = bytes[pos++];
            if (marker == 0x3b) break;
            if (marker == 0x21)
            {
                if (pos >= size) return fail("The GIF extension is incomplete.");
                const auto label = bytes[pos++];
                if (label == 0xf9)
                {
                    if (size - pos < 6 || bytes[pos] != 4 || bytes[pos + 5] != 0) return fail("The GIF frame control is incomplete.");
                    disposal = (bytes[pos + 1] >> 2) & 7;
                    transparent = (bytes[pos + 1] & 1) != 0;
                    delay = juce::jmax(20, word(pos + 2) * 10);
                    transparentIndex = bytes[pos + 4];
                    pos += 6;
                }
                else if (!blocks()) return fail("The GIF extension is incomplete.");
                continue;
            }
            if (marker != 0x2c || size - pos < 9) return fail("The GIF frame is incomplete.");
            const int left = word(pos), top = word(pos + 2), fw = word(pos + 4), fh = word(pos + 6);
            const auto packed = bytes[pos + 8];
            pos += 9;
            if (fw < 1 || fh < 1 || left + fw > width || top + fh > height) return fail("The GIF frame is outside its canvas.");
            const size_t localSize = (packed & 128) ? (size_t) (3 * (1 << ((packed & 7) + 1))) : 0;
            const auto imageStart = pos;
            if (size - pos < localSize + 1) return fail("The GIF frame data is incomplete.");
            pos += localSize + 1; // Colour table and LZW minimum code size.
            if (!blocks()) return fail("The GIF frame data is incomplete.");
            juce::MemoryOutputStream frameData;
            frameData.write("GIF89a", 6);
            frameData.writeShort((short) fw); frameData.writeShort((short) fh);
            frameData.writeByte((char) bytes[10]); frameData.writeByte((char) bytes[11]); frameData.writeByte((char) bytes[12]);
            frameData.write(bytes + 13, paletteSize);
            frameData.writeByte(0x21); frameData.writeByte((char) 0xf9); frameData.writeByte(4);
            frameData.writeByte(transparent ? 1 : 0); frameData.writeShort(0);
            frameData.writeByte((char) transparentIndex); frameData.writeByte(0);
            frameData.writeByte(0x2c); frameData.writeShort(0); frameData.writeShort(0);
            frameData.writeShort((short) fw); frameData.writeShort((short) fh); frameData.writeByte((char) packed);
            frameData.write(bytes + imageStart, pos - imageStart); frameData.writeByte(0x3b);
            juce::MemoryInputStream stream(frameData.getData(), frameData.getDataSize(), false);
            const auto image = juce::GIFImageFormat().decodeImage(stream);
            if (!image.isValid()) return fail("The GIF image data could not be decoded.");
            juce::Colour background = juce::Colours::transparentBlack;
            if (!transparent && paletteSize > (size_t) bytes[11] * 3 + 2)
            {
                const auto index = 13 + (size_t) bytes[11] * 3;
                background = juce::Colour(bytes[index], bytes[index + 1], bytes[index + 2]);
            }
            if (first) { canvas.clear(canvas.getBounds(), background); first = false; }
            const auto previous = disposal == 3 ? canvas.createCopy() : juce::Image();
            { juce::Graphics g(canvas); g.drawImageAt(image, left, top); }
            if (result.frames.size() >= 512 || (result.frames.size() + 1) * (size_t) outputWidth * (size_t) outputHeight * 4 > 128 * 1024 * 1024)
                return fail("Choose a shorter or smaller GIF (up to 512 frames / 128 MB decoded).");
            result.frames.push_back({ canvas.rescaled(outputWidth, outputHeight).createCopy(), delay });
            if (disposal == 2) canvas.clear({ left, top, fw, fh }, background);
            else if (disposal == 3) canvas = previous;
            delay = 100; disposal = 0; transparent = false; transparentIndex = 0;
        }
        if (result.frames.empty()) return fail("The GIF contains no frames.");
        return result;
    }
};
