#!/usr/bin/env python3
"""Gera a capa: app/sce_sys/icon0.png (512x512) e app/sce_sys/pic1.png (1920x1080, fundo do menu do PS4).
Uso: python3 tools/gen_cover.py   (precisa de Pillow e da fonte Poppins Bold)"""
import math
from PIL import Image, ImageDraw, ImageFont, ImageFilter

FONT = "/usr/share/fonts/truetype/google-fonts/Poppins-Bold.ttf"
GOLD, WHITE, ICE = (255, 205, 60), (255, 255, 255), (170, 215, 245)


def lerp(a, b, t):
    return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(3))


def gradient(w, h):
    img = Image.new("RGB", (w, h))
    d = ImageDraw.Draw(img)
    stops = [(0.0, (5, 24, 62)), (0.55, (12, 92, 148)), (1.0, (8, 50, 110))]
    for y in range(h):
        t = y / (h - 1)
        for (t0, c0), (t1, c1) in zip(stops, stops[1:]):
            if t0 <= t <= t1:
                d.line([(0, y), (w, y)], fill=lerp(c0, c1, (t - t0) / (t1 - t0)))
    return img


def glow(img, cx, cy, r):
    layer = Image.new("RGBA", img.size, (0, 0, 0, 0))
    d = ImageDraw.Draw(layer)
    for i in range(40, 0, -1):
        rr = r * i / 40
        d.ellipse([cx - rr, cy - rr, cx + rr, cy + rr], fill=(255, 210, 90, int(3 + 2.2 * (40 - i) / 4)))
    layer = layer.filter(ImageFilter.GaussianBlur(r / 12))
    img.paste(layer, (0, 0), layer)


def rays(img, cx, cy, length, n=14):
    layer = Image.new("RGBA", img.size, (0, 0, 0, 0))
    d = ImageDraw.Draw(layer)
    for i in range(n):
        a0 = 2 * math.pi * i / n
        a1 = a0 + math.pi / n * 0.55
        d.polygon([(cx, cy), (cx + length * math.cos(a0), cy + length * math.sin(a0)),
                   (cx + length * math.cos(a1), cy + length * math.sin(a1))], fill=(255, 235, 170, 20))
    img.paste(layer, (0, 0), layer)


def trident(d, cx, cy, s, color):
    w = 0.075 * s
    d.rectangle([cx - w / 2, cy - 0.55 * s, cx + w / 2, cy + 0.72 * s], fill=color)           # haste
    for dx in (-0.27 * s, 0.27 * s):                                                          # dentes laterais
        d.rectangle([cx + dx - w / 2, cy - 0.42 * s, cx + dx + w / 2, cy - 0.02 * s], fill=color)
    d.rectangle([cx - 0.27 * s - w / 2, cy - 0.06 * s, cx + 0.27 * s + w / 2, cy + 0.02 * s + w * 0.35], fill=color)
    d.pieslice([cx - 0.27 * s - w / 2, cy - 0.20 * s, cx + 0.27 * s + w / 2, cy + 0.20 * s], 0, 180, fill=color)
    d.pieslice([cx - 0.27 * s + w / 2, cy - 0.14 * s, cx + 0.27 * s - w / 2, cy + 0.14 * s], 0, 180, fill=None)
    for dx, tip, half, base in ((0, -0.86, 0.12, -0.50), (-0.27, -0.70, 0.095, -0.36), (0.27, -0.70, 0.095, -0.36)):
        d.polygon([(cx + dx * s, cy + tip * s), (cx + dx * s - half * s, cy + base * s),
                   (cx + dx * s + half * s, cy + base * s)], fill=color)


def emblem(img, cx, cy, r, ss):
    shadow = Image.new("RGBA", img.size, (0, 0, 0, 0))
    sd = ImageDraw.Draw(shadow)
    sd.ellipse([cx - r + 8 * ss, cy - r + 10 * ss, cx + r + 8 * ss, cy + r + 10 * ss], fill=(0, 10, 30, 140))
    shadow = shadow.filter(ImageFilter.GaussianBlur(10 * ss))
    img.paste(shadow, (0, 0), shadow)
    d = ImageDraw.Draw(img)
    d.ellipse([cx - r, cy - r, cx + r, cy + r], fill=(8, 40, 92))
    d.ellipse([cx - r, cy - r, cx + r, cy + r], outline=GOLD, width=int(r * 0.07))
    d.ellipse([cx - r * 0.84, cy - r * 0.84, cx + r * 0.84, cy + r * 0.84], outline=(255, 205, 60, 120), width=max(2, int(r * 0.015)))
    trident(d, cx, cy + r * 0.03, r * 0.78, GOLD)


def waves(img, base, ss, amp):
    w, h = img.size
    layers = [((22, 100, 168), 0, 1.0, 0.0), ((14, 72, 138), 60 * ss, 1.6, 1.3), ((8, 44, 100), 125 * ss, 2.3, 2.6)]
    d = ImageDraw.Draw(img)
    for color, off, freq, ph in layers:
        pts = [(x, base + off + amp * math.sin(x / w * math.pi * 2 * freq + ph)) for x in range(0, w + 8, 8)]
        d.polygon(pts + [(w, h), (0, h)], fill=color)


def text(d, xy, s, size, fill, shadow=True, anchor="la"):
    f = ImageFont.truetype(FONT, size)
    x, y = xy
    if shadow:
        d.text((x + size * 0.04, y + size * 0.05), s, font=f, fill=(3, 16, 44), anchor=anchor)
    d.text((x, y), s, font=f, fill=fill, anchor=anchor)


def pic1():
    ss = 2
    w, h = 1920 * ss, 1080 * ss
    img = gradient(w, h)
    cx, cy = 1400 * ss, 450 * ss
    glow(img, cx, cy, 520 * ss)
    rays(img, cx, cy, 1500 * ss)
    emblem(img, cx, cy, 250 * ss, ss)
    waves(img, 860 * ss, ss, 22 * ss)
    d = ImageDraw.Draw(img)
    text(d, (150 * ss, 250 * ss), "TITANS", 200 * ss, GOLD)
    text(d, (156 * ss, 470 * ss), "OF THE TIDE", 92 * ss, WHITE)
    text(d, (160 * ss, 600 * ss), "P S 4   P O R T", 48 * ss, ICE)
    img.resize((1920, 1080), Image.LANCZOS).save("app/sce_sys/pic1.png")


def icon0():
    ss = 3
    w = 512 * ss
    img = gradient(w, w)
    cx, cy = w // 2, 300 * ss
    glow(img, cx, cy, 230 * ss)
    rays(img, cx, cy, 700 * ss)
    emblem(img, cx, cy, 128 * ss, ss)
    waves(img, 420 * ss, ss, 9 * ss)
    d = ImageDraw.Draw(img)
    text(d, (w // 2, 22 * ss), "TITANS", 78 * ss, GOLD, anchor="ma")
    text(d, (w // 2, 106 * ss), "OF THE TIDE", 34 * ss, WHITE, anchor="ma")
    text(d, (w // 2, 456 * ss), "PS4", 40 * ss, WHITE, anchor="ma")
    img.resize((512, 512), Image.LANCZOS).save("app/sce_sys/icon0.png")


if __name__ == "__main__":
    pic1()
    icon0()
    print("ok")
