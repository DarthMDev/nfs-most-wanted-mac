#!/usr/bin/env python3
"""Convert a TexWizard texture pack's name list (TRACKS/TexWizardX360/meta.json in the game folder, as the Xbox 360
Stuff Pack installs it) to texture-pack.map, the sorted name-hash pairs the core mod's texture_pack.h reads.
Name hash: the game's bStringHash (seed 0xffffffff, x 33 + byte; case sensitive); "0x..." names are hashes already.
usage: texture_pack_map.py <game folder> <out texture-pack.map>"""
import json
import re
import sys
from pathlib import Path

LIMIT = 8192  # TP_MAP_LIMIT in texture_pack.h


def b_hash(name):
    if re.fullmatch(r"0x[0-9a-fA-F]{1,8}", name):
        return int(name, 16)
    value = 0xffffffff
    for byte in name.encode("ascii"):
        value = (value * 33 + byte) & 0xffffffff
    return value


def read_mapping(path):
    text = path.read_text(encoding="utf-8-sig")
    # Keep quoted strings while removing the comments the pack's JSON allows.
    text = re.sub(r'"(?:\\.|[^"\\])*"|//[^\r\n]*|/\*[\s\S]*?\*/', lambda m: m[0] if m[0].startswith('"') else "", text)
    rows = json.loads(text)["textures"]
    pairs, names = {}, {}
    for row in rows:
        if not isinstance(row, list) or len(row) != 2 or not all(isinstance(n, str) for n in row):
            raise SystemExit(f"{path}: entries must be pairs of names")
        a, b = map(b_hash, row)
        if not a or not b:
            raise SystemExit(f"{path}: a name hashes to 0: {row}")
        if a in names and names[a] != row[0]:
            raise SystemExit(f"{path}: two names share a hash: {row}")
        pairs[a] = b  # the pack's own rule: the last duplicate wins
        names[a] = row[0]
    if not 0 < len(pairs) <= LIMIT:
        raise SystemExit(f"{path}: {len(pairs)} names; the loader takes 1 to {LIMIT}")
    return pairs


def main():
    game, out = Path(sys.argv[1]), Path(sys.argv[2])
    folder = next((p for p in (game / "TRACKS").iterdir() if p.name.lower() == "texwizardx360"), None) if (game / "TRACKS").is_dir() else None
    if folder is None or not (folder / "meta.json").is_file() or not (folder / "textures.bin").is_file():
        print("texture pack: none found (TRACKS/TexWizardX360 with meta.json and textures.bin); skipped")
        return 2
    pairs = read_mapping(folder / "meta.json")
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text("NFSMW-TEXPACK-MAP-1\n" + "".join(f"{a:08x} {b:08x}\n" for a, b in sorted(pairs.items())))
    print(f"texture pack: {len(pairs)} names -> {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
