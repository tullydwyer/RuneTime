#!/usr/bin/env python3

from pathlib import Path

from PIL import Image


SOURCE = Path(__file__).with_name("strength.png")
DEST = SOURCE.with_suffix(".c")
PALETTE = [
    (0, 0, 0, 0),
    (28, 25, 21, 255),
    (62, 54, 43, 255),
    (123, 105, 79, 255),
    (181, 156, 116, 255),
    (213, 190, 148, 255),
]


sprite = Image.open(SOURCE).convert("RGBA")
if sprite.size != (48, 48):
    raise ValueError("Strength icon must be 48x48 pixels")


def palette_index(pixel):
    if pixel[3] < 128:
        return 0
    return min(
        range(1, len(PALETTE)),
        key=lambda index: sum((pixel[channel] - PALETTE[index][channel]) ** 2 for channel in range(3)),
    )


pixels = [palette_index(sprite.getpixel((x, y))) for y in range(48) for x in range(48)]
data = bytearray()
for red, green, blue, alpha in PALETTE:
    data.extend((blue, green, red, alpha))
data.extend(b"\x00" * (16 - len(PALETTE)) * 4)
for offset in range(0, len(pixels), 2):
    data.append((pixels[offset] << 4) | pixels[offset + 1])

lines = [
    '#include "lvgl/lvgl.h"',
    "",
    "const uint8_t strength_icon_map[] = {",
]
for offset in range(0, len(data), 16):
    lines.append("  " + ", ".join(f"0x{value:02x}" for value in data[offset : offset + 16]) + ",")
lines += [
    "};",
    "",
    "extern const lv_img_dsc_t strength_icon = {",
    "  {LV_IMG_CF_INDEXED_4BIT, 0, 0, 48, 48},",
    f"  {len(data)},",
    "  strength_icon_map",
    "};",
    "",
]
DEST.write_text("\n".join(lines))
