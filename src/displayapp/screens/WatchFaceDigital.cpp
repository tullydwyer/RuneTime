#include "displayapp/screens/WatchFaceDigital.h"

#include <lvgl/lvgl.h>
#include <cstdio>
#include <algorithm>
#include "displayapp/RuneUi.h"

#include "displayapp/screens/NotificationIcon.h"
#include "displayapp/screens/Symbols.h"
#include "displayapp/screens/WeatherSymbols.h"
#include "components/battery/BatteryController.h"
#include "components/ble/BleController.h"
#include "components/ble/NotificationManager.h"
#include "components/heartrate/HeartRateController.h"
#include "components/motion/MotionController.h"
#include "components/ble/SimpleWeatherService.h"
#include "components/settings/Settings.h"

using namespace Pinetime::Applications::Screens;

WatchFaceDigital::WatchFaceDigital(Controllers::DateTime& dateTimeController,
                                   const Controllers::Battery& batteryController,
                                   const Controllers::Ble& bleController,
                                   const Controllers::AlarmController& alarmController,
                                   Controllers::NotificationManager& notificationManager,
                                   Controllers::Settings& settingsController,
                                   Controllers::HeartRateController& heartRateController,
                                   Controllers::MotionController& motionController,
                                   Controllers::SimpleWeatherService& weatherService)
  : currentDateTime {{}},
    dateTimeController {dateTimeController},
    notificationManager {notificationManager},
    settingsController {settingsController},
    heartRateController {heartRateController},
    motionController {motionController},
    weatherService {weatherService},
    statusIcons(batteryController, bleController, alarmController) {

  const auto face = RuneUi::Home();
  label_time = face.time;
  label_time_ampm = face.ampm;
  label_date = face.date;
  heartbeatIcon = face.heartIcon;
  heartbeatValue = face.heart;
  stepIcon = face.stepIcon;
  stepValue = face.steps;
  notificationIcon = face.notification;
  weatherIcon = face.weatherIcon;
  temperature = face.temperature;
  questLabel = face.quest;
  statusIcons.Create();
  lv_obj_align(statusIcons.GetObject(), nullptr, LV_ALIGN_IN_TOP_RIGHT, -8, 4);

  taskRefresh = lv_task_create(RefreshTaskCallback, LV_DISP_DEF_REFR_PERIOD, LV_TASK_PRIO_MID, this);
  Refresh();
}

WatchFaceDigital::~WatchFaceDigital() {
  lv_task_del(taskRefresh);
  lv_obj_clean(lv_scr_act());
}

void WatchFaceDigital::Refresh() {
  statusIcons.Update();

  notificationState = notificationManager.AreNewNotificationsAvailable();
  if (notificationState.IsUpdated()) {
    lv_label_set_text_static(notificationIcon, NotificationIcon::GetIcon(notificationState.Get()));
  }

  currentDateTime = std::chrono::time_point_cast<std::chrono::minutes>(dateTimeController.CurrentDateTime());

  if (currentDateTime.IsUpdated()) {
    uint8_t hour = dateTimeController.Hours();
    uint8_t minute = dateTimeController.Minutes();

    if (settingsController.GetClockType() == Controllers::Settings::ClockType::H12) {
      char ampmChar[3] = "AM";
      if (hour == 0) {
        hour = 12;
      } else if (hour == 12) {
        ampmChar[0] = 'P';
      } else if (hour > 12) {
        hour = hour - 12;
        ampmChar[0] = 'P';
      }
      lv_label_set_text(label_time_ampm, ampmChar);
      lv_label_set_text_fmt(label_time, "%02d:%02d", hour, minute);
      lv_obj_align(label_time, nullptr, LV_ALIGN_IN_TOP_MID, 0, 69);
    } else {
      lv_label_set_text_fmt(label_time, "%02d:%02d", hour, minute);
      lv_obj_align(label_time, nullptr, LV_ALIGN_IN_TOP_MID, 0, 69);
    }

    currentDate = std::chrono::time_point_cast<std::chrono::days>(currentDateTime.Get());
    if (currentDate.IsUpdated()) {
      uint8_t day = dateTimeController.Day();
      if (settingsController.GetClockType() == Controllers::Settings::ClockType::H24) {
        lv_label_set_text_fmt(label_date,
                              "%s %d %s",
                              dateTimeController.DayOfWeekShortToString(),
                              day,
                              dateTimeController.MonthShortToString());
      } else {
        lv_label_set_text_fmt(label_date,
                              "%s %s %d",
                              dateTimeController.DayOfWeekShortToString(),
                              dateTimeController.MonthShortToString(),
                              day);
      }
      lv_obj_align(label_date, nullptr, LV_ALIGN_IN_TOP_MID, 0, 130);
    }
  }

  heartbeat = heartRateController.HeartRate();
  heartbeatRunning = heartRateController.State() != Controllers::HeartRateController::States::Stopped;
  if (heartbeat.IsUpdated() || heartbeatRunning.IsUpdated()) {
    if (heartbeatRunning.Get()) {
      lv_obj_set_style_local_text_color(heartbeatIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, Colors::deepOrange);
      lv_label_set_text_fmt(heartbeatValue, "%d", heartbeat.Get());
    } else {
      lv_obj_set_style_local_text_color(heartbeatIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, Colors::gray);
      lv_label_set_text_static(heartbeatValue, "--");
    }

    lv_obj_realign(heartbeatIcon);
    lv_obj_realign(heartbeatValue);
  }

  stepCount = motionController.NbSteps();
  if (stepCount.IsUpdated()) {
    if (stepCount.Get() > 999999) {
      lv_label_set_text_static(stepValue, "999999+");
    } else {
      lv_label_set_text_fmt(stepValue, "%lu", static_cast<unsigned long>(stepCount.Get()));
    }
    const auto goal = std::max<uint32_t>(1, settingsController.GetStepsGoal());
    const auto progress = std::min<uint64_t>(100, static_cast<uint64_t>(stepCount.Get()) * 100 / goal);
    if (progress == 100) {
      lv_label_set_text_static(questLabel, "QUEST COMPLETE!");
    } else {
      lv_label_set_text_fmt(questLabel, "DAILY QUEST: %lu%%", static_cast<unsigned long>(progress));
    }
  }

  currentWeather = weatherService.Current();
  if (currentWeather.IsUpdated()) {
    auto optCurrentWeather = currentWeather.Get();
    if (optCurrentWeather) {
      int16_t temp = optCurrentWeather->temperature.Celsius();
      char tempUnit = 'C';
      if (settingsController.GetWeatherFormat() == Controllers::Settings::WeatherFormat::Imperial) {
        temp = optCurrentWeather->temperature.Fahrenheit();
        tempUnit = 'F';
      }
      lv_label_set_text_fmt(temperature, "%d°%c", temp, tempUnit);
      lv_label_set_text(weatherIcon, Symbols::GetSymbol(optCurrentWeather->iconId, weatherService.IsNight()));
    } else {
      lv_label_set_text_static(temperature, "");
      lv_label_set_text(weatherIcon, "");
    }
    lv_obj_realign(temperature);
    lv_obj_realign(weatherIcon);
  }
}
