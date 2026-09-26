#include "displayapp/screens/Tile.h"
#include "displayapp/screens/BatteryIcon.h"
#include "components/ble/BleController.h"
#include "displayapp/InfiniTimeTheme.h"
#include "displayapp/RuneUi.h"

LV_IMG_DECLARE(strength_icon)

using namespace Pinetime::Applications::Screens;

namespace {
  lv_color_t InventoryColor(Pinetime::Applications::Apps app) {
    using Pinetime::Applications::Apps;
    switch (app) {
      case Apps::Steps:
      case Apps::Navigation:
      case Apps::Weather:
        return Colors::green;
      case Apps::HeartRate:
      case Apps::Alarm:
        return Colors::deepOrange;
      case Apps::Music:
      case Apps::Paint:
      case Apps::Twos:
        return Colors::blue;
      default:
        return Colors::gold;
    }
  }

  const char* InventoryName(Pinetime::Applications::Apps app) {
    using Pinetime::Applications::Apps;
    switch (app) {
      case Apps::Strength:
        return "Melee";
      case Apps::StopWatch:
        return "Chrono";
      case Apps::Alarm:
        return "Alarm";
      case Apps::Timer:
        return "Timer";
      case Apps::Steps:
        return "Steps";
      case Apps::HeartRate:
        return "Health";
      case Apps::Music:
        return "Bard";
      case Apps::Paint:
        return "Paint";
      case Apps::Paddle:
        return "Paddle";
      case Apps::Twos:
        return "Runes";
      case Apps::Dice:
        return "Dice";
      case Apps::Metronome:
        return "Tick";
      case Apps::Navigation:
        return "Travel";
      case Apps::Calculator:
        return "Maths";
      case Apps::Weather:
        return "Skies";
      case Apps::Motion:
        return "Motion";
      default:
        return "";
    }
  }

  void lv_update_task(struct _lv_task_t* task) {
    auto* user_data = static_cast<Tile*>(task->user_data);
    user_data->UpdateScreen();
  }

  void event_handler(lv_obj_t* obj, lv_event_t event) {
    if (event != LV_EVENT_VALUE_CHANGED) {
      return;
    }

    Tile* screen = static_cast<Tile*>(obj->user_data);
    auto* eventDataPtr = (uint32_t*) lv_event_get_data();
    uint32_t eventData = *eventDataPtr;
    screen->OnValueChangedEvent(obj, eventData);
  }
}

Tile::Tile(uint8_t screenID,
           uint8_t numScreens,
           DisplayApp* app,
           Controllers::Settings& settingsController,
           const Controllers::Battery& batteryController,
           const Controllers::Ble& bleController,
           const Controllers::AlarmController& alarmController,
           Controllers::DateTime& dateTimeController,
           std::array<Applications, 6>& applications)
  : app {app},
    dateTimeController {dateTimeController},
    pageIndicator(screenID, numScreens),
    statusIcons(batteryController, bleController, alarmController) {

  settingsController.SetAppMenu(screenID);

  statusIcons.Create();
  lv_obj_align(statusIcons.GetObject(), lv_scr_act(), LV_ALIGN_IN_TOP_RIGHT, -8, 4);

  // Time
  label_time = RuneUi::Text("", 8, 5, Colors::parchment);
  lv_label_set_align(label_time, LV_LABEL_ALIGN_CENTER);
  lv_obj_align(label_time, nullptr, LV_ALIGN_IN_TOP_LEFT, 8, 5);

  RuneUi::Text("INVENTORY", 8, 32, Colors::gold, &jetbrains_mono_bold_20);
  auto* page = RuneUi::Text("", 192, 35, Colors::lightGray);
  lv_label_set_text_fmt(page, "%u/%u", screenID + 1, numScreens);
  pageIndicator.Create();

  uint8_t btIndex = 0;
  for (uint8_t i = 0; i < 6; i++) {
    if (i == 3) {
      btnmMap[btIndex++] = "\n";
    }
    btnmMap[btIndex] = " ";
    btIndex++;
    apps[i] = applications[i].application;
  }
  btnmMap[btIndex] = "";

  btnm1 = RuneUi::Inventory(btnmMap);

  for (uint8_t i = 0; i < 6; i++) {
    lv_btnmatrix_set_btn_ctrl(btnm1, i, LV_BTNMATRIX_CTRL_CLICK_TRIG);
    if (applications[i].application == Apps::None || !applications[i].enabled) {
      lv_btnmatrix_set_btn_ctrl(btnm1, i, LV_BTNMATRIX_CTRL_DISABLED);
    }
  }

  auto* matrixData = static_cast<lv_btnmatrix_ext_t*>(lv_obj_get_ext_attr(btnm1));
  for (uint8_t i = 0; i < 6; i++) {
    const lv_area_t& button = matrixData->button_areas[i];
    if (apps[i] == Apps::None) {
      continue;
    }
    RuneUi::InventoryCaption(btnm1, i, InventoryName(apps[i]), applications[i].enabled);
    if (apps[i] != Apps::Strength) {
      RuneUi::InventoryIcon(btnm1, i, applications[i].icon, applications[i].enabled ? InventoryColor(apps[i]) : Colors::gray);
    }
    if (apps[i] != Apps::Strength) {
      continue;
    }
    lv_obj_t* icon = lv_img_create(lv_scr_act(), nullptr);
    lv_img_set_src(icon, &strength_icon);
    lv_img_set_zoom(icon, 128);
    lv_img_set_antialias(icon, false);
    lv_obj_align(icon, btnm1, LV_ALIGN_IN_TOP_LEFT, (button.x1 + button.x2 - 48) / 2, (button.y1 + button.y2 - 48) / 2 - 10);
    lv_obj_set_click(icon, false);
  }

  btnm1->user_data = this;
  lv_obj_set_event_cb(btnm1, event_handler);

  taskUpdate = lv_task_create(lv_update_task, 5000, LV_TASK_PRIO_MID, this);

  UpdateScreen();
}

Tile::~Tile() {
  lv_task_del(taskUpdate);
  lv_obj_clean(lv_scr_act());
}

void Tile::UpdateScreen() {
  lv_label_set_text(label_time, dateTimeController.FormattedTime().c_str());
  statusIcons.Update();
}

void Tile::OnValueChangedEvent(lv_obj_t* obj, uint32_t buttonId) {
  if (obj != btnm1 || buttonId >= 6 || apps[buttonId] == Apps::None) {
    return;
  }

  app->StartApp(apps[buttonId], DisplayApp::FullRefreshDirections::Up);
  running = false;
}
