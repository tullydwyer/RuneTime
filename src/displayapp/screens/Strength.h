#pragma once

#include <cstdint>
#include <lvgl/lvgl.h>

#include "components/motion/MotionController.h"
#include "displayapp/Controllers.h"
#include "displayapp/apps/Apps.h"
#include "displayapp/screens/Screen.h"
#include "systemtask/WakeLock.h"

namespace Pinetime::Applications::Screens {
  class Strength : public Screen {
  public:
    Strength(Controllers::MotionController& motionController, Controllers::FS& filesystem, System::SystemTask& systemTask);
    ~Strength() override;

    void Refresh() override;
    void ToggleWorkout();

  private:
    bool ReadProgress(const char* path, uint32_t& result);
    void LoadProgress();
    bool SaveProgress();
    void Render();

    Controllers::MotionController& motionController;
    Controllers::FS& filesystem;
    System::WakeLock wakeLock;
    uint32_t xp = 0;
    uint32_t savedXp = 0;
    uint32_t sessionReps = 0;
    uint32_t observedReps = 0;
    bool workoutRunning = true;
    bool saveFailed = false;

    lv_obj_t* levelLabel;
    lv_obj_t* xpLabel;
    lv_obj_t* nextLabel;
    lv_obj_t* repsLabel;
    lv_obj_t* statusLabel;
    lv_obj_t* toggleButton;
    lv_obj_t* toggleLabel;
    lv_task_t* refreshTask;
  };
}

namespace Pinetime::Applications {
  template <>
  struct AppTraits<Apps::Strength> {
    static constexpr Apps app = Apps::Strength;
    static constexpr const char* icon = "STR";

    static Screens::Screen* Create(AppControllers& controllers) {
      return new Screens::Strength(controllers.motionController, controllers.filesystem, *controllers.systemTask);
    }

    static bool IsAvailable(Controllers::FS& /*filesystem*/) {
      return true;
    }
  };
}
