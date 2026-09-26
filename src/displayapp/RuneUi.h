#pragma once

#include "displayapp/InfiniTimeTheme.h"
#include "displayapp/screens/Symbols.h"

LV_IMG_DECLARE(rune_scroll_clock);
LV_IMG_DECLARE(rune_scroll_skill_left);
LV_IMG_DECLARE(rune_scroll_skill_right);
LV_IMG_DECLARE(rune_scroll_quest);
LV_IMG_DECLARE(rune_scroll_slot);
LV_IMG_DECLARE(rune_scroll_row);
LV_IMG_DECLARE(rune_scroll_utility);

// Hand-drawn indexed sprites retain the stepped, irregular edges of 2004 UI art.
namespace RuneUi {
  inline void StoneBackdrop() {
    lv_obj_set_style_local_bg_color(lv_scr_act(), LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, Colors::stone);
  }

  inline lv_obj_t* Sprite(const lv_img_dsc_t* image, int x, int y) {
    auto* sprite = lv_img_create(lv_scr_act(), nullptr);
    lv_img_set_src(sprite, image);
    lv_obj_set_pos(sprite, x, y);
    lv_obj_set_click(sprite, false);
    return sprite;
  }

  inline lv_obj_t* Text(const char* text, int x, int y, lv_color_t color = Colors::parchment, const lv_font_t* font = &rune_small) {
    auto* label = lv_label_create(lv_scr_act(), nullptr);
    lv_label_set_text(label, text);
    lv_obj_set_style_local_text_font(label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, font);
    lv_obj_set_style_local_text_color(label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color);
    lv_obj_align(label, nullptr, LV_ALIGN_IN_TOP_LEFT, x, y);
    lv_obj_set_click(label, false);
    return label;
  }

  inline void HomeFrame() {
    StoneBackdrop();
    auto* title = Text("RUNE TIME", 0, 31, Colors::gold, &jetbrains_mono_bold_20);
    lv_obj_align(title, nullptr, LV_ALIGN_IN_TOP_MID, 0, 31);
    Sprite(&rune_scroll_clock, 5, 59);
    Sprite(&rune_scroll_skill_left, 5, 158);
    Sprite(&rune_scroll_skill_right, 118, 158);
    Sprite(&rune_scroll_quest, 5, 208);
    Text("HITPOINTS", 13, 166, Colors::ink);
    Text("AGILITY", 126, 166, Colors::ink);
  }

