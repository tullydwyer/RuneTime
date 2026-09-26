# RuneTime UI

The default digital face is now **RuneTime** (Settings → Watch face). The other
restyled faces are **Guild clock** and **Quest journal**. Existing watch-face
selection and all workout/XP data remain intact.

Inspired by the [2004Scape gallery](https://lostcity.rs/t/my-2004scape-diary-gallery-no-clickbait-check-the-item/7765/3), the shared UI uses pixel-drawn, chipped cobblestone and frayed parchment scrolls. The artwork is generated deterministically by `src/displayapp/icons/runetime/generate.py` as compact 4-bit LVGL images and linked into the firmware, without a downloaded resource pack. Dark ink, moss-green accents, gold headings, and a wax quest seal echo the 2004 interface. The six-slot inventory has colored skill icons and readable app captions; quick settings has Light, Torch, Chat and Settings tabs. Settings,
notifications, timers, fitness apps and system dialogs use the shared palette.
Special-purpose colors, such as the paint palette and flashlight output, retain
their meaning. Legacy customizable faces remain available.

On the RuneTime face, Hitpoints shows measured heart rate (or `--` when stopped),
and Agility shows the actual daily step count. The daily walking quest tracks the
configured step goal and shows **QUEST COMPLETE!** at 100%. It does not award or
modify Strength XP.

## Validation

Build using `doc/ota-macos.md`, then run:

```sh
python3 scripts/render_runetime_ui.py
c++ -std=c++17 -Isrc tests/strength_workout.cpp -o build/ui-preview/strength-test
build/ui-preview/strength-test
```

The native harness uses the firmware's LVGL sources, generated fonts, shared
home, launcher and settings widgets, and theme. It checks label bounds, long numbers, AM/PM,
quest states, touch-target sizes, inert captions and theme reinitialization.
It renders 240×240 PPM previews in `build/ui-preview/`. The Strength regression
test covers level thresholds and workout motion detection. These checks do not
replace exercising touch, sensor readings and transitions on the physical watch.

After OTA, check the GATT firmware revision is `1.16.0-runetime2004`, then validate
on the watch using Settings → Firmware → Validate.
