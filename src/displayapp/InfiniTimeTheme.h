#pragma once

#include <lvgl/lvgl.h>

// RuneTime: the 2004 stone frame, parchment panels and pixel-era accents.
namespace Colors {
  static constexpr lv_color_t deepOrange = LV_COLOR_MAKE(0xd4, 0x65, 0x4c);
  static constexpr lv_color_t orange = LV_COLOR_MAKE(0xe5, 0xb8, 0x57);
  static constexpr lv_color_t green = LV_COLOR_MAKE(0xa8, 0xb8, 0x63);
  static constexpr lv_color_t blue = LV_COLOR_MAKE(0x83, 0xb5, 0xbd);
  static constexpr lv_color_t lightGray = LV_COLOR_MAKE(0xbc, 0xb0, 0x91);
  static constexpr lv_color_t gray = LV_COLOR_MAKE(0x78, 0x71, 0x60);
  static constexpr lv_color_t parchment = LV_COLOR_MAKE(0xeb, 0xdf, 0xbc);
  static constexpr lv_color_t gold = LV_COLOR_MAKE(0xff, 0xd4, 0x64);
  static constexpr lv_color_t border = LV_COLOR_MAKE(0x83, 0x81, 0x74);
  static constexpr lv_color_t bg = LV_COLOR_MAKE(0x50, 0x4f, 0x48);
  static constexpr lv_color_t bgAlt = LV_COLOR_MAKE(0x39, 0x38, 0x32);
  static constexpr lv_color_t bgDark = LV_COLOR_MAKE(0x26, 0x25, 0x20);
  static constexpr lv_color_t highlight = LV_COLOR_MAKE(0x62, 0x70, 0x3c);
  static constexpr lv_color_t stone = LV_COLOR_MAKE(0x51, 0x50, 0x49);
  static constexpr lv_color_t stoneLight = LV_COLOR_MAKE(0x83, 0x81, 0x74);
  static constexpr lv_color_t stoneShadow = LV_COLOR_MAKE(0x24, 0x23, 0x20);
  static constexpr lv_color_t scroll = LV_COLOR_MAKE(0xba, 0xaf, 0x99);
  static constexpr lv_color_t scrollLight = LV_COLOR_MAKE(0xd2, 0xc6, 0xb1);
  static constexpr lv_color_t ink = LV_COLOR_MAKE(0x30, 0x2b, 0x22);
};

LV_FONT_DECLARE(rune_small);
LV_FONT_DECLARE(rune_clock);

/**
 * Initialize the default
 * @param color_primary the primary color of the theme
 * @param color_secondary the secondary color for the theme
 * @param flags ORed flags starting with `LV_THEME_DEF_FLAG_...`
 * @param font_small pointer to a small font
 * @param font_normal pointer to a normal font
 * @param font_subtitle pointer to a large font
 * @param font_title pointer to a extra large font
 * @return a pointer to reference this theme later
 */
lv_theme_t* lv_pinetime_theme_init();
