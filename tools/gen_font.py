#!/usr/bin/env python3
"""Gera app/src/font8x16.h (ASCII 32..126, 8x16, 1 bit por pixel) a partir da DejaVu Sans Mono.
Uso: python3 tools/gen_font.py [caminho/DejaVuSansMono.ttf]
"""
import sys
from PIL import Image, ImageDraw, ImageFont

path = sys.argv[1] if len(sys.argv) > 1 else "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf"
font = ImageFont.truetype(path, 13)
rows = []
for code in range(32, 127):
    img = Image.new("L", (8, 16), 0)
    ImageDraw.Draw(img).text((0, 0), chr(code), font=font, fill=255)
    px = img.load()
    glyph = []
    for y in range(16):
        b = 0
        for x in range(8):
            if px[x, y] > 110:
                b |= 0x80 >> x
        glyph.append(b)
    rows.append((code, glyph))

with open("app/src/font8x16.h", "w") as f:
    f.write("/* Gerado por tools/gen_font.py. Glifos derivados da DejaVu Sans Mono (licenca Bitstream Vera, ver CREDITS.md). */\n")
    f.write("#pragma once\n#include <stdint.h>\n\n")
    f.write("/* ASCII 32..126, 16 linhas por glifo, bit 7 = pixel da esquerda */\n")
    f.write("static const uint8_t k_font8x16[95][16] = {\n")
    for code, g in rows:
        f.write("    {" + ",".join("0x%02X" % v for v in g) + "}, /* %d */\n" % code)
    f.write("};\n")
print("ok: %d glifos" % len(rows))
