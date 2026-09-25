#include "displayapp/screens/Strength.h"

#include <algorithm>

#include "components/fs/FS.h"
#include "displayapp/icons/strength/strength.c"
#include "displayapp/InfiniTimeTheme.h"

using namespace Pinetime::Applications::Screens;

namespace {
  constexpr char progressPath[] = "/strength.dat";
  constexpr char temporaryPath[] = "/strength.tmp";
  constexpr uint32_t progressMagic = 0x52554e45;
  constexpr uint32_t progressVersion = 1;

  struct ProgressRecord {
    uint32_t magic;
    uint32_t version;
    uint32_t xp;
    uint32_t check;
  };

  void ToggleEvent(lv_obj_t* object, lv_event_t event) {
    if (event == LV_EVENT_CLICKED) {
      static_cast<Strength*>(object->user_data)->ToggleWorkout();
    }
  }
}

Strength::Strength(Controllers::MotionController& motionController, Controllers::FS& filesystem, System::SystemTask& systemTask)
  : motionController {motionController}, filesystem {filesystem}, wakeLock {systemTask} {
  LoadProgress();
  observedReps = motionController.StrengthReps();

  lv_obj_t* background = lv_obj_create(lv_scr_act(), nullptr);
  lv_obj_set_size(background, 240, 240);
  lv_obj_set_click(background, false);
  lv_obj_set_style_local_radius(background, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 0);
  lv_obj_set_style_local_border_width(background, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 0);
  lv_obj_set_style_local_bg_color(background, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, lv_color_make(22, 17, 13));

  lv_obj_t* title = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(title, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_make(232, 204, 143));
  lv_label_set_text_static(title, "STRENGTH");
  lv_obj_align(title, nullptr, LV_ALIGN_IN_TOP_MID, 0, 3);

  lv_obj_t* panel = lv_obj_create(lv_scr_act(), nullptr);
  lv_obj_set_size(panel, 224, 62);
  lv_obj_align(panel, nullptr, LV_ALIGN_IN_TOP_MID, 0, 27);
  lv_obj_set_click(panel, false);
  lv_obj_set_style_local_radius(panel, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 0);
  lv_obj_set_style_local_border_width(panel, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 2);
  lv_obj_set_style_local_border_color(panel, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, lv_color_make(135, 104, 57));
  lv_obj_set_style_local_bg_color(panel, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, lv_color_make(49, 37, 24));

  lv_obj_t* icon = lv_img_create(lv_scr_act(), nullptr);
  lv_img_set_src(icon, &strength_icon);
  lv_obj_align(icon, panel, LV_ALIGN_IN_LEFT_MID, 7, 0);
  lv_obj_set_click(icon, false);

  levelLabel = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_font(levelLabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_42);
  lv_obj_set_style_local_text_color(levelLabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_make(246, 217, 121));
  lv_obj_align(levelLabel, icon, LV_ALIGN_OUT_RIGHT_MID, 14, 0);

  xpLabel = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(xpLabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_make(239, 224, 191));
  lv_obj_align(xpLabel, nullptr, LV_ALIGN_IN_TOP_MID, 0, 89);

  nextLabel = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(nextLabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_make(181, 164, 132));
  lv_obj_align(nextLabel, nullptr, LV_ALIGN_IN_TOP_MID, 0, 111);

  progressBar = lv_bar_create(lv_scr_act(), nullptr);
  lv_obj_set_size(progressBar, 204, 11);
  lv_bar_set_range(progressBar, 0, 100);
  lv_obj_align(progressBar, nullptr, LV_ALIGN_IN_TOP_MID, 0, 139);
  lv_obj_set_style_local_radius(progressBar, LV_BAR_PART_BG, LV_STATE_DEFAULT, 0);
  lv_obj_set_style_local_radius(progressBar, LV_BAR_PART_INDIC, LV_STATE_DEFAULT, 0);
  lv_obj_set_style_local_bg_color(progressBar, LV_BAR_PART_BG, LV_STATE_DEFAULT, lv_color_make(59, 44, 27));
  lv_obj_set_style_local_bg_color(progressBar, LV_BAR_PART_INDIC, LV_STATE_DEFAULT, lv_color_make(192, 139, 56));

  repsLabel = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(repsLabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_make(225, 190, 105));
  lv_obj_align(repsLabel, nullptr, LV_ALIGN_IN_TOP_MID, 0, 157);

  statusLabel = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(statusLabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_make(181, 164, 132));
  lv_obj_align(statusLabel, nullptr, LV_ALIGN_IN_TOP_MID, 0, 180);

  toggleButton = lv_btn_create(lv_scr_act(), nullptr);
  toggleButton->user_data = this;
  lv_obj_set_event_cb(toggleButton, ToggleEvent);
  lv_obj_set_size(toggleButton, 110, 37);
  lv_obj_align(toggleButton, nullptr, LV_ALIGN_IN_BOTTOM_MID, 0, -2);
  lv_obj_set_style_local_radius(toggleButton, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, 0);
  lv_obj_set_style_local_bg_color(toggleButton, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, lv_color_make(73, 57, 38));
  lv_obj_set_style_local_border_width(toggleButton, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, 2);
  lv_obj_set_style_local_border_color(toggleButton, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, lv_color_make(157, 122, 66));
  toggleLabel = lv_label_create(toggleButton, nullptr);

  motionController.StartStrengthWorkout();
  wakeLock.Lock();
  Render();
  refreshTask = lv_task_create(RefreshTaskCallback, 100, LV_TASK_PRIO_MID, this);
}

