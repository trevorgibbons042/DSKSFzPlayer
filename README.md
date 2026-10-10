# DSK SFz Player

![Version 1.0](https://img.shields.io/badge/version-1.0-blue)
![Platform](https://img.shields.io/badge/platform-VST3%20%7C%20Standalone-green)

**DSK SFz Player** is a high-performance, 64-voice polyphonic SFZ/SF2/Decent Sampler sampler. Designed for music producers and sound designers, it provides a seamless workflow for loading SFZ, SoundFont2 (SF2), and Decent Sampler (.dspreset/.dslibrary) instrument libraries with full MIDI control, flexible modulation, and professional-grade effects.


<div align="center">
  <img src="screenshots/screenshot01.png" alt="DSK SFz Player Interface" width="600"/>
</div>

---

## 🚀 Key Features

* **Polyphony:** 64-voice polyphonic engine for rich, layered soundscapes.
* **Versatile Formats:** Works as a **VST3 Plugin** or as a **Standalone Application**.
* **Triple Sampler Engine:** Loads **SFZ**, **SF2 (SoundFont2)**, and **Decent Sampler** (`.dspreset`/`.dslibrary`) libraries in a single Instruments browser, with drag & drop support.
* **Audio Support:** Compatible with WAV, FLAC, and OGG audio formats inside SFZ/Decent Sampler files.
* **Drag & Drop Workflow:** Effortlessly load `.sfz`, `.sf2`, `.dspreset`, `.dslibrary` files, folders, or `.zip` packs directly into the plugin.
* **Instrument Browser:** Loaded SF2 and Decent Sampler banks show an expandable instrument list on the left. Use the up/down buttons to step through instruments; each bank remembers its last selected preset across reloads. Scrolling the list does not change the selected sound.
* **Custom SoundFont Names:** Right-click a SoundFont in the library and choose **Rename SoundFont**, or right-click its bank heading, instrument, or search result to rename it. Click the loaded instrument name at the top to rename it directly. Names are saved in player settings, included in configuration export/import, and used by search. Leave the name blank to restore the original. The `.sf2` file stays unchanged.
* **Instrument Navigation:** Press Up/Down in the Instruments list to load the previous/next instrument. The same keys work while typing in the library search. Closed groups are skipped; arrows never open them. Use **Collapse all** to close every SoundFont group at once. Search and filters follow the Instruments panel theme.
* **SoundFont Folders:** Click **F+** to create a folder, then right-click a SoundFont heading and choose **Move SoundFont to folder**. Folder organization is saved and included in config export/import. It does not move the files on disk.
* **Library Search:** Search instrument names across every SoundFont in your added library folders. For example, search `bass` to find matching presets inside all `.sf2` banks; click an instrument to load it. Search filters the same Instruments tree, keeping presets under their SoundFont heading without displaying file paths. Clearing search restores all instruments. Searches run in the background and also match SoundFont names.
* **SoundFont Presets:** Browse and load presets inside each SoundFont group in Instruments. Clicking the name in the header opens Rename.
* **Sound Sculpting:**
    * Amplitude & Filter envelopes, two versatile LFOs — available for both SFZ and SF2, replacing each preset's own envelope/filter with the plugin's knobs.
    * Seven built-in effects (drive, chorus, phaser, flanger, delay, reverb, and EQ), available for both SFZ and SF2.
* **Automation:** Real-time parameter automation for dynamic performances.
* **Intuitive Library Management:** Effortlessly organize, browse, and manage your growing collection of instrument libraries.
* **Ratings & Categories:** Click 1–5 stars beside the loaded instrument; click the same rating again to clear it. Use **Category** to choose Drums, Keys, Lead Synth, and other types, or create a custom category. Filter the library by category and minimum stars, with or without a search term. SoundFont presets are rated individually. Ratings and categories save automatically and are included in configuration export/import.
* **Compact Voices Area:** Use **Visual** beside the Voices/recorder controls to choose Tiny, Small, Medium, Large, or Hide. Drag the grip above the visual panel for a custom height, or double-click it to toggle compact mode.
* **Visuals:** Choose **EQ response** or an animation from **Visuals** above the keys. Use **Add GIF...** or drag a GIF onto the player to load it; **My GIFs** switches between your saved GIF choices. The EQ graph follows the knobs, automation and bypass. Visual height stays fixed when resizing the window.
* **Customizable Interface:** Use **Options → GUI Scale** to scale the entire interface from 50% to 200% (subject to screen space). The player remembers the setting. Multiple themes, window resizing, and tooltips are also available.


<div align="center">
  <img src="screenshots/dsk_sfz_player_themes.png" alt="DSK SFz Player themes" width="600"/>
</div>

---

## 🎹 Quick Start

### Loading Instruments
Getting started with DSK SFz Player is intuitive:

1.  **Drag & Drop:** Simply drag an `.sfz`, `.sf2`, `.dspreset`, or `.dslibrary` file, a folder containing instruments, or a `.zip` library pack onto the plugin window.
2.  **Open Button:** Use the "Open SFZ/SF2/DS/ZIP" button to browse your local files (DS = Decent Sampler).
3.  **Library Tree:** Organize your collections using the built-in Library panel. Double-click any entry to load it immediately.
4.  **SF2 Presets:** If an SF2 file has more than one bank/preset, its instruments appear in the left browser — click the instrument name in the header to reopen it anytime.

### Playability
* **MIDI Ready:** Connect your favorite MIDI controller or use your PC keyboard to trigger notes.
* **Parameter Control:** Right-click any knob to type a precise numeric value. Press Enter or click Apply to set it; press Escape to cancel. The popup shows the allowed range.
* **Global Config:** Export and import your library paths, favorites, and theme settings as `.json` files.

---

## 🛠 Tech Stack
Built with C++ and the [JUCE Framework](https://juce.com/).

---

## 📦 Installation & System Requirements
* **Formats:** VST3, Standalone.
* **Operating Systems:** macOS, Windows, Linux.
* *Check the latest releases for binary downloads.*

---

## 📄 Documentation
For a detailed guide on all features, MIDI mapping, and configuration, please refer to the `manual.md` file included in this repository.

---

## Downloads
[Click here to download the latest version](https://github.com/dskmusic/DSKSFzPlayer/releases/latest)
---


## 🔗 Links
* **Official Website:** [www.dskmusic.com](http://www.dskmusic.com)

---
*Developed by DSK Music.*

*Made with ❤️ in Gran Canaria, Spain.*
