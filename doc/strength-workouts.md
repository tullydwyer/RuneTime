# Strength workouts

Every confirmed rep awards **4 Strength XP**, with the existing level thresholds
and saved progress. All exercises can be logged: bench press, curls, rows,
presses, squats, deadlifts, leg machines, and lifts with the other arm. There is
no exercise-specific multiplier or requirement to move the watch to earn XP.

## On the watch

- **START** begins an automatic estimate. Hold your wrist still briefly, then
  perform the set. **FINISH** (or the side button) stops counting for review.
- Review the **SET** rep count. Use **− / +** to correct missed or extra reps;
  hold a button to repeat. **SAVE** awards XP once and writes it to flash.
- **LOG** skips the sensor and opens the same editable rep count. It starts at
  10 reps, or the last nonzero saved set in this visit to the app. Adjust to the
  reps actually completed, then save. This is useful for fixed-wrist bench
  presses, leg machines, the unwatched arm, and very slow or short movements.
- Save a zero-rep set to discard it without XP. The side button/back navigation
  from review also discards the unconfirmed set. Save before leaving the app.
- Only START enables automatic estimates; rest, setup, and manual editing do
  not award XP. The display stays awake during an automatic set. A save failure
  is shown and retried while the screen remains open.

Automatic counting is an estimate, not exercise recognition. A wrist
accelerometer cannot reliably distinguish every lifting pattern or see a lift
that does not move that wrist. Confirmed set entry is the reliable path for all
exercises; do not rely on automatic estimates being exact for presses.

## Detection changes

The old detector required a large departure from its initial acceleration
vector, and moved that baseline as soon as a rep entered its return window.
The baseline could creep toward the peak, causing later repetitions to be
missed. It also treated reps over eight seconds as incomplete and could stay
anchored to an old posture after changing exercises.

The detector now keeps the rest baseline across reps, filters short noise,
accepts smaller excursions, allows up to 20 seconds for a return, and reanchors
after eight seconds in a stable new posture. Resting that long midway through
a rep can intentionally reset the estimate. Missing samples restart calibration.
These heuristics still favor lifts that rotate the wrist; manual correction is
part of the workout flow, rather than silently withholding experience.

## Verification

```sh
mkdir -p build/ui-preview
c++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -Isrc \
  tests/strength_workout.cpp -o build/ui-preview/strength-test
build/ui-preview/strength-test
python3 scripts/render_runetime_ui.py
```

Tests cover synthetic repeated motion across axes, either direction, small
ranges and 2/4/12-second tempos; stationary noise, a brief impact, changed
posture, sample gaps and timestamp wrap; corrected/manual sets, zero reps,
repeated save attempts, late sensor counts, rep bounds and the XP cap. LVGL
previews check the ready, counting and review layouts with maximum values.
Synthetic traces are not evidence of accuracy on a real workout.

Build and installation use [the macOS OTA guide](ota-macos.md). The firmware
revision is `1.16.0-runetime-strength2`. On-device acceptance checks:

1. Compare estimates against ten actual curls and ten bench presses, with
   different tempos. Correct each count and save; ten reps must add 40 XP.
2. Log a leg-machine set without moving the wrist; confirm the same XP reward.
3. Rest, change posture, start another set, and check counting recovers.
4. Reopen the app and reboot after saving to verify XP persists.
5. Validate installed firmware under **Settings → Firmware → Validate**.
