#!/usr/bin/env python3
"""Generate compact 4-bit LVGL pixel art. Run after changing the designs below."""
from pathlib import Path
import math

PALETTE = [
    (0, 0, 0, 0),       # transparency
    (30, 29, 26, 255),  # deep crevice
    (48, 46, 40, 255),  # mortar
    (66, 64, 57, 255),  # dark stone
    (85, 83, 75, 255),  # stone
    (107, 104, 94, 255),# raised stone
    (134, 130, 115, 255),# stone glint
    (104, 87, 68, 255), # parchment outline
    (151, 131, 102, 255),# parchment shadow
    (186, 167, 135, 255),# parchment
    (210, 192, 156, 255),# parchment light
    (228, 210, 174, 255),# fresh parchment
    (114, 107, 86, 255),# aged speck
    (104, 121, 73, 255),# moss
    (204, 161, 81, 255),# metal gold
    (143, 64, 49, 255), # sealing wax
]


def noise(x, y, seed=0):
    n = (x * 374761393 + y * 668265263 + seed * 1442695041) & 0xffffffff
    n = ((n ^ (n >> 13)) * 1274126177) & 0xffffffff
    return (n ^ (n >> 16)) & 0xffffffff


def wall(w, h):
    result = []
    for y in range(h):
        row = y // 26
        for x in range(w):
            block_x = x + (row & 1) * 17
            block = block_x // 34
            gx = block_x % 34
            gy = y % 26
            # Offset each stone's chiseled joints and chip away its corners.
            seam_y = noise(block, row, 1) % 5 + noise(x // 4, row, 3) % 3 - 2
            seam_x = noise(block, row, 2) % 4 + noise(y // 4, block, 4) % 3 - 2
            horizontal = gy <= seam_y
            vertical = gx <= seam_x
            corner = min(gx, 33 - gx) + min(gy, 25 - gy) < 7
            upper = gy <= 5
            left = gx <= 6
            lower = gy >= 22
            right = gx >= 30
            grain = noise(x, y, block + row * 31) % 19
            if horizontal or vertical or corner:
                color = 1 if grain < 11 else 2
            elif upper or left:
                color = 6 if grain < 5 else 5 if grain < 14 else 4
            elif lower or right:
                color = 2 if grain < 8 else 3
            else:
                color = (3 if grain < 3 else 5 if grain > 16 else 4)
                if grain == 9 and (x + y) % 11 == 0:
                    color = 13
            result.append(color)
    return result


def parchment(w, h, seed):
    result = []
    for y in range(h):
        for x in range(w):
            # Stepped corners, frayed edges, and gently bowed scroll ends.
            wave_x = int(2 * math.sin(y * .27 + seed))
            wave_y = int(2 * math.sin(x * .11 + seed * .7))
            fray = (noise(x // 3, y // 3, seed) % 3) - 1
            left = 4 + wave_x + fray
            right = w - 5 + wave_x - fray
            top = 4 + wave_y + fray
            bottom = h - 5 + wave_y - fray
            corner = max(0, 8 - min(y, h - 1 - y)) // 2
            left += corner
            right -= corner
            dist = min(x - left, right - x, y - top, bottom - y)
            grain = noise(x, y, seed) % 37
            if dist < 0:
                color = 0
            elif dist == 0:
                color = 1 if grain < 22 else 7
            elif dist == 1:
                color = 7
            elif dist < 4:
                color = 8 if grain < 24 else 9
            else:
                color = 9
                if grain < 3:
                    color = 8
                elif grain > 34:
                    color = 10
                if (x + seed * 7) % 67 in (0, 1) and grain == 19:
                    color = 12
                # Rolled hems and softly lit creases are broken into pixels.
                if abs(y - top - 5) < 2 and noise(x // 4, y, seed) % 4 == 0:
                    color = 10
                if abs(bottom - y - 5) < 2 and noise(x // 4, y, seed + 1) % 5 == 0:
                    color = 8
            result.append(color)
    return result


def stone_tile(w, h, seed, button=False):
    result = []
    for y in range(h):
        for x in range(w):
            edge = 3 + (noise(x // 3, y // 3, seed) % 3) - 1
            corner = min(x, w - 1 - x) + min(y, h - 1 - y)
            dist = min(x, w - 1 - x, y, h - 1 - y)
            grain = noise(x, y, seed) % 17
            if dist < edge or corner < 10:
                color = 0
            elif dist < edge + 2:
                color = 1 if grain < 9 else 6
            elif dist < edge + 5:
                color = 5 if x + y < (w + h) // 2 else 3
            else:
                color = (5 if grain > 14 else 4) if button else (3 if grain < 12 else 4)
                if grain == 8 and (x + 2 * y) % 11 == 0:
                    color = 14 if button else 13
            result.append(color)
    return result


IMAGES = {
    'rune_wall': (240, 240, wall(240, 240)),
    'rune_scroll_clock': (230, 96, parchment(230, 96, 1)),
    'rune_scroll_skill_left': (110, 48, parchment(110, 48, 2)),
    'rune_scroll_skill_right': (117, 48, parchment(117, 48, 3)),
    'rune_scroll_quest': (230, 31, parchment(230, 31, 4)),
    'rune_scroll_slot': (70, 78, parchment(70, 78, 5)),
    'rune_scroll_row': (228, 47, parchment(228, 47, 6)),
    'rune_scroll_utility': (115, 100, parchment(115, 100, 7)),
    'rune_scroll_strength_session': (230, 59, parchment(230, 59, 8)),
    'rune_strength_slot': (70, 78, stone_tile(70, 78, 9)),
    'rune_strength_button': (76, 34, stone_tile(76, 34, 10, True)),
}

# A tiny wax seal closes the quest scroll without a long progress bar.
quest_width, quest_height, quest_pixels = IMAGES['rune_scroll_quest']
for seal_y in range(10, 23):
    for seal_x in range(207, 220):
        dx, dy = seal_x - 213, seal_y - 16
        if dx * dx + dy * dy <= 36 and quest_pixels[seal_y * quest_width + seal_x]:
            quest_pixels[seal_y * quest_width + seal_x] = 15 if (dx + dy) % 4 else 14

lines = ['#include "lvgl/lvgl.h"', '']
for name, (width, height, pixels) in IMAGES.items():
    assert len(pixels) == width * height
    data = bytearray()
    for red, green, blue, alpha in PALETTE:
        data.extend((blue, green, red, alpha))
    for y in range(height):
        row = pixels[y * width:(y + 1) * width]
        for x in range(0, width, 2):
            data.append((row[x] << 4) | (row[x + 1] if x + 1 < width else 0))
    lines += [f'const uint8_t {name}_map[] = {{']
    for offset in range(0, len(data), 24):
        lines.append('  ' + ', '.join(f'0x{value:02x}' for value in data[offset:offset + 24]) + ',')
    lines += ['};', f'const lv_img_dsc_t {name} = {{',
              f'  {{LV_IMG_CF_INDEXED_4BIT, 0, 0, {width}, {height}}},',
              f'  {len(data)},', f'  {name}_map', '};', '']

Path(__file__).with_name('runetime.c').write_text('\n'.join(lines))
print('Generated', len(IMAGES), 'sprites,', sum((w + 1) // 2 * h + 64 for w, h, _ in IMAGES.values()), 'bytes')
