#!/usr/bin/env python3
"""Write docs/SETTINGS.md from mods/core/nfsmw/mod.toml, so the settings list can never drift from the code.

    python3 tools/setup_kit/settings_doc.py            # rewrite docs/SETTINGS.md
    python3 tools/setup_kit/settings_doc.py --check    # exit 1 if docs/SETTINGS.md is out of date"""
import re, sys, tomllib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MOD = ROOT / "mods/core/nfsmw/mod.toml"
OUT = ROOT / "docs/SETTINGS.md"
PAGES = [("graphics", "Graphics"), ("graphics.appearance", "Graphics: look and effects"), ("visual", "Visual details"),
         ("controls", "Controls"), ("feedback", "Controller rumble"), ("career", "Career and Extra Options"),
         ("audio", "Audio"), ("fixes", "Fixes"), ("diagnostics", "Diagnostics"), ("", "Other")]
APPLY = {"live": "at once", "restart": "after a restart", "renderer": "after Apply", "next_screen": "at the next screen or load",
         "new_career": "for new careers", "game_video": "with the game's video settings"}


def value_text(spec):
    d = spec.get("default")
    if spec.get("type") == "bool":
        return "on" if d else "off"
    if "choices" in spec:
        choices = spec["choices"].split("|")
        lo = spec.get("min", 0)
        i = d - lo
        if 0 <= i < len(choices) and choices[i]:
            return choices[i]
    unit = spec.get("unit", "")
    return f"{unit}{d}" if unit == "$" else f"{d}"


def render():
    settings = tomllib.loads(MOD.read_text())["settings"]
    groups = {}
    for key, spec in settings.items():
        groups.setdefault(spec.get("group", ""), []).append((key, spec))
    out = ["# Settings", "",
           "Every setting of the native app, written from `mods/core/nfsmw/mod.toml` by "
           "`tools/setup_kit/settings_doc.py` (do not edit by hand).", "",
           "Open the settings menu with **F10** (Fn+F10 on a laptop), the controller's touchpad button, or Back+Start. "
           "Each row says when a change takes effect. The same keys can be set in `NativeOptions.ini` in your profile "
           "folder (see the companion guide).", ""]
    for gid, title in PAGES:
        rows = groups.pop(gid, [])
        if not rows:
            continue
        out += [f"## {title}", "", "| Setting | Key | Default | Takes effect | What it does |", "|---|---|---|---|---|"]
        for key, spec in rows:
            label = spec.get("label", key).replace("|", "/")
            help_text = re.sub(r"\s+", " ", spec.get("help", "")).replace("|", "/")
            out.append(f"| {label} | `{key}` | {value_text(spec)} | {APPLY.get(spec.get('apply', ''), spec.get('apply', ''))} | {help_text} |")
        out.append("")
    assert not groups, f"settings in unknown groups: {list(groups)}"
    return "\n".join(out)


if __name__ == "__main__":
    text = render()
    if "--check" in sys.argv:
        sys.exit(0 if OUT.exists() and OUT.read_text() == text else 1)
    OUT.write_text(text)
    print(f"wrote {OUT.relative_to(ROOT)}")
