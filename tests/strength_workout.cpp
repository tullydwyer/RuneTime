#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include "components/motion/StrengthWorkout.h"
#include "components/motion/StrengthSet.h"

using Pinetime::Controllers::StrengthSet;
using Pinetime::Controllers::StrengthWorkout;

struct Trace {
  StrengthWorkout workout;
  uint32_t time = 0;
  unsigned events = 0;

  void Sample(int x, int y, int z) {
    events += workout.Update(x, y, z, time);
    time += 100;
    assert(events == workout.Reps());
  }

  void Hold(unsigned duration, int x = 0, int y = 0, int z = 1024) {
    for (unsigned i = 0; i < duration; i += 100) {
      Sample(x, y, z);
    }
  }

  // Synthetic orientation traces, not recordings from an actual lifter.
  void Curl(unsigned duration, double angle, unsigned axis, int sign, unsigned rest = 600) {
    constexpr double pi = 3.141592653589793;
    for (unsigned i = 0; i < duration; i += 100) {
      const double theta = angle * (1 - std::cos(2 * pi * i / duration)) / 2;
      const int moving = sign * static_cast<int>(1024 * std::sin(theta));
      const int gravity = static_cast<int>(1024 * std::cos(theta));
      if (axis == 0) {
        Sample(moving, 0, gravity);
      } else if (axis == 1) {
        Sample(0, moving, gravity);
      } else {
        Sample(gravity, moving, 0);
      }
    }
    if (axis == 2) {
      Hold(rest, 1024, 0, 0);
    } else {
      Hold(rest);
    }
  }
};

int main() {
  assert(StrengthWorkout::Level(0) == 1);
  assert(StrengthWorkout::Level(82) == 1);
  assert(StrengthWorkout::Level(83) == 2);
  assert(StrengthWorkout::Level(13034430) == 98);
  assert(StrengthWorkout::Level(13034431) == 99);
  assert(StrengthWorkout::Level(StrengthWorkout::maxXp) == 99);
  assert(StrengthWorkout::XpForLevel(2) == 83);
  assert(StrengthWorkout::XpForLevel(99) == 13034431);

  // Smaller ranges, slow tempos, either wrist and multiple starting postures.
  for (unsigned axis = 0; axis < 3; ++axis) {
    for (int sign : {-1, 1}) {
      for (unsigned duration : {2000u, 4000u, 12000u}) {
        Trace trace;
        trace.Hold(1000, axis == 2 ? 1024 : 0, 0, axis == 2 ? 0 : 1024);
        assert(!trace.workout.IsCalibrating());
        for (unsigned rep = 1; rep <= 5; ++rep) {
          trace.Curl(duration, 0.35, axis, sign);
          if (trace.workout.Reps() != rep) {
            std::fprintf(stderr, "axis=%u sign=%d duration=%u rep=%u actual=%u\n", axis, sign, duration, rep, trace.workout.Reps());
          }
          assert(trace.workout.Reps() == rep);
        }
      }
    }
  }

  // Continuous reps without a deliberate pause at the bottom.
  for (double angle : {0.35, 0.6, 1.5}) {
    Trace continuous;
    continuous.Hold(1000);
    for (unsigned rep = 0; rep < 12; ++rep) {
      continuous.Curl(2000, angle, 0, 1, 0);
    }
    continuous.Hold(600);
    assert(continuous.events == 12);
  }

  Trace noise;
  noise.Hold(1000);
  for (unsigned i = 0; i < 600; ++i) {
    noise.Sample(i % 2 ? 25 : -25, i % 3 ? 15 : -15, 1024 + static_cast<int>(i % 11) - 5);
  }
  noise.Sample(1000, 0, 1024); // One isolated knock, not a rep.
  noise.Hold(2000);
  assert(noise.events == 0);

  Trace posture;
  posture.Hold(1000);
  posture.Hold(10000, 1024, 0, 0); // Rest in a new pose, then lift.
  assert(posture.events == 0);
  posture.Curl(3000, 0.6, 2, 1);
  assert(posture.events == 1);

  Trace gap;
  gap.Hold(1000);
  gap.Hold(500, 600, 0, 900);
  gap.time += 2000; // Missing samples must not complete a partial rep.
  gap.Hold(1000);
  assert(gap.events == 0);
  gap.Curl(3000, 0.6, 0, 1);
  assert(gap.events == 1);
  gap.workout.Reset();
  assert(gap.workout.Reps() == 0 && gap.workout.IsCalibrating());

  Trace rollover;
  rollover.time = UINT32_MAX - 1500;
  rollover.Hold(1000);
  rollover.Curl(3000, 0.6, 0, 1);
  assert(rollover.events == 1);

  // XP is based on confirmed sets: undetectable lifts earn exactly the same XP.
  StrengthSet set;
  uint32_t xp = 83;
  set.Start(100);
  set.Observe(107);
  assert(set.Reps() == 7);
  assert(set.Confirm(xp) == xp); // Cannot save an unfinished estimate.
  set.Finish();
  set.Observe(108); // Sensor command lag / movement while editing is ignored.
  set.Adjust(3);
  xp = set.Confirm(xp);
  assert(xp == 123);
  assert(set.Confirm(xp) == xp); // Refresh / retry cannot award twice.
  set.LogManually();             // Bench, leg press, opposite arm: no motion required.
  assert(set.Reps() == 10);
  set.Adjust(2);
  xp = set.Confirm(xp);
  assert(xp == 171);
  set.LogManually();
  assert(set.Reps() == 12);
  set.Adjust(-1000);
  assert(set.Reps() == 0);
  assert(set.Confirm(xp) == xp); // Zero reps discards a mistaken set.
  set.LogManually();
  set.Adjust(2000);
  assert(set.Reps() == StrengthSet::maxReps);
  assert(set.Confirm(StrengthWorkout::maxXp - 1) == StrengthWorkout::maxXp);
  set.Start(UINT32_MAX - 2);
  set.Observe(2);
  assert(set.Reps() == 5); // Lifetime sensor counter wrap.
  puts("Strength motion estimates, set correction, manual logging and XP: PASS");
}
