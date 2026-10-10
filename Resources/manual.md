# DSK SFz Player — User Manual / Manual de Usuario

Version 1.0 · DSK Music · www.dskmusic.com

---

# ENGLISH

## Overview

DSK SFz Player is a 64-voice polyphonic SFZ/SF2/Decent Sampler sampler available as a VST3 plugin
and as a standalone application. Load any SFZ, SF2 (SoundFont2) or Decent Sampler (.dspreset /
.dslibrary) instrument and play it with full MIDI control, seven effects, and real-time parameter
automation.

Supported audio formats inside SFZ/Decent Sampler libraries: WAV, FLAC, OGG. SF2 (SoundFont2)
files are also supported directly. Note: for SF2 instruments, the Amp ADSR and Filter/Mod (LFO)
sections replace the soundfont's own built-in envelope and filter with the plugin's knobs (same
behaviour as SFZ). The Amp ADSR is applied per note; Filter and Mod are applied to the mixed
output rather than per voice, since the SF2 engine renders all voices already mixed together.
Decent Sampler (.dspreset/.dslibrary) instruments are parsed into the same engine SFZ uses, so
every plugin control (ADSR, filter, LFOs, effects, round robin, crossfades) works exactly as with
SFZ; the preset's own `<effects>` and `<ui>` blocks are not imported, since the plugin always uses
its own fixed UI and effects chain regardless of format (same approach as with SF2).

---

## Loading an Instrument

### A  Drag & Drop (onto the plugin window)

| What you drop                        | Result                                                          |
|---------------------------------------|-----------------------------------------------------------------|
| .sfz, .sf2 or .dspreset file          | Instrument loads immediately                                    |
| .dslibrary pack                       | Extracted internally and loaded (no destination prompt); its presets are available in Instruments |
| Folder with SFZ/SF2/Decent Sampler    | Folder is added to the Library panel                           |
| .zip pack                             | You choose a destination, it extracts, then shows in Library   |

Only one instrument file (SFZ/SF2/.dspreset/.dslibrary) and one generic ZIP are loaded per drop;
all dragged folders are added.

### B  Open Button

Click **Open SFZ/SF2/DS/ZIP** (top-right of the main panel) to browse for .sfz, .sf2,
.dspreset, .dslibrary, or .zip files. "DS" stands for Decent Sampler.

### C  Instruments

Click an instrument in the Instruments tree on the left to load it. SoundFont presets appear
under their SoundFont name. Up/Down loads the previous/next instrument in open groups;
closed groups are skipped. Clicking the loaded name in the header opens Rename.

---

## Instruments Panel (left column)

Click **+** to add a library folder. **−** lets you choose a folder to remove from the
library; the files stay on disk. **R** rescans the folders. **Collapse all** closes all groups.
**F+** creates a SoundFont folder. Right-click a SoundFont heading and choose
**Move SoundFont to folder** to organize it. Right-click a folder to rename or remove it.
The SoundFont files stay in their original locations; folders are included in config exports.

Search filters this same tree across your SoundFonts and other instrument files. Matching
presets stay under their SoundFont name, with no file paths. **X** clears the search.
Category and minimum-rating filters can be combined with search.

## Ratings, Categories and Names

Click one of the five stars next to the loaded name to rate an instrument from 1–5.
Click the current rating again to clear it. **Category** opens category choices only,
including a custom category option. Click the top instrument name to rename it.
Right-click an instrument for rating, category and rename options; right-click a
SoundFont heading to rename the bank. Leave a custom name blank to restore its original.
Names, ratings and categories save in player settings and configuration exports;
the source instrument files stay unchanged.

---

## Parameters

### AMP Section

| Knob    | Range            | Description                              |
|---------|------------------|------------------------------------------|
| Volume  | −60 to +6 dB     | Master output level                      |
| Pan     | −100 to +100     | Stereo panning (L ← 0 → R)              |
| Semit.  | −24 to +24       | Pitch transpose in semitones             |
| Cents   | −100 to +100     | Fine pitch adjustment in cents           |
| A       | 0.001 – 10 s     | Amplitude Attack time                    |
| D       | 0.001 – 10 s     | Amplitude Decay time                     |
| S       | 0 – 100 %        | Amplitude Sustain level                  |
| R       | 0.001 – 10 s     | Amplitude Release time                   |

