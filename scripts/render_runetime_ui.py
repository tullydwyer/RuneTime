#!/usr/bin/env python3
"""Build a native, headless LVGL preview using the firmware's real theme/fonts.
Run after the Docker firmware build has generated its fonts. Needs a C/C++ compiler.
Outputs PPM files under build/ui-preview; no watch or desktop UI is required.
"""
from pathlib import Path
import concurrent.futures
import subprocess

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'build/ui-preview'
OUT.mkdir(parents=True, exist_ok=True)
(OUT / 'FreeRTOS.h').write_text('#pragma once\n#include <stdlib.h>\n#include <stdint.h>\n#define pvPortMalloc malloc\n#define vPortFree free\n')
includes = ['-I'+str(OUT), '-I'+str(ROOT/'src'), '-I'+str(ROOT/'src/libs')]
sources = sorted((ROOT/'src/libs/lvgl/src').rglob('*.c'))
sources += sorted((ROOT/'build/src/displayapp/fonts').glob('*.c'))
sources += [ROOT/'src/displayapp/icons/runetime/runetime.c', ROOT/'src/displayapp/InfiniTimeTheme.cpp', ROOT/'tests/runetime_ui.cpp']

def compile(source):
    obj = OUT / (str(source.relative_to(ROOT)).replace('/', '_') + '.o')
    if not obj.exists() or max(source.stat().st_mtime, (ROOT/'src/displayapp/RuneUi.h').stat().st_mtime, (ROOT/'src/displayapp/InfiniTimeTheme.h').stat().st_mtime) > obj.stat().st_mtime:
        cpp = source.suffix == '.cpp'
        subprocess.run(['c++' if cpp else 'cc', '-std=c++17' if cpp else '-std=c99', '-O1', *includes, '-c', str(source), '-o', str(obj)], check=True)
    return str(obj)

with concurrent.futures.ThreadPoolExecutor(max_workers=8) as pool:
    objects = list(pool.map(compile, sources))
subprocess.run(['c++', *objects, '-o', str(OUT/'render')], check=True)
subprocess.run([str(OUT/'render')], cwd=OUT, check=True)
print(f'LVGL preview and layout checks passed: {OUT}')
