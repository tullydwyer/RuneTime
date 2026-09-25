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
  const lv_point_t levelDivider[] = {{0, 66}, {68, 0}};

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
  lv_obj_set_style_local_bg_color(background, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, lv_color_make(47, 45, 39));

  lv_obj_t* panelShadow = lv_obj_create(lv_scr_act(), nullptr);
  lv_obj_set_size(panelShadow, 228, 112);
  lv_obj_align(panelShadow, nullptr, LV_ALIGN_IN_TOP_MID, 0, 3);
  lv_obj_set_click(panelShadow, false);
  lv_obj_set_style_local_radius(panelShadow, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 6);
  lv_obj_set_style_local_border_width(panelShadow, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 0);
  lv_obj_set_style_local_bg_color(panelShadow, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, lv_color_make(33, 32, 28));

  statPanel = lv_obj_create(lv_scr_act(), nullptr);
  lv_obj_set_size(statPanel, 222, 106);
  lv_obj_align(statPanel, nullptr, LV_ALIGN_IN_TOP_MID, 0, 6);
  lv_obj_set_click(statPanel, false);
  lv_obj_set_style_local_radius(statPanel, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 5);
  lv_obj_set_style_local_border_width(statPanel, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 4);
  lv_obj_set_style_local_border_color(statPanel, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, lv_color_make(102, 99, 87));
  lv_obj_set_style_local_bg_color(statPanel, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, lv_color_make(80, 81, 77));

  lv_obj_t* icon = lv_img_create(lv_scr_act(), nullptr);
  lv_img_set_src(icon, &strength_icon);
  lv_img_set_zoom(icon, 384);
  lv_img_set_antialias(icon, false);
  lv_obj_align(icon, statPanel, LV_ALIGN_IN_LEFT_MID, 16, 0);
  lv_obj_set_click(icon, false);

  lv_obj_t* divider = lv_line_create(lv_scr_act(), nullptr);
  lv_line_set_points(divider, levelDivider, 2);
  lv_obj_set_style_local_line_width(divider, LV_LINE_PART_MAIN, LV_STATE_DEFAULT, 3);
  lv_obj_set_style_local_line_color(divider, LV_LINE_PART_MAIN, LV_STATE_DEFAULT, lv_color_make(18, 18, 16));
  lv_obj_align(divider, statPanel, LV_ALIGN_IN_TOP_LEFT, 121, 18);
  lv_obj_set_click(divider, false);

  levelLabel = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_font(levelLabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_42);
  lv_obj_set_style_local_text_color(levelLabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_make(215, 220, 67));

  baseLevelLabel = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_font(baseLevelLabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_42);
  lv_obj_set_style_local_text_color(baseLevelLabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_make(215, 220, 67));

  xpLabel = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(xpLabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_make(232, 227, 201));
  lv_obj_align(xpLabel, nullptr, LV_ALIGN_IN_TOP_MID, 0, 115);

  nextLabel = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(nextLabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_make(185, 183, 154));
  lv_obj_align(nextLabel, nullptr, LV_ALIGN_IN_TOP_MID, 0, 136);

  progressBar = lv_bar_create(lv_scr_act(), nullptr);
  lv_obj_set_size(progressBar, 204, 9);
  lv_bar_set_range(progressBar, 0, 100);
  lv_obj_align(progressBar, nullptr, LV_ALIGN_IN_TOP_MID, 0, 158);
  lv_obj_set_style_local_radius(progressBar, LV_BAR_PART_BG, LV_STATE_DEFAULT, 0);
  lv_obj_set_style_local_radius(progressBar, LV_BAR_PART_INDIC, LV_STATE_DEFAULT, 0);
  lv_obj_set_style_local_bg_color(progressBar, LV_BAR_PART_BG, LV_STATE_DEFAULT, lv_color_make(31, 31, 27));
  lv_obj_set_style_local_bg_color(progressBar, LV_BAR_PART_INDIC, LV_STATE_DEFAULT, lv_color_make(173, 179, 61));

  repsLabel = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(repsLabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_make(216, 220, 104));
  lv_obj_align(repsLabel, nullptr, LV_ALIGN_IN_TOP_MID, 0, 169);

  statusLabel = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(statusLabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_make(185, 183, 154));
  lv_obj_align(statusLabel, nullptr, LV_ALIGN_IN_TOP_MID, 0, 188);

  toggleButton = lv_btn_create(lv_scr_act(), nullptr);
  toggleButton->user_data = this;
  lv_obj_set_event_cb(toggleButton, ToggleEvent);
  lv_obj_set_size(toggleButton, 110, 29);
  lv_obj_align(toggleButton, nullptr, LV_ALIGN_IN_BOTTOM_MID, 0, -1);
  lv_obj_set_style_local_radius(toggleButton, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, 3);
  lv_obj_set_style_local_bg_color(toggleButton, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, lv_color_make(78, 78, 70));
  lv_obj_set_style_local_border_width(toggleButton, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, 2);
  lv_obj_set_style_local_border_color(toggleButton, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, lv_color_make(137, 132, 112));
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
  lv_label_set_text_fmt(levelLabel, "%u", level);
  lv_obj_align(levelLabel, statPanel, LV_ALIGN_IN_TOP_RIGHT, -56, 0);
  lv_label_set_text_fmt(baseLevelLabel, "%u", level);
  lv_obj_align(baseLevelLabel, statPanel, LV_ALIGN_IN_BOTTOM_RIGHT, -9, -1);
  lv_label_set_text_fmt(xpLabel, "%lu XP", static_cast<unsigned long>(xp));
  lv_obj_align(xpLabel, nullptr, LV_ALIGN_IN_TOP_MID, 0, 115);

  if (level == 99) {
    lv_label_set_text_static(nextLabel, "MAX LEVEL");
    lv_bar_set_value(progressBar, 100, LV_ANIM_OFF);
  } else {
    const uint32_t nextXp = Controllers::StrengthWorkout::XpForLevel(level + 1);
    const uint32_t previousXp = Controllers::StrengthWorkout::XpForLevel(level);
    lv_label_set_text_fmt(nextLabel, "To %u: %lu XP", level + 1, static_cast<unsigned long>(nextXp - xp));
    lv_bar_set_value(progressBar, 100 * (xp - previousXp) / (nextXp - previousXp), LV_ANIM_OFF);
  }
  lv_obj_align(nextLabel, nullptr, LV_ALIGN_IN_TOP_MID, 0, 136);

  lv_label_set_text_fmt(repsLabel, "Reps %lu   +4 XP", static_cast<unsigned long>(sessionReps));
  lv_obj_align(repsLabel, nullptr, LV_ALIGN_IN_TOP_MID, 0, 169);
  if (saveFailed) {
    lv_label_set_text_static(statusLabel, "Save failed");
  } else if (!workoutRunning) {
    lv_label_set_text_static(statusLabel, "Paused");
  } else if (motionController.StrengthIsCalibrating()) {
    lv_label_set_text_static(statusLabel, "Calibrating...");
  } else {
    lv_label_set_text_static(statusLabel, "Move out and back");
  }
  lv_obj_align(statusLabel, nullptr, LV_ALIGN_IN_TOP_MID, 0, 188);
  lv_label_set_text_static(toggleLabel, workoutRunning ? "Pause" : "Resume");
}