Strength::~Strength() {
  lv_task_del(refreshTask);
  motionController.StopStrengthWorkout();
  Refresh();
  if (savedXp != xp) {
    SaveProgress();
  }
  lv_obj_clean(lv_scr_act());
}

bool Strength::ReadProgress(const char* path, uint32_t& result) {
  lfs_file_t file;
  if (filesystem.FileOpen(&file, path, LFS_O_RDONLY) != LFS_ERR_OK) {
    return false;
  }
  ProgressRecord record {};
  const int read = filesystem.FileRead(&file, reinterpret_cast<uint8_t*>(&record), sizeof(record));
  filesystem.FileClose(&file);
  if (read != static_cast<int>(sizeof(record)) || record.magic != progressMagic || record.version != progressVersion ||
      record.xp > Controllers::StrengthWorkout::maxXp || record.check != (record.magic ^ record.version ^ record.xp)) {
    return false;
  }
  result = record.xp;
  return true;
}

void Strength::LoadProgress() {
  uint32_t mainXp = 0;
  uint32_t temporaryXp = 0;
  const bool mainValid = ReadProgress(progressPath, mainXp);
  const bool temporaryValid = ReadProgress(temporaryPath, temporaryXp);
  xp = std::max(mainXp, temporaryXp);
  savedXp = mainValid || temporaryValid ? xp : 0;
  if (temporaryValid && temporaryXp > mainXp) {
    filesystem.Rename(temporaryPath, progressPath);
  }
}

bool Strength::SaveProgress() {
  ProgressRecord record {progressMagic, progressVersion, xp, progressMagic ^ progressVersion ^ xp};
  lfs_file_t file;
  if (filesystem.FileOpen(&file, temporaryPath, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC) != LFS_ERR_OK) {
    return false;
  }
  const int written = filesystem.FileWrite(&file, reinterpret_cast<const uint8_t*>(&record), sizeof(record));
  const int closed = filesystem.FileClose(&file);
  if (written != static_cast<int>(sizeof(record)) || closed != LFS_ERR_OK || filesystem.Rename(temporaryPath, progressPath) != LFS_ERR_OK) {
    return false;
  }
  savedXp = xp;
  return true;
}

void Strength::Refresh() {
  const uint32_t currentReps = motionController.StrengthReps();
  if (currentReps > observedReps) {
    const uint32_t newReps = currentReps - observedReps;
    sessionReps += newReps;
    xp = static_cast<uint32_t>(std::min<uint64_t>(Controllers::StrengthWorkout::maxXp,
                                                  static_cast<uint64_t>(xp) +
                                                    static_cast<uint64_t>(newReps) * Controllers::StrengthWorkout::xpPerRep));
    observedReps = currentReps;
  }
  if (xp != savedXp) {
    saveFailed = !SaveProgress();
  }
  Render();
}

void Strength::ToggleWorkout() {
  if (workoutRunning) {
    Refresh();
    motionController.StopStrengthWorkout();
    wakeLock.Release();
    workoutRunning = false;
  } else {
    motionController.StartStrengthWorkout();
    wakeLock.Lock();
    workoutRunning = true;
  }
  Render();
}

void Strength::Render() {
  const uint8_t level = Controllers::StrengthWorkout::Level(xp);
  lv_label_set_text_fmt(levelLabel, "LVL %u", level);
  lv_obj_align(levelLabel, nullptr, LV_ALIGN_IN_TOP_RIGHT, -17, 38);
  lv_label_set_text_fmt(xpLabel, "%lu XP", static_cast<unsigned long>(xp));
  lv_obj_align(xpLabel, nullptr, LV_ALIGN_IN_TOP_MID, 0, 89);

  if (level == 99) {
    lv_label_set_text_static(nextLabel, "MAX LEVEL");
    lv_bar_set_value(progressBar, 100, LV_ANIM_OFF);
  } else {
    const uint32_t nextXp = Controllers::StrengthWorkout::XpForLevel(level + 1);
    const uint32_t previousXp = Controllers::StrengthWorkout::XpForLevel(level);
    lv_label_set_text_fmt(nextLabel, "%lu XP to level %u", static_cast<unsigned long>(nextXp - xp), level + 1);
    lv_bar_set_value(progressBar, 100 * (xp - previousXp) / (nextXp - previousXp), LV_ANIM_OFF);
  }
  lv_obj_align(nextLabel, nullptr, LV_ALIGN_IN_TOP_MID, 0, 111);

  lv_label_set_text_fmt(repsLabel, "Reps %lu   +4 XP", static_cast<unsigned long>(sessionReps));
  lv_obj_align(repsLabel, nullptr, LV_ALIGN_IN_TOP_MID, 0, 157);
  if (saveFailed) {
    lv_label_set_text_static(statusLabel, "Save failed");
  } else if (!workoutRunning) {
    lv_label_set_text_static(statusLabel, "Paused");
  } else if (motionController.StrengthIsCalibrating()) {
    lv_label_set_text_static(statusLabel, "Hold still to calibrate");
  } else {
    lv_label_set_text_static(statusLabel, "Move out and back");
  }
  lv_obj_align(statusLabel, nullptr, LV_ALIGN_IN_TOP_MID, 0, 180);
  lv_label_set_text_static(toggleLabel, workoutRunning ? "Pause" : "Resume");
}
