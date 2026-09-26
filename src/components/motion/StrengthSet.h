#pragma once

#include <algorithm>
#include <cstdint>
#include "components/motion/StrengthWorkout.h"

namespace Pinetime::Controllers {
  // Sensor reps are estimates until the lifter confirms the completed set.
  // Manual entry also supports exercises with no observable wrist movement.
  class StrengthSet {
  public:
    enum class State { Ready, Counting, Review };
    static constexpr uint16_t maxReps = 999;

    void Start(uint32_t sensorReps) {
      if (state != State::Ready) {
        return;
      }
      reps = 0;
      observedReps = sensorReps;
      state = State::Counting;
    }

    void Observe(uint32_t sensorReps) {
      if (state == State::Counting) {
        reps = static_cast<uint16_t>(std::min<uint64_t>(maxReps, static_cast<uint64_t>(reps) + (sensorReps - observedReps)));
        observedReps = sensorReps;
      }
    }

    void Finish() {
      if (state == State::Counting) {
        state = State::Review;
      }
    }

    void LogManually() {
      if (state == State::Ready) {
        reps = lastSavedReps;
        state = State::Review;
      }
    }

    void Adjust(int delta) {
      if (state == State::Review) {
        reps = static_cast<uint16_t>(std::max(0, std::min(static_cast<int>(maxReps), reps + delta)));
      }
    }

    uint32_t Confirm(uint32_t xp) {
      if (state != State::Review) {
        return xp;
      }
      state = State::Ready;
      if (reps > 0) {
        lastSavedReps = reps;
      }
      return static_cast<uint32_t>(
        std::min<uint64_t>(StrengthWorkout::maxXp, static_cast<uint64_t>(xp) + reps * StrengthWorkout::xpPerRep));
    }

    State CurrentState() const {
      return state;
    }

    uint16_t Reps() const {
      return reps;
    }

  private:
    State state = State::Ready;
    uint16_t reps = 0;
    uint16_t lastSavedReps = 10;
    uint32_t observedReps = 0;
  };
}
