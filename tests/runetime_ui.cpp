#include <cassert>
#include <cstdio>
#include <initializer_list>
#include "displayapp/icons/strength/strength.c"
#include "displayapp/RuneUi.h"

extern "C" uint32_t xTaskGetTickCount() {
  return 100;
}

static lv_color_t frame[240 * 240];

static void Flush(lv_disp_drv_t* driver, const lv_area_t* area, lv_color_t* pixels) {
  for (int y = area->y1; y <= area->y2; ++y)
    for (int x = area->x1; x <= area->x2; ++x)
      frame[y * 240 + x] = *pixels++;
  lv_disp_flush_ready(driver);
}

static void Save(const char* path) {
  lv_refr_now(nullptr);
  auto* file = fopen(path, "wb");
  assert(file);
  fprintf(file, "P6\n240 240\n255\n");
  for (auto pixel : frame) {
    lv_color32_t rgb;
    rgb.full = lv_color_to32(pixel);
    const unsigned char bytes[] = {rgb.ch.red, rgb.ch.green, rgb.ch.blue};
    fwrite(bytes, 1, 3, file);
  }
  fclose(file);
}

static void Fits(lv_obj_t* obj, int x1, int y1, int x2, int y2) {
  lv_area_t area;
  lv_obj_get_coords(obj, &area);
  if (!(area.x1 >= x1 && area.y1 >= y1 && area.x2 <= x2 && area.y2 <= y2))
    fprintf(stderr, "Bounds %d,%d..%d,%d expected %d,%d..%d,%d\n", area.x1, area.y1, area.x2, area.y2, x1, y1, x2, y2);
  assert(area.x1 >= x1 && area.y1 >= y1 && area.x2 <= x2 && area.y2 <= y2);
}

