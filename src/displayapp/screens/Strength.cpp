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

  void MinusEvent(lv_obj_t* object, lv_event_t event) {
    if (event == LV_EVENT_CLICKED || event == LV_EVENT_LONG_PRESSED_REPEAT) {
      static_cast<Strength*>(object->user_data)->AdjustReps(-1);
    }
  }

  void PlusEvent(lv_obj_t* object, lv_event_t event) {
    auto* screen = static_cast<Strength*>(object->user_data);
    if (event == LV_EVENT_CLICKED) {
      screen->LogSet();
    } else if (event == LV_EVENT_LONG_PRESSED_REPEAT) {
      screen->AdjustReps(1);
    }
  }
}

Strength::Strength(Controllers::MotionController& motionController, Controllers::FS& filesystem, System::SystemTask& systemTask)
  : motionController {motionController}, filesystem {filesystem}, wakeLock {systemTask} {
  LoadProgress();

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

  minusButton = skill.minus;
  plusButton = skill.plus;
  plusLabel = skill.plusText;
  minusButton->user_data = this;
  plusButton->user_data = this;
  lv_obj_set_event_cb(minusButton, MinusEvent);
  lv_obj_set_event_cb(plusButton, PlusEvent);
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
  set.Observe(motionController.StrengthReps());
  if (xp != savedXp) {
    saveFailed = !SaveProgress();
  }
  Render();
}

void Strength::ToggleWorkout() {
  using State = Controllers::StrengthSet::State;
  switch (set.CurrentState()) {
    case State::Ready:
      set.Start(motionController.StrengthReps());
      motionController.StartStrengthWorkout();
      wakeLock.Lock();
      break;
    case State::Counting:
      set.Observe(motionController.StrengthReps());
      motionController.StopStrengthWorkout();
      set.Finish();
      wakeLock.Release();
      break;
    case State::Review:
      xp = set.Confirm(xp);
      // Refresh retries failed writes without awarding the set a second time.
      Refresh();
      break;
  }
  Render();
}

void Strength::AdjustReps(int delta) {
  set.Adjust(delta);
  Render();
}

void Strength::LogSet() {
  if (set.CurrentState() == Controllers::StrengthSet::State::Ready) {
    set.LogManually();
  } else {
    set.Adjust(1);
  }
  Render();
}

bool Strength::OnButtonPushed() {
  // A button press while lifting finishes the estimate for review, rather than
  // silently abandoning the set when the display app navigates away.
  if (set.CurrentState() == Controllers::StrengthSet::State::Counting) {
    ToggleWorkout();
    return true;
  }
  return false;
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

  using State = Controllers::StrengthSet::State;
  const auto state = set.CurrentState();
  lv_label_set_text_fmt(repsLabel, "%s %u REPS", state == State::Ready ? "LAST" : "SET", set.Reps());
  if (saveFailed) {
    lv_label_set_text_static(statusLabel, "Save failed: retrying");
  } else if (state == State::Ready) {
    lv_label_set_text_static(statusLabel, "Start or log any lift");
  } else if (state == State::Review) {
    lv_label_set_text_static(statusLabel, "Adjust reps, then save");
  } else if (motionController.StrengthIsCalibrating()) {
    lv_label_set_text_static(statusLabel, "Hold still briefly");
  } else {
    lv_label_set_text_static(statusLabel, "Auto estimate");
  }
  lv_label_set_text_static(toggleLabel, state == State::Ready ? "START" : (state == State::Counting ? "FINISH" : "SAVE"));
  lv_obj_align(toggleLabel, toggleButton, LV_ALIGN_CENTER, 0, 0);
  lv_label_set_text_static(plusLabel, state == State::Ready ? "LOG" : "+");
  lv_obj_align(plusLabel, plusButton, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_hidden(minusButton, state != State::Review);
  lv_obj_set_hidden(plusButton, state == State::Counting);
}
