# CHANGES
Overhaul instrument browsing, ratings, effects, EQ, and interface controls

Library and instrument browser:
- Combine SoundFonts and instruments into one Instruments browser.
- Search across all SoundFont presets using any instrument name, including custom names, while preserving folder and bank grouping.
- Remove file paths from search results and match search/filter styling to the browser.
- Add custom names for SoundFonts and individual instruments.
- Replace favorites with independent 1–5 star ratings for instruments, SoundFonts, and folders.
- Use consistent drawn stars throughout the interface, showing only filled stars beside rated browser items.
- Add instrument categories and combined category, rating, and text filters.
- Add custom SoundFont folders, folder icons, renaming, and bank organization without moving source files.
- Sort by highest rating first, then folders before SoundFonts, then alphabetically.
- Add hiding and restoring instruments through a collapsed Hidden instruments section at the bottom.
- Add Collapse all.
- Make Up/Down and Left/Right load the previous or next instrument while skipping closed groups and hidden instruments.
- Remember the last selected preset for each bank.
- Save and export/import custom names, ratings, categories, hidden states, and folder organization.
- Simplify context menus: keep Star rating only for folders and Move to folder only for SoundFont banks.

Interface:
- Separate the SoundFont name and instrument name in the header, each with independent clickable ratings and click-to-rename.
- Remove the old header instrument selector.
- Keep the category button focused on category selection.
- Merge standalone audio/MIDI and plugin-state options into the GUI’s Options menu.
- Add GUI scaling and improve window resizing so the right-side sections fill the available width.
- Fix browser-divider resizing so it does not resize or disrupt the whole window.
- Make the bottom visual panel independently resizable and move Voices with it.
- Remove the redundant Visual size button.
- Add right-click numeric entry for knobs and Voices.
- Limit displayed numbers to two decimal places without reducing parameter precision.
- Enlarge the Voices value field.
- Add a dark piano keyboard appearance for dark themes.
- Remove the promotional banner and its remote-loading integration.

Effects and performance:
- Fix chorus, delay, and reverb behavior.
- Add phaser and flanger effects with rate, depth, mix, and bypass controls.
- Enable effects when their controls are adjusted.
- Fix SFZ voice allocation and SoundFont voice-limit reductions so setting Voices to 1 is enforced.
- Add adjustable note glide from 0–2000 ms, with 0 disabling glide.
- Add optional MIDI modulation-wheel control of LFO 1 depth, LFO 2 depth, or both.
- Use the Depth knobs as the maximum modulation-wheel depth.
- Append new parameters to preserve existing host parameter ordering.

Visuals and EQ:
- Add animated GIF visuals, file selection, drag-and-drop, saved GIF selection, and GIF-library management.
- Move EQ into the bottom Visuals selector.
- Add a live audio spectrum alongside the actual EQ response curve.
- Add adjustable Low, Mid, and High frequencies and widths.
- Support dragging EQ handles for frequency/gain, scrolling for width, and right-clicking for exact values.
- Enforce Low < Mid < High during editing, automation, and state restoration.
- Keep the displayed EQ response synchronized with the audio filters.
- Hide EQ handles, labels, and mouse interactions while EQ is bypassed.
- Save and automate glide, modulation-wheel routing, and EQ settings.

Validation and documentation:
- Update the README, manual, and build/test configuration.
- Add regression coverage for effects, library search, browser navigation and organization, editor controls, animated GIFs, glide, MIDI modulation, voice limits, and EQ response.


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