int main() {
  lv_init();
  static lv_color_t buffer[240 * 20];
  static lv_disp_buf_t displayBuffer;
  lv_disp_buf_init(&displayBuffer, buffer, nullptr, 240 * 20);
  lv_disp_drv_t driver;
  lv_disp_drv_init(&driver);
  driver.hor_res = 240;
  driver.ver_res = 240;
  driver.buffer = &displayBuffer;
  driver.flush_cb = Flush;
  lv_disp_drv_register(&driver);
  lv_theme_set_act(lv_pinetime_theme_init());
  lv_theme_apply(lv_scr_act(), LV_THEME_SCR);

  auto home = RuneUi::Home();
  lv_label_set_text(home.time, "23:59");
  lv_obj_align(home.time, nullptr, LV_ALIGN_IN_TOP_MID, 0, 69);
  lv_label_set_text(home.date, "Wed 30 Sep");
  lv_obj_align(home.date, nullptr, LV_ALIGN_IN_TOP_MID, 0, 130);
  lv_label_set_text(home.heart, "188");
  lv_label_set_text(home.steps, "999999+");
  lv_label_set_text(home.temperature, "-40°F");
  lv_label_set_text(home.weatherIcon, "\xef\x86\x85");
  lv_label_set_text(home.quest, "DAILY QUEST: 72%");
  Fits(home.time, 10, 67, 229, 129);
  Fits(home.date, 10, 130, 229, 151);
  Fits(home.heart, 10, 180, 109, 203);
  Fits(home.steps, 120, 180, 229, 203);
  Fits(home.quest, 8, 212, 231, 232);
  // Render a representative day after exercising the worst-case bounds above.
  lv_label_set_text(home.time, "10:24");
  lv_obj_align(home.time, nullptr, LV_ALIGN_IN_TOP_MID, 0, 69);
  lv_label_set_text(home.date, "Mon 12 Aug");
  lv_obj_align(home.date, nullptr, LV_ALIGN_IN_TOP_MID, 0, 130);
  lv_label_set_text(home.heart, "72");
  lv_label_set_text(home.steps, "7240");
  lv_label_set_text(home.temperature, "21°C");
  Save("home.ppm");
  lv_label_set_text(home.time, "12:00");
  lv_obj_align(home.time, nullptr, LV_ALIGN_IN_TOP_MID, 0, 69);
  lv_label_set_text(home.ampm, "AM");
  lv_label_set_text(home.heart, "--");
  lv_label_set_text(home.quest, "QUEST COMPLETE!");
  Fits(home.ampm, 200, 30, 239, 60);
  Save("home-complete.ppm");
  lv_obj_clean(lv_scr_act());

  namespace Symbols = Pinetime::Applications::Screens::Symbols;
  RuneUi::Text("INVENTORY", 8, 32, Colors::gold, &jetbrains_mono_bold_20);
  RuneUi::Text("1/3", 192, 35, Colors::lightGray);
  RuneUi::Text("12:34", 8, 5);
  const char* map[] = {" ", " ", " ", "\n", " ", " ", " ", ""};
  auto* inventory = RuneUi::Inventory(map);
  const char* captions[] = {"Melee", "Chrono", "Alarm", "Timer", "Steps", "Health"};
  const char* icons[] = {Symbols::heartBeat, Symbols::stopWatch, Symbols::bell, Symbols::hourGlass, Symbols::shoe, Symbols::heartBeat};
  const lv_color_t colors[] = {Colors::deepOrange, Colors::gold, Colors::deepOrange, Colors::gold, Colors::green, Colors::deepOrange};
  for (unsigned i = 0; i < 6; ++i) {
    auto* icon = RuneUi::InventoryIcon(inventory, i, icons[i], colors[i]);
    auto* caption = RuneUi::InventoryCaption(inventory, i, captions[i], true);
    const auto area = static_cast<lv_btnmatrix_ext_t*>(lv_obj_get_ext_attr(inventory))->button_areas[i];
    Fits(caption, 6 + area.x1, 62 + area.y1, 6 + area.x2, 62 + area.y2);
    Fits(icon, 6 + area.x1, 62 + area.y1, 6 + area.x2, 62 + area.y2);
    assert(!lv_obj_get_click(caption) && !lv_obj_get_click(icon));
    assert(area.x2 - area.x1 >= 44 && area.y2 - area.y1 >= 44);
  }
  Save("inventory.ppm");
  lv_obj_clean(lv_scr_act());

  RuneUi::StoneBackdrop();
  RuneUi::Text("SETTINGS", 10, 5, Colors::gold, &jetbrains_mono_bold_20);
  auto* list = lv_cont_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_bg_opa(list, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_TRANSP);
  lv_obj_set_style_local_border_width(list, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 0);
  lv_obj_set_style_local_pad_inner(list, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 4);
  lv_obj_set_pos(list, 4, 34);
  lv_obj_set_size(list, 228, 206);
  lv_cont_set_layout(list, LV_LAYOUT_COLUMN_LEFT);
  const char* settingNames[] = {"Display", "Wake Up", "Time format", "Watch face"};
  const char* settingIcons[] = {Symbols::sun, Symbols::eye, Symbols::clock, Symbols::home};
  for (int i = 0; i < 4; ++i) {
    auto* row = RuneUi::SettingsRow(list, settingIcons[i], settingNames[i], 228, 47);
    Fits(row, 4, 34, 231, 239);
  }
  Save("settings.ppm");
  lv_obj_clean(lv_scr_act());

  const char* utilities[] = {Symbols::brightnessHigh, Symbols::flashlight, Symbols::notificationsOn, Symbols::settings};
  const char* utilityCaptions[] = {"LIGHT", "TORCH", "CHAT", "SETTINGS"};
  for (int i = 0; i < 4; ++i) {
    auto* button = lv_btn_create(lv_scr_act(), nullptr);
    lv_obj_set_size(button, 115, 100);
    lv_obj_set_style_local_bg_color(button, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, Colors::scroll);
    lv_obj_set_style_local_border_width(button, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, 2);
    lv_obj_set_style_local_border_color(button, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, Colors::stoneShadow);
    lv_obj_set_pos(button, (i % 2) * 125, 30 + (i / 2) * 110);
    RuneUi::SkinUtilityButton(button);
    auto* icon = lv_label_create(button, nullptr);
    lv_obj_set_style_local_text_font(icon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_sys_48);
    lv_label_set_text(icon, utilities[i]);
    RuneUi::ButtonCaption(button, icon, utilityCaptions[i]);
    auto* caption = lv_obj_get_child(button, nullptr);
    const int x = (i % 2) * 125, y = 30 + (i / 2) * 110;
    Fits(icon, x, y, x + 114, y + 99);
    Fits(caption, x, y, x + 114, y + 99);
  }
  Save("quick-settings.ppm");
  lv_obj_clean(lv_scr_act());

  RuneUi::Text("CONTROL PANEL", 10, 8, Colors::gold, &jetbrains_mono_bold_20);
  const char* names[] = {"Normal", "Pressed", "Active", "Disabled"};
  lv_state_t states[] = {LV_STATE_DEFAULT, LV_STATE_PRESSED, LV_STATE_CHECKED, LV_STATE_DISABLED};
  for (int i = 0; i < 4; ++i) {
    auto* button = lv_btn_create(lv_scr_act(), nullptr);
    lv_obj_set_pos(button, 10, 42 + i * 47);
    lv_obj_set_size(button, 220, 41);
    lv_obj_set_state(button, states[i]);
    auto* label = lv_label_create(button, nullptr);
    lv_label_set_text(label, names[i]);
  }
  Save("controls.ppm");
  lv_obj_clean(lv_scr_act());

  auto skill = RuneUi::StrengthTab();
  lv_label_set_text(skill.level, "99 / 99");
  lv_label_set_text(skill.xp, "200000000");
  lv_label_set_text(skill.next, "MASTERY ACHIEVED");
  lv_label_set_text(skill.reps, "SET 999 REPS");
  lv_label_set_text(skill.status, "Adjust reps, then save");
  lv_label_set_text(skill.buttonText, "SAVE");
  lv_label_set_text(skill.plusText, "+");
  lv_obj_set_hidden(skill.minus, false);
  lv_obj_align(skill.buttonText, skill.button, LV_ALIGN_CENTER, 0, 0);
  lv_obj_align(skill.plusText, skill.plus, LV_ALIGN_CENTER, 0, 0);
  Fits(skill.level, 90, 87, 226, 122);
  Fits(skill.xp, 110, 122, 225, 144);
  Fits(skill.next, 12, 142, 226, 164);
  Fits(skill.reps, 11, 180, 146, 201);
  Fits(skill.status, 11, 160, 237, 184);
  Fits(skill.button, 160, 200, 239, 239);
  Fits(skill.buttonText, 161, 201, 237, 239);
  for (auto* button : {skill.minus, skill.plus, skill.button}) {
    assert(lv_obj_get_click(button));
    assert(lv_obj_get_width(button) >= 38 && lv_obj_get_height(button) >= 38);
  }
  Save("strength-review.ppm");
  lv_label_set_text(skill.status, "Save failed: retrying");
  Fits(skill.status, 11, 160, 237, 184);
  lv_label_set_text(skill.level, "42 / 99");
  lv_label_set_text(skill.xp, "43222");
  lv_label_set_text(skill.next, "To 43: 2307 XP");
  lv_label_set_text(skill.reps, "SET 7 REPS");
  lv_label_set_text(skill.status, "Hold still briefly");
  lv_label_set_text(skill.buttonText, "FINISH");
  lv_obj_set_hidden(skill.minus, true);
  lv_obj_set_hidden(skill.plus, true);
  lv_obj_align(skill.buttonText, skill.button, LV_ALIGN_CENTER, 0, 0);
  Fits(skill.buttonText, 161, 201, 237, 239);
  Save("strength.ppm");
  lv_label_set_text(skill.status, "Start or log any lift");
  lv_label_set_text(skill.buttonText, "START");
  lv_label_set_text(skill.plusText, "LOG");
  lv_obj_set_hidden(skill.plus, false);
  lv_obj_align(skill.plusText, skill.plus, LV_ALIGN_CENTER, 0, 0);
  lv_obj_align(skill.buttonText, skill.button, LV_ALIGN_CENTER, 0, 0);
  Save("strength-ready.ppm");
  // Reinitializing the theme is supported and must not corrupt live styles.
  lv_pinetime_theme_init();
  lv_obj_clean(lv_scr_act());
  puts("Home/inventory/settings/strength bounds, long values, quest states, theme reset: PASS");
}
