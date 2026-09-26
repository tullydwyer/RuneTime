#include "displayapp/screens/Strength.h"

#include <algorithm>

#include "components/fs/FS.h"
#include "displayapp/icons/strength/strength.c"
#include "displayapp/RuneUi.h"

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

  const auto skill = RuneUi::StrengthTab();
  levelLabel = skill.level;
  xpLabel = skill.xp;
  nextLabel = skill.next;
  repsLabel = skill.reps;
  statusLabel = skill.status;
  toggleButton = skill.button;
  toggleLabel = skill.buttonText;
  toggleButton->user_data = this;
  lv_obj_set_event_cb(toggleButton, ToggleEvent);

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
  lv_label_set_text_fmt(levelLabel, "%u / 99", level);
  lv_label_set_text_fmt(xpLabel, "%lu", static_cast<unsigned long>(xp));

  if (level == 99) {
    lv_label_set_text_static(nextLabel, "MASTERY ACHIEVED");
  } else {
    const uint32_t nextXp = Controllers::StrengthWorkout::XpForLevel(level + 1);
    lv_label_set_text_fmt(nextLabel, "To %u: %lu XP", level + 1, static_cast<unsigned long>(nextXp - xp));
  }

  lv_label_set_text_fmt(repsLabel, "REPS %lu", static_cast<unsigned long>(sessionReps));
  if (saveFailed) {
    lv_label_set_text_static(statusLabel, "Save failed");
  } else if (!workoutRunning) {
    lv_label_set_text_static(statusLabel, "Paused");
  } else if (motionController.StrengthIsCalibrating()) {
    lv_label_set_text_static(statusLabel, "Calibrating...");
  } else {
    lv_label_set_text_static(statusLabel, "Out + back");
  }
  lv_label_set_text_static(toggleLabel, workoutRunning ? "PAUSE" : "RESUME");
  lv_obj_align(toggleLabel, toggleButton, LV_ALIGN_CENTER, 0, 0);
}
