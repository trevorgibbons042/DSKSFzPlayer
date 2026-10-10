#pragma once
#include <JuceHeader.h>

inline void writeGifFixture(const juce::File& file)
{
    juce::MemoryOutputStream out;
    out.write("GIF89a", 6); out.writeShort(2); out.writeShort(1);
    out.writeByte((char) 0x81); out.writeByte(3); out.writeByte(0);
    const unsigned char palette[] = { 255,0,0, 0,255,0, 0,0,255, 0,0,0 };
    out.write(palette, sizeof(palette));
    const auto frame = [&](int left, int width, int colour, int disposal, int delay, bool transparent)
    {
        out.writeByte(0x21); out.writeByte((char) 0xf9); out.writeByte(4);
        out.writeByte((char) (disposal * 4 + (transparent ? 1 : 0)));
        out.writeShort((short) delay); out.writeByte(3); out.writeByte(0);
        out.writeByte(0x2c); out.writeShort((short) left); out.writeShort(0);
        out.writeShort((short) width); out.writeShort(1); out.writeByte(0);
        out.writeByte(2);
        // Reset before each pixel so all codes stay three bits wide.
        unsigned int bits = 0; int count = 0;
        for (int i = 0; i < width; ++i) { bits |= 4u << count; count += 3; bits |= (unsigned int) colour << count; count += 3; }
        bits |= 5u << count; count += 3;
        const int bytes = (count + 7) / 8;
        out.writeByte((char) bytes);
        for (int i = 0; i < bytes; ++i) out.writeByte((char) (bits >> (i * 8)));
        out.writeByte(0);
    };
    frame(0, 2, 0, 1, 4, false); // red/red
    frame(1, 1, 1, 3, 7, false); // red/green, then restore red/red
    frame(0, 1, 2, 2, 3, false); // blue/red, then clear left to black
    frame(1, 1, 1, 1, 5, false); // black/green
    frame(1, 1, 3, 1, 5, true);  // transparent pixel preserves green
    out.writeByte(0x3b);
    file.replaceWithData(out.getData(), out.getDataSize());
}