  inline lv_obj_t* Inventory(const char** map) {
    StoneBackdrop();
    auto* matrix = lv_btnmatrix_create(lv_scr_act(), nullptr);
    lv_btnmatrix_set_map(matrix, map);
    lv_obj_set_size(matrix, 224, 164);
    lv_obj_set_style_local_bg_opa(matrix, LV_BTNMATRIX_PART_BG, LV_STATE_DEFAULT, LV_OPA_TRANSP);
    lv_obj_align(matrix, nullptr, LV_ALIGN_IN_TOP_MID, -2, 62);
    lv_obj_set_style_local_bg_opa(matrix, LV_BTNMATRIX_PART_BTN, LV_STATE_DEFAULT, LV_OPA_TRANSP);
    lv_obj_set_style_local_bg_opa(matrix, LV_BTNMATRIX_PART_BTN, LV_STATE_PRESSED, LV_OPA_TRANSP);
    lv_obj_set_style_local_bg_opa(matrix, LV_BTNMATRIX_PART_BTN, LV_STATE_DISABLED, LV_OPA_TRANSP);
    lv_obj_set_style_local_border_width(matrix, LV_BTNMATRIX_PART_BTN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_pad_all(matrix, LV_BTNMATRIX_PART_BG, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_pad_inner(matrix, LV_BTNMATRIX_PART_BG, LV_STATE_DEFAULT, 8);
    lv_obj_set_style_local_pad_bottom(matrix, LV_BTNMATRIX_PART_BTN, LV_STATE_DEFAULT, 22);
    lv_obj_set_style_local_text_color(matrix, LV_BTNMATRIX_PART_BTN, LV_STATE_DEFAULT, Colors::ink);
    const auto* areas = static_cast<lv_btnmatrix_ext_t*>(lv_obj_get_ext_attr(matrix))->button_areas;
    for (unsigned i = 0; i < 6; ++i) {
      Sprite(&rune_scroll_slot, lv_obj_get_x(matrix) + (areas[i].x1 + areas[i].x2 + 1 - 70) / 2,
             lv_obj_get_y(matrix) + (areas[i].y1 + areas[i].y2 + 1 - 78) / 2);
    }
    return matrix;
  }

  inline lv_obj_t* InventoryIcon(lv_obj_t* matrix, unsigned index, const char* symbol, lv_color_t color) {
    const auto& area = static_cast<lv_btnmatrix_ext_t*>(lv_obj_get_ext_attr(matrix))->button_areas[index];
    auto* icon = Text(symbol, 0, 0, color, &jetbrains_mono_bold_20);
    lv_obj_align(icon, matrix, LV_ALIGN_IN_TOP_LEFT, (area.x1 + area.x2 + 1 - lv_obj_get_width(icon)) / 2, area.y1 + 13);
    return icon;
  }

  inline lv_obj_t* InventoryCaption(lv_obj_t* matrix, unsigned index, const char* name, bool enabled) {
    const auto& area = static_cast<lv_btnmatrix_ext_t*>(lv_obj_get_ext_attr(matrix))->button_areas[index];
    auto* caption = Text(name, 0, 0, enabled ? Colors::ink : Colors::gray);
    lv_obj_align(caption, matrix, LV_ALIGN_IN_TOP_LEFT, (area.x1 + area.x2 + 1 - lv_obj_get_width(caption)) / 2, area.y2 - 21);
    return caption;
  }

  inline lv_obj_t* SettingsRow(lv_obj_t* parent, const char* iconText, const char* name, int width, int height) {
    auto* button = lv_btn_create(parent, nullptr);
    lv_obj_set_size(button, width, height);
    lv_btn_set_layout(button, LV_LAYOUT_OFF);
    lv_obj_set_style_local_bg_opa(button, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_TRANSP);
    lv_obj_set_style_local_bg_opa(button, LV_BTN_PART_MAIN, LV_STATE_PRESSED, LV_OPA_TRANSP);
    lv_obj_set_style_local_border_width(button, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, 0);
    auto* paper = lv_img_create(button, nullptr);
    lv_img_set_src(paper, &rune_scroll_row);
    lv_obj_align(paper, button, LV_ALIGN_IN_TOP_LEFT, 0, 0);
    lv_obj_set_click(paper, false);

    auto* icon = lv_label_create(button, nullptr);
    lv_label_set_text_static(icon, iconText);
    lv_obj_set_style_local_text_color(icon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, Colors::ink);
    lv_label_set_long_mode(icon, LV_LABEL_LONG_CROP);
    lv_label_set_align(icon, LV_LABEL_ALIGN_CENTER);
    lv_obj_set_width(icon, height);
    lv_obj_align(icon, nullptr, LV_ALIGN_IN_LEFT_MID, 0, 0);

    auto* text = lv_label_create(button, nullptr);
    lv_label_set_text(text, name);
    lv_obj_set_style_local_text_color(text, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, Colors::ink);
    lv_obj_align(text, icon, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
    return button;
  }

  inline void SkinUtilityButton(lv_obj_t* button) {
    lv_btn_set_layout(button, LV_LAYOUT_OFF);
    lv_obj_set_style_local_bg_opa(button, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_TRANSP);
    lv_obj_set_style_local_bg_opa(button, LV_BTN_PART_MAIN, LV_STATE_PRESSED, LV_OPA_TRANSP);
    lv_obj_set_style_local_border_width(button, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, 0);
    auto* paper = lv_img_create(button, nullptr);
    lv_img_set_src(paper, &rune_scroll_utility);
    lv_obj_align(paper, button, LV_ALIGN_IN_TOP_LEFT, 0, 0);
    lv_obj_set_click(paper, false);
  }

  inline void ButtonCaption(lv_obj_t* button, lv_obj_t* icon, const char* text) {
    lv_btn_set_layout(button, LV_LAYOUT_OFF);
    lv_obj_set_style_local_text_color(icon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, Colors::ink);
    lv_obj_align(icon, button, LV_ALIGN_CENTER, 0, -22);
    auto* caption = lv_label_create(button, nullptr);
    lv_label_set_text_static(caption, text);
    lv_obj_set_style_local_text_font(caption, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &rune_small);
    lv_obj_set_style_local_text_color(caption, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, Colors::ink);
    lv_obj_align(caption, button, LV_ALIGN_IN_BOTTOM_MID, 0, -23);
  }

  struct HomeWidgets {
    lv_obj_t *time, *ampm, *date, *heartIcon, *heart, *stepIcon, *steps;
    lv_obj_t *notification, *weatherIcon, *temperature, *quest;
  };

  inline HomeWidgets Home() {
    namespace Symbols = Pinetime::Applications::Screens::Symbols;
    HomeFrame();
    return {Text("00:00", 0, 69, Colors::ink, &rune_clock),
            Text("", 208, 35, Colors::parchment),
            Text("", 0, 130, Colors::ink),
            Text(Symbols::heartBeat, 15, 181, Colors::deepOrange, &jetbrains_mono_bold_20),
            Text("--", 44, 183, Colors::ink),
            Text(Symbols::shoe, 126, 181, Colors::green, &jetbrains_mono_bold_20),
            Text("0", 154, 183, Colors::ink),
            Text("", 8, 32, Colors::gold, &jetbrains_mono_bold_20),
            Text("", 8, 2, Colors::blue, &fontawesome_weathericons),
            Text("", 36, 5, Colors::parchment),
            Text("DAILY QUEST: WALK", 13, 215, Colors::ink)};
  }
}
