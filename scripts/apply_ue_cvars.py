#!/usr/bin/env python3
"""Aplica 900p / render scale 3D / 30 FPS / graficos Medium nos .ini da Unreal Engine.

Uso:  python3 scripts/apply_ue_cvars.py <pasta-com-os-ini>

Os valores vem das variaveis de ambiente PS4_* (o workflow ps4_patch.yml preenche); sem elas, valem os padroes abaixo.
- DefaultEngine.ini: se existir, recebe [SystemSettings] e [/Script/Engine.Engine] (sem tocar no resto do arquivo).
- Engine.ini e GameUserSettings.ini: gerados/atualizados (os Default*.ini ficam dentro dos .pak do jogo empacotado,
  entao o runtime do port aplica estes dois no boot).
- DefaultDeviceProfiles.ini e qualquer outro .ini: copiados sem mudanca.
"""
import os
import re
import sys
from pathlib import Path


def env(name, default):
    v = os.environ.get(name, "").strip()
    return v or default


def number(name, default, lo, hi):
    raw = env(name, default)
    try:
        v = float(raw)
    except ValueError:
        sys.exit("erro: %s=%r nao e numero" % (name, raw))
    if not lo <= v <= hi:
        sys.exit("erro: %s=%s fora do intervalo %s..%s" % (name, raw, lo, hi))
    return raw


WIDTH = number("PS4_WIDTH", "1600", 640, 3840)
HEIGHT = number("PS4_HEIGHT", "900", 360, 2160)
SCREEN_PCT = number("PS4_SCREEN_PERCENTAGE", "75", 25, 100)   # 75 = 25% a menos (por eixo)
MAX_FPS = number("PS4_MAX_FPS", "30", 15, 120)
TEXTURE_Q = number("PS4_TEXTURE_QUALITY", "1", 0, 4)         # 1 = Medium
SHADOW_Q = number("PS4_SHADOW_QUALITY", "1", 0, 4)
SSR_Q = number("PS4_SSR_QUALITY", "2", 0, 4)
BLOOM_Q = number("PS4_BLOOM_QUALITY", "3", 0, 5)
POOL_MB = number("PS4_TEXTURE_POOL_MB", "1024", 128, 8192)    # chute inicial: ajustar com teste no console
SYNC_INTERVAL = str(max(1, round(60.0 / float(MAX_FPS))))      # 30 FPS em tela de 60 Hz -> 2

SYSTEM_SETTINGS = [
    ("r.ScreenPercentage", SCREEN_PCT),
    ("t.MaxFPS", MAX_FPS),
    ("r.VSync", "1"),
    ("rhi.SyncInterval", SYNC_INTERVAL),
    ("sg.TextureQuality", TEXTURE_Q),
    ("sg.ShadowQuality", SHADOW_Q),
    ("r.BloomQuality", BLOOM_Q),
    ("r.SSR.Quality", SSR_Q),
    ("r.Streaming.PoolSize", POOL_MB),
]
ENGINE_SECTION = [
    ("bUseFixedFrameRate", "True"),
    ("FixedFrameRate", "%.6f" % float(MAX_FPS)),
    ("bSmoothFrameRate", "False"),
]
GAME_USER_SETTINGS = [
    ("ResolutionSizeX", WIDTH),
    ("ResolutionSizeY", HEIGHT),
    ("LastUserConfirmedResolutionSizeX", WIDTH),
    ("LastUserConfirmedResolutionSizeY", HEIGHT),
    ("FullscreenMode", "0"),
    ("LastConfirmedFullscreenMode", "0"),
    ("bUseVSync", "True"),
    ("FrameRateLimit", "%.6f" % float(MAX_FPS)),
]

SECTION_RE = re.compile(r"^\s*\[(.+?)\]\s*$")


def read_ini(path):
    raw = path.read_bytes()
    if raw.startswith((b"\xff\xfe", b"\xfe\xff")):
        enc = "utf-16"
    elif raw.startswith(b"\xef\xbb\xbf"):
        enc = "utf-8-sig"
    else:
        enc = "utf-8"
    try:
        text = raw.decode(enc)
    except UnicodeDecodeError:
        enc = "latin-1"
        text = raw.decode(enc)
    eol = "\r\n" if "\r\n" in text else "\n"
    lines = text.replace("\r\n", "\n").split("\n")
    if lines and lines[-1] == "":
        lines.pop()
    return lines, enc, eol


def write_ini(path, lines, enc, eol):
    path.write_bytes((eol.join(lines) + eol).encode(enc))


def set_key(lines, section, key, value):
    """Poe key=value em [section]: troca as linhas existentes ou insere no fim da secao. Retorna o texto do log."""
    start, end = None, len(lines)
    for i, line in enumerate(lines):
        m = SECTION_RE.match(line)
        if not m:
            continue
        if start is not None:
            end = i
            break
        if m.group(1).strip().lower() == section.lower():
            start = i
    if start is None:
        if lines and lines[-1].strip():
            lines.append("")
        lines += ["[%s]" % section, "%s=%s" % (key, value)]
        return "+ [%s] %s=%s (secao nova)" % (section, key, value)
    key_re = re.compile(r"^\s*" + re.escape(key) + r"\s*=(.*)$", re.I)
    old = None
    for i in range(start + 1, end):
        m = key_re.match(lines[i])
        if m:
            if old is None:
                old = m.group(1).strip()
            lines[i] = "%s=%s" % (key, value)
    if old is not None:
        return "~ [%s] %s: %s -> %s" % (section, key, old, value)
    at = end
    while at > start + 1 and not lines[at - 1].strip():
        at -= 1
    lines.insert(at, "%s=%s" % (key, value))
    return "+ [%s] %s=%s" % (section, key, value)


def apply(path, blocks, create):
    if path.exists():
        lines, enc, eol = read_ini(path)
    elif create:
        lines, enc, eol = [], "utf-8", "\n"
    else:
        return
    print("== %s%s" % (path.name, " (gerado)" if create and not path.exists() else ""))
    for section, items in blocks:
        for key, value in items:
            print("  " + set_key(lines, section, key, value))
    write_ini(path, lines, enc, eol)


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    folder = Path(sys.argv[1])
    if not folder.is_dir() or not list(folder.glob("*.ini")):
        sys.exit("erro: nenhum .ini em %s" % folder)
    print("alvo: %sx%s, render scale %s%%, %s FPS, texturas=%s sombras=%s ssr=%s bloom=%s, pool %s MB" % (
        WIDTH, HEIGHT, SCREEN_PCT, MAX_FPS, TEXTURE_Q, SHADOW_Q, SSR_Q, BLOOM_Q, POOL_MB))
    engine_blocks = [("SystemSettings", SYSTEM_SETTINGS), ("/Script/Engine.Engine", ENGINE_SECTION)]
    apply(folder / "DefaultEngine.ini", engine_blocks, create=False)
    apply(folder / "Engine.ini", engine_blocks, create=True)
    apply(folder / "GameUserSettings.ini", [("/Script/Engine.GameUserSettings", GAME_USER_SETTINGS)], create=True)
    untouched = [p.name for p in sorted(folder.glob("*.ini"))
                 if p.name not in ("DefaultEngine.ini", "Engine.ini", "GameUserSettings.ini")]
    if untouched:
        print("sem mudanca: " + ", ".join(untouched))


if __name__ == "__main__":
    main()
