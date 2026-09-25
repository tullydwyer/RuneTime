#include <cassert>
#include "components/motion/StrengthWorkout.h"

using Pinetime::Controllers::StrengthWorkout;

int main() {
  assert(StrengthWorkout::Level(0) == 1);
  assert(StrengthWorkout::Level(82) == 1);
  assert(StrengthWorkout::Level(83) == 2);
  assert(StrengthWorkout::Level(13034430) == 98);
  assert(StrengthWorkout::Level(13034431) == 99);
  assert(StrengthWorkout::Level(StrengthWorkout::maxXp) == 99);
  assert(StrengthWorkout::XpForLevel(2) == 83);
  assert(StrengthWorkout::XpForLevel(99) == 13034431);

  StrengthWorkout workout;
  for (uint32_t t = 0; t <= 600; t += 100) {
    assert(!workout.Update(0, 0, 1024, t));
  }
  assert(!workout.IsCalibrating());

  assert(!workout.Update(500, 0, 900, 700));
  assert(!workout.Update(500, 0, 900, 800));
  assert(!workout.Update(500, 0, 900, 900));
  assert(!workout.Update(0, 0, 1024, 1000));
  assert(workout.Update(0, 0, 1024, 1100));
  assert(workout.Reps() == 1);

  for (uint32_t t = 1200; t <= 1700; t += 100) {
    assert(!workout.Update(0, 0, 1024, t));
  }
  assert(!workout.Update(500, 0, 900, 1800));
  assert(!workout.Update(500, 0, 900, 1900));
  assert(!workout.Update(500, 0, 900, 2000));
  assert(!workout.Update(0, 0, 1024, 2100));
  assert(workout.Update(0, 0, 1024, 2200));
  assert(workout.Reps() == 2);

  workout.Reset();
  assert(workout.Reps() == 0);
  for (uint32_t t = 0; t <= 600; t += 100) {
    workout.Update(0, 0, 1024, t);
  }
  assert(!workout.Update(500, 0, 900, 700));
  assert(!workout.Update(0, 0, 1024, 800));
  assert(workout.Reps() == 0);
  assert(!workout.Update(500, 0, 900, 900));
  assert(!workout.Update(500, 0, 900, 1000));
  assert(!workout.Update(500, 0, 900, 9200));
  assert(workout.Reps() == 0);
}
