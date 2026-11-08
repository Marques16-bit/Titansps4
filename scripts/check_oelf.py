#!/usr/bin/env python3
"""Confere o .oelf gerado pelo create-fself: tem de haver exatamente 1 segmento LOAD so de leitura/escrita (RW).
Com 2 ou mais, o PS4 recusa o app (CE-41839-5).
Uso: python3 scripts/check_oelf.py build/titans_eboot/titans.oelf
"""
import os
import shutil
import subprocess
import sys


def find_readelf():
    openorbis = os.environ.get("OPENORBIS", "/opt/pacbrew/ps4/openorbis")
    for cand in (os.path.join(openorbis, "bin", "llvm-readelf"), shutil.which("llvm-readelf"), shutil.which("readelf")):
        if cand and os.path.exists(cand):
            return cand
    return None


def main(path):
    tool = find_readelf()
    if not tool:
        print("check_oelf: readelf nao encontrado, pulando a conferencia")
        return 0
    out = subprocess.run([tool, "-l", "-W", path], capture_output=True, text=True)
    if out.returncode != 0:
        print("check_oelf: %s falhou em %s\n%s" % (tool, path, out.stderr), file=sys.stderr)
        return 1
    rw = 0
    for line in out.stdout.splitlines():
        cols = line.split()
        # LOAD offset vaddr paddr filesz memsz flags... align
        if len(cols) >= 8 and cols[0] == "LOAD" and "".join(cols[6:-1]) == "RW":
            rw += 1
    if rw != 1:
        print("check_oelf: %s tem %d segmentos LOAD RW (esperado 1); o PS4 vai recusar o app.\n%s" % (path, rw, out.stdout),
              file=sys.stderr)
        return 1
    print("check_oelf: segmentos ok em", path)
    return 0


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    sys.exit(main(sys.argv[1]))
