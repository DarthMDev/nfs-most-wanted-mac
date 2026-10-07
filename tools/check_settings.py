#!/usr/bin/env python3
"""Settings drift check. Read-only.

Compares the authoritative schema (mods/core/nfsmw/mod.toml [settings]) with:
  - NativeOptions.ini (documented keys and their commented default values),
  - native_options.h (INI alias table targets),
  - docs/SETTINGS.md (keys mentioned in backticks).
Exit status 1 if any hard drift is found (unknown key, default mismatch, alias to a missing key).
Usage: tools/check_settings.py [path to the repository]; run by tests/test_settings_schema.py."""
import re, sys, tomllib
from pathlib import Path

root = Path(sys.argv[1] if len(sys.argv) > 1 else '.')
mod = root / 'mods/core/nfsmw'
schema = tomllib.loads((mod / 'mod.toml').read_text())['settings']
hard, soft = [], []

def norm(v):
    if isinstance(v, bool): return int(v)
    return v

# 1. NativeOptions.ini: "; key = value" in [Native]; "; Key = value" in alias sections
ini = (mod / 'NativeOptions.ini').read_text().splitlines()
aliases = {}
for m in re.finditer(r'\{"(\w+)",\s*"(\w+)",\s*"(\w+)",\s*([\d.]+)\}', (mod / 'native_options.h').read_text()):
    aliases[(m[1], m[2])] = (m[3], float(m[4]))
section, ini_keys = None, set()
for n, line in enumerate(ini, 1):
    s = line.strip()
    if s.startswith('['): section = s.strip('[]'); continue
    m = re.match(r';?\s*([A-Za-z][A-Za-z0-9_]*)\s*=\s*(-?[\d.]+)\b', s)
    if not m: continue
    key, val = m[1], float(m[2])
    if section == 'Native':
        native, scale = key, 1.0
    elif (section, key) in aliases:
        native, scale = aliases[(section, key)]
    else:
        hard.append(f'NativeOptions.ini:{n} [{section}] {key}: no schema setting or alias'); continue
    ini_keys.add(native)
    if native not in schema:
        hard.append(f'NativeOptions.ini:{n} {key} -> {native}: not in mod.toml schema'); continue
    d = norm(schema[native].get('default'))
    if d is not None and abs(val * scale - d) > 1e-6:
        hard.append(f'NativeOptions.ini:{n} {key}={m[2]} (x{scale:g}) but schema default {native}={d}')
for (sec, key), (native, _) in aliases.items():
    if native not in schema: hard.append(f'native_options.h alias [{sec}] {key} -> {native}: not in schema')

# 2. Settings guide: backticked snake_case keys
doc = (root / 'docs/SETTINGS.md').read_text()
ticked = set(re.findall(r'`([a-z][a-z0-9_]*)`', doc))
mentioned = {k for k in ticked if k in schema} | {k for k in ticked if '_' in k}
known_other = set()  # kit/host settings documented in the same guide are reported, not failed
for k in sorted(mentioned - set(schema)):
    soft.append(f'guide mentions `{k}`: not a core.nfsmw schema key (kit/host setting, env var, or stale)')
undocumented = sorted(k for k in schema if k not in mentioned and k not in ini_keys)
first = doc.splitlines()[0] if doc else ''
if re.search(r'Test\d+|Development \d+', first): soft.append(f'guide title looks stale: "{first}"')

print(f'schema: {len(schema)} settings; INI documents {len(ini_keys)}; guide mentions {len(mentioned & set(schema))}')
print(f'undocumented in both INI and guide: {len(undocumented)}: ' + ', '.join(undocumented))
for h in hard: print('DRIFT', h)
for s in soft: print('NOTE ', s)
print('RESULT', 'FAIL' if hard else 'PASS', f'({len(hard)} drift, {len(soft)} notes)')
sys.exit(1 if hard else 0)
