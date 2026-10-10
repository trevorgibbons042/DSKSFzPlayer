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
