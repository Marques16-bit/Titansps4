#!/usr/bin/env bash
# Baixa a toolchain OpenOrbis do PacBrew (pacotes ps4-openorbis-*) e descompacta em <raiz>/opt/pacbrew, sem pacman.
# Cada pacote e conferido pelo SHA-256 que consta no banco do repositorio.
#   fetch-pacbrew.sh [raiz, padrao /]     (precisa de escrita em <raiz>/opt)
set -euo pipefail
exec python3 - "${1:-/}" <<'PY'
import glob
import hashlib
import os
import subprocess
import sys
import tempfile
import urllib.request

root = sys.argv[1]
repo = os.environ.get("PACBREW_REPO", "https://pacman.mydedibox.fr/pacbrew/packages").rstrip("/")


def download(name, dest):
    with urllib.request.urlopen(repo + "/" + name, timeout=120) as resp, open(dest, "wb") as out:
        while True:
            block = resp.read(1 << 20)
            if not block:
                break
            out.write(block)


def sha256_of(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for block in iter(lambda: f.read(1 << 20), b""):
            h.update(block)
    return h.hexdigest()


def field(desc_text, key):
    lines = desc_text.splitlines()
    return lines[lines.index("%" + key + "%") + 1].strip()


with tempfile.TemporaryDirectory() as tmp:
    db = os.path.join(tmp, "pacbrew.db")
    db_dir = os.path.join(tmp, "db")
    os.makedirs(db_dir)
    download("pacbrew.db", db)
    subprocess.run(["tar", "-xf", db, "-C", db_dir], check=True)

    done = 0
    for entry in sorted(glob.glob(os.path.join(db_dir, "ps4-openorbis-*"))):
        desc = os.path.join(entry, "desc")
        if not os.path.isfile(desc):
            continue
        with open(desc) as f:
            text = f.read()
        name, expected = field(text, "FILENAME"), field(text, "SHA256SUM")
        print("pacbrew:", name, flush=True)
        pkg = os.path.join(tmp, name)
        download(name, pkg)
        if sha256_of(pkg) != expected:
            sys.exit("pacbrew: %s nao confere com o SHA-256 do repositorio" % name)
        subprocess.run(["tar", "-xf", pkg, "-C", root, "--exclude=.PKGINFO", "--exclude=.MTREE",
                        "--exclude=.BUILDINFO", "--exclude=.INSTALL"], check=True)
        os.remove(pkg)
        done += 1

    if not done:
        sys.exit("pacbrew: nenhum pacote ps4-openorbis-* em " + repo)
    print("pacbrew: %d pacotes em %s/opt/pacbrew/ps4/openorbis" % (done, root.rstrip("/")))
PY
