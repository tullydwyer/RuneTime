#include "displayapp/screens/List.h"
#include "displayapp/DisplayApp.h"
#include "displayapp/screens/Symbols.h"
#include "displayapp/InfiniTimeTheme.h"
#include "displayapp/RuneUi.h"

using namespace Pinetime::Applications::Screens;

namespace {
  void ButtonEventHandler(lv_obj_t* obj, lv_event_t event) {
    auto* screen = static_cast<List*>(obj->user_data);
    screen->OnButtonEvent(obj, event);
  }
}

List::List(uint8_t screenID,
           uint8_t numScreens,
           DisplayApp* app,
           Controllers::Settings& settingsController,
           std::array<Applications, MAXLISTITEMS>& applications)
  : app {app}, settingsController {settingsController}, pageIndicator(screenID, numScreens) {

  RuneUi::StoneBackdrop();
  RuneUi::Text("SETTINGS", 10, 5, Colors::gold, &jetbrains_mono_bold_20);

  settingsController.SetSettingsMenu(screenID);

  pageIndicator.Create();

  lv_obj_t* container = lv_cont_create(lv_scr_act(), nullptr);

  lv_obj_set_style_local_bg_opa(container, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_TRANSP);
  static constexpr int innerPad = 4;
  lv_obj_set_style_local_pad_inner(container, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, innerPad);
  lv_obj_set_style_local_border_width(container, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 0);

  lv_obj_set_pos(container, 4, 34);
  lv_obj_set_width(container, LV_HOR_RES - 12);
  lv_obj_set_height(container, LV_VER_RES - 34);
  lv_cont_set_layout(container, LV_LAYOUT_COLUMN_LEFT);

  for (int i = 0; i < MAXLISTITEMS; i++) {
    apps[i] = applications[i].application;
    if (applications[i].application != Apps::None) {

      static constexpr int btnHeight = (LV_VER_RES_MAX - 38 - ((MAXLISTITEMS - 1) * innerPad)) / MAXLISTITEMS;
      itemApps[i] = RuneUi::SettingsRow(container, applications[i].icon, applications[i].name, LV_HOR_RES - 12, btnHeight);
      lv_obj_set_event_cb(itemApps[i], ButtonEventHandler);
      itemApps[i]->user_data = this;
    }
  }
}

List::~List() {
  lv_obj_clean(lv_scr_act());
}

void List::OnButtonEvent(lv_obj_t* object, lv_event_t event) {
  if (event == LV_EVENT_CLICKED) {
    for (int i = 0; i < MAXLISTITEMS; i++) {
      if (apps[i] != Apps::None && object == itemApps[i]) {
        app->StartApp(apps[i], DisplayApp::FullRefreshDirections::Up);
        running = false;
        return;
      }
    }
  }
}
