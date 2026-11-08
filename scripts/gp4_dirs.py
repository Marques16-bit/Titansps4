#!/usr/bin/env python3
"""Garante que o .gp4 liste todas as pastas dos arquivos do pacote (o create-gp4 as vezes omite alguma).
Uso: python3 scripts/gp4_dirs.py pkg.gp4
"""
import sys
import xml.etree.ElementTree as ET


def ensure_dir(parent, name, created):
    for d in parent.findall("dir"):
        if d.get("targ_name") == name:
            return d
    d = ET.SubElement(parent, "dir", {"targ_name": name})
    created.append(name)
    return d


def main(path):
    tree = ET.parse(path)
    top = tree.getroot().find(".//rootdir")
    if top is None:
        return
    created = []
    for f in tree.getroot().iter("file"):
        node = top
        for name in f.get("targ_path", "").split("/")[:-1]:
            node = ensure_dir(node, name, created)
    if created:
        tree.write(path, encoding="utf-8", xml_declaration=True)
        print("gp4: pastas adicionadas:", ", ".join(created))


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    main(sys.argv[1])