The **ADSR display** (to the right of the knobs) shows a live preview of the envelope shape.
The **L / R meters** show the current output level.

### Filter Section

| Knob   | Description                                             |
|--------|---------------------------------------------------------|
| Type   | LP (Low-Pass) · HP (High-Pass) · BP (Band-Pass) · Notch |
| Cutoff | Filter cutoff frequency — 20 Hz to 20 kHz              |
| Res    | Resonance / Q factor                                    |
| Env    | Filter envelope amount (−1 = full down, +1 = full up)  |
| A/D/S/R| Filter envelope shape (same ranges as AMP)             |

### Modulation (MOD) Section

Two independent LFOs (LFO 1 and LFO 2).

| Control | Options / Range          | Description              |
|---------|--------------------------|--------------------------|
| Shape   | Sine · Tri · Saw · Sq · S&H | LFO waveform shape    |
| Target  | Pitch · Cutoff · Pan · Vol  | Modulation destination |
| Rate    | 0.1 – 20 Hz              | LFO speed                |
| Depth   | 0 – 1                    | Modulation amount        |

### FX Section

Seven effect slots in two rows, each enabled with its labelled ON/OFF button. Adjusting a chorus, phaser, flanger, delay, or reverb knob also enables that effect.

Signal order: Drive → Chorus → Phaser → Flanger → Delay → Reverb → EQ.

| Effect | Parameters           | Description                          |
|--------|----------------------|--------------------------------------|
| DRIVE  | Drive · Mix          | Soft saturation / overdrive          |
| CHORUS | Rate · Depth · Mix   | Chorus modulation effect             |
| PHASER | Rate · Depth · Mix   | Six-stage sweeping phase effect      |
| FLANGER | Rate · Depth · Mix  | Short modulated delay with feedback  |
| DELAY  | Time · Fbk · Mix     | Stereo echo (1 ms – 2000 ms)        |
| REVERB | Size · Damp · Mix    | Algorithmic reverb                   |
| EQ     | Low · Mid · High     | Three-band EQ (±12 dB per band)     |

### Footer

| Control  | Description                                      |
|----------|--------------------------------------------------|
| Voices   | Maximum simultaneous voices: 1 – 64             |
| RR Reset | Reset round-robin counters to the first sample  |

### EQ Graph

The graph shows the combined response of Low (200 Hz shelf),
Mid (1 kHz peak) and High (4 kHz shelf). It follows knob changes and host automation.
Choose **EQ response** from **Visuals** in the bottom panel above the keys. With EQ off, the graph shows a flat bypass response.

### Visualizer

Use **Visual** beside Voices to choose Tiny, Small, Medium, Large, or Hide. Drag the divider
above the visual panel for a custom height; double-click it to hide/restore the visualizer.


**Visuals → Add GIF...** loads an animated GIF. You can also drag a GIF onto the player. **My GIFs** lets you choose between your
added GIFs. Choices are remembered; keep the GIF files available in their original locations.
The visual height stays fixed while the window is resized. Drag the left divider to change
the browser split; it does not change the window dimensions.

The panel below the footer (above the keyboard) shows a decorative animation that reacts to the
sound the plugin produces — it idles gently when nothing plays and comes alive with the audio
level. Click the small gear button in its bottom-right corner to pick from several animation
styles (Starfield, Bubbles, Spectrum Bars, Pulse Rings, Plasma Glow, Fireflies, Orbit, Waveform);
your choice is remembered and its colours automatically match the current theme (dark or light).

---

## Recording (Standalone only)

The recording transport bar sits in the footer, between the Voices slider and RR Reset.
It is only available in the **Standalone application** — it is completely hidden in the
VST3 plugin.

