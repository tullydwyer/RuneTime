#pragma once

#include <array>
#include <cstdint>

namespace Pinetime::Controllers {
  class StrengthWorkout {
  public:
    static constexpr uint32_t xpPerRep = 4;
    static constexpr uint32_t maxXp = 200000000;

    static uint8_t Level(uint32_t xp) {
      uint8_t level = 1;
      while (level < levelXp.size() && xp >= levelXp[level]) {
        ++level;
      }
      return level;
    }

    static uint32_t XpForLevel(uint8_t level) {
      if (level <= 1) {
        return 0;
      }
      if (level > levelXp.size()) {
        return levelXp.back();
      }
      return levelXp[level - 1];
    }

    void Reset() {
      state = State::Calibrating;
      lastSample = 0;
      phaseStart = 0;
      baseline = {};
      filtered = {};
      stableSample = {};
      stableSince = 0;
      reps = 0;
      hasSample = false;
    }

    bool Update(int16_t x, int16_t y, int16_t z, uint32_t milliseconds) {
      const Vector raw {x, y, z};
      if (!hasSample || milliseconds - lastSample > 1000) {
        baseline = raw;
        filtered = raw;
        stableSample = raw;
        stableSince = milliseconds;
        state = State::Calibrating;
        phaseStart = milliseconds;
        hasSample = true;
      }
      lastSample = milliseconds;
      // 10 Hz input, low-pass to reject a single vibration/impact sample.
      filtered = {static_cast<int16_t>((filtered.x + x) / 2),
                  static_cast<int16_t>((filtered.y + y) / 2),
                  static_cast<int16_t>((filtered.z + z) / 2)};
      const Vector sample = filtered;
      if (DistanceSquared(sample, stableSample) > 60u * 60u) {
        stableSample = sample;
        stableSince = milliseconds;
      }
      // Recover when a new exercise starts in a different posture. Allow long
      // pauses within a rep before treating the held position as a new baseline.
      if (state != State::Calibrating && milliseconds - stableSince >= 8000) {
        baseline = sample;
        state = State::Ready;
      }

      const uint64_t distance = DistanceSquared(sample, baseline);
      switch (state) {
        case State::Calibrating:
          if (distance > 80u * 80u) {
            baseline = sample;
            phaseStart = milliseconds;
          } else if (milliseconds - phaseStart >= 500) {
            state = State::Ready;
          }
          break;
        case State::Ready:
          if (distance >= 220u * 220u) {
            state = State::Outbound;
            phaseStart = milliseconds;
          }
          break;
        case State::Outbound:
          if (distance < 160u * 160u) {
            state = State::Ready;
          } else if (milliseconds - phaseStart >= 200) {
            state = State::Returning;
          }
          break;
        case State::Returning:
          if (milliseconds - phaseStart > 20000) {
            baseline = sample;
            state = State::Calibrating;
            phaseStart = milliseconds;
          } else if (distance <= 120u * 120u && milliseconds - phaseStart >= 600) {
            ++reps;
            // Keep the original rest pose; moving it at the return threshold
            // makes it creep toward the peak and lose later reps in the set.
            state = State::Cooldown;
            phaseStart = milliseconds;
            return true;
          }
          break;
        case State::Cooldown:
          if (milliseconds - phaseStart >= 300) {
            state = State::Ready;
          }
          break;
      }
      return false;
    }

    uint32_t Reps() const {
      return reps;
    }

    bool IsCalibrating() const {
      return state == State::Calibrating;
    }

  private:
    struct Vector {
      int16_t x = 0;
      int16_t y = 0;
      int16_t z = 0;
    };

    static uint64_t DistanceSquared(Vector a, Vector b) {
      const int64_t dx = static_cast<int32_t>(a.x) - b.x;
      const int64_t dy = static_cast<int32_t>(a.y) - b.y;
      const int64_t dz = static_cast<int32_t>(a.z) - b.z;
      return dx * dx + dy * dy + dz * dz;
    }

    enum class State { Calibrating, Ready, Outbound, Returning, Cooldown };
    State state = State::Calibrating;
    Vector baseline;
    Vector filtered;
    Vector stableSample;
    uint32_t stableSince = 0;
    uint32_t lastSample = 0;
    uint32_t phaseStart = 0;
    uint32_t reps = 0;
    bool hasSample = false;

    // OSRS thresholds: floor(sum(floor(level + 300 * 2^(level / 7))) / 4).
    static constexpr std::array<uint32_t, 99> levelXp = {
      0,       83,      174,     276,     388,     512,      650,      801,     969,     1154,    1358,    1584,    1833,
      2107,    2411,    2746,    3115,    3523,    3973,     4470,     5018,    5624,    6291,    7028,    7842,    8740,
      9730,    10824,   12031,   13363,   14833,   16456,    18247,    20224,   22406,   24815,   27473,   30408,   33648,
      37224,   41171,   45529,   50339,   55649,   61512,    67983,    75127,   83014,   91721,   101333,  111945,  123660,
      136594,  150872,  166636,  184040,  203254,  224466,   247886,   273742,  302288,  333804,  368599,  407015,  449428,
      496254,  547953,  605032,  668051,  737627,  814445,   899257,   992895,  1096278, 1210421, 1336443, 1475581, 1629200,
      1798808, 1986068, 2192818, 2421087, 2673114, 2951373,  3258594,  3597792, 3972294, 4385776, 4842295, 5346332, 5902831,
      6517253, 7195629, 7944614, 8771558, 9684577, 10692629, 11805606, 13034431};
  };
}