| Control      | Description                                                            |
|---------------|-------------------------------------------------------------------------|
| ● Record      | Starts/stops recording the plugin's full audio output to MP3          |
| ▶ Play        | Plays back the last take instantly (kept in memory, no file re-read)  |
| ■ Stop        | Stops recording or playback                                            |
| 📁 Folder      | Choose the destination folder for recordings (defaults to Desktop)    |
| Time counter  | Shows elapsed recording time, or position/total during playback       |

Recordings capture whatever is actually sounding — notes played from an external MIDI
device or from the PC keyboard — after all effects, exactly as heard. Files are saved as
`DSK_SFz_Player_YYYY-MM-DD_HHMMSS.mp3` in the chosen folder. Maximum recording length is
10 minutes per take; recording stops automatically (and the file is finalised) if that
limit is reached.

---

## Options Menu

Click **Options** (top-right) to access:

| Item                     | Description                                                  |
|--------------------------|--------------------------------------------------------------|
| All notes off            | Immediately silence all playing notes (MIDI panic)          |
| Reset round-robin …      | Restart round-robin sample cycling from the beginning        |
| Reset all parameters     | Restore every knob / button to its default value             |
| Export config…           | Save library folders + favorites + theme to a .json file     |
| Import config…           | Restore from a previously exported .json file                |
| Theme                    | Change the colour theme (see Themes section)                 |
| Help…                    | Open this manual                                             |
| About…                   | Plugin version and website link                              |

---

## Themes

Six colour themes are available under **Options → Theme**:

| Theme       | Style                          |
|-------------|--------------------------------|
| Deep Navy   | Dark — cyan accents (default)  |
| Midnight    | Dark — orange accents          |
| Forest      | Dark — green accents           |
| Deep Purple | Dark — purple accents          |
| Light Gray  | Light — blue accents           |
| Warm Cream  | Light — brown / red accents    |

The selected theme is saved automatically and restored on the next launch.
It is also included when you export the config file.

---

## Export & Import Config

### Export config…
Saves the following to a .json file of your choice:
- Library folder paths
- All Favorites (root entries and named folders)
- Current theme

### Import config…
Restores the above from a previously exported .json file.
The existing library and favorites are completely replaced by the imported data.

> **Note:** After extracting a .zip instrument pack, the Library shows only the
> newly extracted folder. Previous library entries are replaced.

---

## Tips & Shortcuts

- **PC Keyboard → MIDI:** Click the plugin window to focus it, then type to play notes
  (standard piano layout: A=C4, W=C#4, S=D4, E=D#4, D=E4, F=F4, etc.).
- **Hover tooltips:** Hover any knob or button to see its name and range.
- **Right-click knobs:** Shows a popup to type an exact value or reset to default.
- **Resizable window:** Drag the bottom-right corner to resize (min 860 × 360).

---
---

# ESPAÑOL

## Descripción general

DSK SFz Player es un sampler SFZ/SF2/Decent Sampler polifónico de 64 voces disponible como plugin
VST3 y como aplicación independiente (Standalone). Carga cualquier instrumento en formato SFZ,
SF2 (SoundFont2) o Decent Sampler (.dspreset / .dslibrary) y tócalo con control MIDI completo,
cinco efectos y automatización de parámetros en tiempo real.

Formatos de audio compatibles dentro de las librerías SFZ/Decent Sampler: WAV, FLAC, OGG. Los
archivos SF2 (SoundFont2) también son compatibles directamente. Nota: en instrumentos SF2, las
secciones Amp ADSR y Filter/Mod (LFO) sustituyen la envolvente y el filtro propios del soundfont
por los knobs del plugin (mismo comportamiento que en SFZ). El ADSR de amplitud se aplica por
nota; el Filtro y el Mod se aplican sobre la mezcla final en vez de por voz, ya que el motor SF2
renderiza todas las voces ya mezcladas. Los instrumentos Decent Sampler (.dspreset/.dslibrary) se
convierten al mismo motor que usa SFZ, por lo que todos los controles del plugin (ADSR, filtro,
LFOs, efectos, round robin, crossfades) funcionan exactamente igual que con SFZ; los bloques
`<effects>` y `<ui>` propios del preset no se importan, ya que el plugin siempre usa su propia
interfaz y cadena de efectos fija, independientemente del formato (mismo criterio que con SF2).

---

## Cargar un instrumento

### A  Arrastrar y soltar (sobre la ventana del plugin)

| Qué arrastras                        | Resultado                                                              |
|----------------------------------------|--------------------------------------------------------------------------|
| Archivo .sfz, .sf2 o .dspreset        | El instrumento se carga inmediatamente                                 |
| Pack .dslibrary                       | Se extrae internamente y se carga (sin preguntar destino); sus presets aparecen en Instruments |
| Carpeta con SFZ/SF2/Decent Sampler    | La carpeta se añade al panel de Librería                              |
| Pack .zip                             | Eliges dónde extraer, se descomprime y aparece en la Librería         |

Solo se carga un archivo de instrumento (SFZ/SF2/.dspreset/.dslibrary) y un ZIP genérico por
soltar; todas las carpetas arrastradas se añaden.

### B  Botón Open

Haz clic en **Open SFZ/SF2/DS/ZIP** (arriba a la derecha) para buscar archivos .sfz,
.sf2, .dspreset, .dslibrary o .zip. "DS" significa Decent Sampler.

### C  Instruments

Haz clic en un instrumento del árbol Instruments para cargarlo. Los presets SF2 aparecen
bajo el nombre de su SoundFont. Arriba/Abajo carga el instrumento anterior/siguiente de los
grupos abiertos; omite los grupos cerrados. Haz clic en el nombre superior para renombrarlo.

---

## Panel Instruments (columna izquierda)

**+** añade una carpeta. **−** permite elegir una carpeta para quitarla de la biblioteca;
los archivos permanecen en disco. **R** actualiza la lista. **Collapse all** cierra los grupos.
**F+** crea una carpeta SoundFont. Haz clic derecho en la cabecera de un SoundFont y
elige **Move SoundFont to folder**. El clic derecho en una carpeta permite renombrarla
o quitarla; no modifica la ubicación de los archivos. Las carpetas se incluyen en la exportación.
La búsqueda filtra el mismo árbol y conserva los presets bajo su SoundFont, sin rutas de
archivo. **X** borra la búsqueda. Puedes combinarla con filtros de categoría y valoración.

## Estrellas, categorías y nombres

Haz clic en una de las cinco estrellas para valorar el instrumento de 1 a 5. Haz clic en
la valoración actual para borrarla. **Category** muestra solamente categorías, incluida
una opción personalizada. Haz clic en el nombre superior para renombrar el instrumento.
El clic derecho ofrece valoración, categoría y nombre; en una cabecera SoundFont permite
renombrar el banco. Un nombre vacío restaura el original. Estos datos se guardan en los
ajustes y se incluyen en la exportación de configuración; no modifican el archivo original.

---

## Parámetros

### Sección AMP

| Knob    | Rango            | Descripción                              |
|---------|------------------|------------------------------------------|
| Volume  | −60 a +6 dB      | Nivel de salida maestro                  |
| Pan     | −100 a +100      | Panorama estéreo (Izq ← 0 → Der)        |
| Semit.  | −24 a +24        | Transposición en semitonos               |
| Cents   | −100 a +100      | Afinación fina en centésimas             |
| A       | 0.001 – 10 s     | Tiempo de Ataque de la amplitud          |
| D       | 0.001 – 10 s     | Tiempo de Caída (Decay)                  |
| S       | 0 – 100 %        | Nivel de Sostenimiento (Sustain)         |
| R       | 0.001 – 10 s     | Tiempo de Liberación (Release)           |

La **visualización ADSR** (a la derecha de los knobs) muestra una previsualización en tiempo real.
Los **medidores L / R** muestran el nivel de salida actual.

### Sección Filter (Filtro)

| Knob   | Descripción                                                      |
|--------|------------------------------------------------------------------|
| Type   | LP (Paso Bajo) · HP (Paso Alto) · BP (Paso de Banda) · Notch   |
| Cutoff | Frecuencia de corte — 20 Hz a 20 kHz                            |
| Res    | Resonancia / Factor Q                                            |
| Env    | Cantidad de envolvente (−1 = baja, +1 = sube)                   |
| A/D/S/R| Forma de la envolvente del filtro (mismos rangos que AMP)       |

### Sección de Modulación (MOD)

Dos LFOs independientes (LFO 1 y LFO 2).

| Control | Opciones / Rango               | Descripción              |
|---------|--------------------------------|--------------------------|
| Shape   | Sine · Tri · Saw · Sq · S&H   | Forma de onda del LFO    |
| Target  | Pitch · Cutoff · Pan · Vol     | Destino de modulación    |
| Rate    | 0.1 – 20 Hz                    | Velocidad del LFO        |
| Depth   | 0 – 1                          | Cantidad de modulación   |

### Sección FX (Efectos)

Cinco ranuras de efectos independientes, cada una activada con su botón.

| Efecto | Parámetros           | Descripción                             |
|--------|----------------------|-----------------------------------------|
| DRIVE  | Drive · Mix          | Saturación suave / overdrive            |
| CHORUS | Rate · Depth · Mix   | Efecto de chorus con modulación         |
| PHASER | Rate · Depth · Mix   | Six-stage sweeping phase effect      |
| FLANGER | Rate · Depth · Mix  | Short modulated delay with feedback  |
| DELAY  | Time · Fbk · Mix     | Eco estéreo (1 ms – 2000 ms)           |
| REVERB | Size · Damp · Mix    | Reverberación algorítmica               |
| EQ     | Low · Mid · High     | Ecualizador de tres bandas (±12 dB)    |

### Pie de página

| Control  | Descripción                                                  |
|----------|--------------------------------------------------------------|
| Voices   | Número máximo de voces simultáneas: 1 – 64                  |
| RR Reset | Reinicia los contadores de round-robin a la primera muestra  |

### Gráfica EQ

Elige **EQ response** en **Visuals**, encima del teclado. La gráfica muestra la respuesta conjunta de Low (200 Hz), Mid (1 kHz) y High
(4 kHz). Sigue los controles y la automatización. Con EQ apagado, la respuesta es plana.
La altura del visual se mantiene al redimensionar la ventana.

### Visualizador

**Visual**, junto a Voices, permite elegir Tiny, Small, Medium, Large o Hide. Arrastra el
separador encima del panel visual para ajustar la altura; un doble clic lo oculta/restaura.


**Visuals → Add GIF...** añade un GIF animado; también puedes arrastrarlo al reproductor. **My GIFs** permite elegir entre los GIFs
añadidos. La selección se guarda; conserva los archivos GIF en su ubicación original.
Arrastrar el separador izquierdo cambia la anchura de la biblioteca sin redimensionar la ventana.

El panel situado debajo del pie de página (justo encima del teclado) muestra una animación
decorativa que reacciona al sonido que emite el plugin: se mueve suavemente en reposo y cobra
vida con el nivel de audio. Haz clic en el pequeño botón con forma de engranaje en su esquina
inferior derecha para elegir entre varios estilos de animación (Starfield, Bubbles, Spectrum
Bars, Pulse Rings, Plasma Glow, Fireflies, Orbit, Waveform); tu elección se recuerda y sus
colores se adaptan automáticamente al tema actual (oscuro o claro).

---

## Grabación (solo Standalone)

La barra de transporte de grabación está en el pie de página, entre el slider Voices y
RR Reset. Solo está disponible en la **aplicación Standalone** — queda completamente
oculta en el plugin VST3.

| Control        | Descripción                                                              |
|-----------------|---------------------------------------------------------------------------|
| ● Grabar        | Inicia/detiene la grabación de la salida de audio completa del plugin a MP3 |
| ▶ Reproducir    | Reproduce la última toma al instante (queda en memoria, sin releer el archivo) |
| ■ Detener       | Detiene la grabación o la reproducción                                    |
| 📁 Carpeta       | Elige la carpeta de destino de las grabaciones (por defecto, Escritorio) |
| Contador        | Muestra el tiempo de grabación transcurrido, o la posición/total al reproducir |

La grabación captura lo que realmente suena — notas tocadas desde un dispositivo MIDI
externo o desde el teclado del PC — después de todos los efectos, tal como se escucha.
Los archivos se guardan como `DSK_SFz_Player_YYYY-MM-DD_HHMMSS.mp3` en la carpeta elegida.
La duración máxima de grabación es de 10 minutos por toma; la grabación se detiene sola
(y el archivo se finaliza) si se alcanza ese límite.

---

## Menú Options

Haz clic en **Options** (arriba a la derecha) para acceder a:

| Opción                   | Descripción                                                          |
|--------------------------|----------------------------------------------------------------------|
| All notes off            | Silencia inmediatamente todas las notas (pánico MIDI)               |
| Reset round-robin …      | Reinicia el ciclo de muestras round-robin desde el principio        |
| Reset all parameters     | Restaura todos los controles a sus valores por defecto              |
| Export config…           | Guarda librería + favoritos + tema en un archivo .json              |
| Import config…           | Restaura desde un archivo .json exportado anteriormente             |
| Theme                    | Cambia el tema de color (ver sección Temas)                         |
| Help…                    | Abre este manual                                                     |
| About…                   | Versión del plugin y enlace a la web                                |

---

## Temas (Themes)

Hay seis temas de color disponibles en **Options → Theme**:

| Tema        | Estilo                               |
|-------------|--------------------------------------|
| Deep Navy   | Oscuro — acentos cyan (predeterminado) |
| Midnight    | Oscuro — acentos naranja             |
| Forest      | Oscuro — acentos verde               |
| Deep Purple | Oscuro — acentos púrpura             |
| Light Gray  | Claro — acentos azul                 |
| Warm Cream  | Claro — acentos marrón / rojo        |

El tema seleccionado se guarda automáticamente y se restaura al siguiente inicio.
También se incluye al exportar el archivo de configuración.

---

## Exportar e Importar configuración

### Export config…
Guarda lo siguiente en un archivo .json:
- Rutas de las carpetas de la librería
- Todos los Favoritos (entradas raíz y carpetas con nombre)
- Tema actual

### Import config…
Restaura lo anterior desde un archivo .json exportado previamente.
La librería y los favoritos existentes se reemplazan completamente con los datos importados.

> **Nota:** Al extraer un pack .zip de instrumentos, la Librería mostrará únicamente
> la carpeta recién extraída. Las entradas anteriores de la librería se reemplazan.

---

## Consejos y atajos

- **Teclado del PC → MIDI:** Haz clic en la ventana del plugin para darle el foco y
  escribe para tocar notas (A=Do4, W=Do#4, S=Re4, E=Re#4, D=Mi4, F=Fa4, etc.).
- **Tooltips al pasar el cursor:** Pasa el cursor sobre cualquier knob o botón para ver
  su nombre y rango.
- **Clic derecho en los knobs:** Muestra un menú para introducir un valor exacto o restablecer.
- **Ventana redimensionable:** Arrastra la esquina inferior derecha (mín. 860 × 360).

---

*DSK Music · www.dskmusic.com*

### Glide and modulation wheel

The **Glide ms** knob in AMP slides new notes from the previous pitch over the chosen time (0–2000 ms). Set it to 0 to turn glide off. Use **Voices = 1** for solo note slides. Glide works with SFZ, SoundFont and Decent Sampler instruments.

In MOD, choose **Wheel: LFO 1**, **Wheel: LFO 2**, or **Wheel: Both** to control modulation depth with a MIDI modulation wheel (CC1). The Depth knobs set the maximum depth: wheel down gives zero modulation, wheel up gives the full knob setting. **Wheel: Off** keeps the knobs in sole control. These settings are saved with the plugin state and can be automated.

The piano keyboard automatically uses dark keys with dark themes and light keys with light themes. Played notes remain highlighted in the theme accent colour.

### Adjustable EQ bands

Choose **Visuals → EQ**. Drag the Low, Mid or High handle horizontally to change its frequency and vertically to boost or cut. Scroll over a handle to adjust its width; larger widths affect a broader range. Right-click a handle to type frequency, gain and width. Low always stays below Mid, and High stays above Mid, including when automating or restoring a preset. Turn EQ on to show and edit the band handles. Changes are saved in plugin state.
