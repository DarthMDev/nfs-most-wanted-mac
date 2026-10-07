#!/usr/bin/env python3
"""Build clean-pools.bin (the light pools' lights) from the player's own PC game: the lamp classes of
effects/lamp-classes.toml applied to the lamp scenery of the PC track data. World head = instance base + planar
rotation x class anchor. Per light: position, radius, colour x intensity x activation(kind), falloff d0, section.
Format 'CPL1': u32 count, f32 params[4], then count x {f32 px, py, pz, radius, r, g, b, d0; u32 section, kind}.
usage: pools_bin.py <game folder> <lamp-classes.toml> <out.bin>"""
import collections
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from pc_scenery import pc_sections, section_number, instances, infos


def parse(p):
    classes, tables, cur = {}, collections.defaultdict(dict), None
    for line in Path(p).read_text().splitlines():
        line = line.split('#', 1)[0].strip()
        if not line:
            continue
        if line == '[[class]]':
            cur = {}
            continue
        if line.startswith('[') and line.endswith(']'):
            cur = tables[line.strip('[]')]
            continue
        if cur is None or '=' not in line:
            continue
        k, v = (x.strip() for x in line.split('=', 1))
        if v.startswith('['):
            v = [float(x) for x in v.strip('[]').split(',')]
        elif v in ('true', 'false'):
            v = v == 'true'
        elif v.startswith('"'):
            v = v.strip('"')
        else:
            v = float(v)
        cur[k] = v
        if k == 'name':
            classes[v] = cur
    return classes, tables


def main():
    game, classes_file, out = sys.argv[1], sys.argv[2], Path(sys.argv[3])
    cls, tab = parse(classes_file)
    act = tab['activation']
    params = tab['params'].get('CleanPoolParams', [40.0, 70.0, 1.0, 0.0])
    C = {k: v for k, v in cls.items() if v.get('enabled') and 'anchor' in v}
    recs, n = [], collections.Counter()
    for s in pc_sections(game):
        sec = section_number(s)
        names = infos(s)
        for i in instances(s):
            name = names[i['info']] if 0 <= i['info'] < len(names) else ''
            c = C.get(name)
            if not c:
                continue
            r = i['rot']
            sc = (r[0] ** 2 + r[1] ** 2) ** 0.5 or 1.0
            cs, sn = r[0] / sc, r[1] / sc
            ax, ay, az = c['anchor']
            p = (i['pos'][0] + ax * cs - ay * sn, i['pos'][1] + ax * sn + ay * cs, i['pos'][2] + az)
            kind = c.get('kind', 'exterior')
            a = float(act.get(kind, 0.0))
            col = [x * float(c['intensity']) * a for x in c['color']]
            if a <= 0:
                continue
            recs.append(struct.pack('<8f2I', *p, float(c['radius']), *col, float(c['falloff']), sec, 1 if kind == 'tunnel' else 0))
            n[name] += 1
    if not recs:
        sys.exit('no lamp scenery found in TRACKS/STREAML2RA.BUN; is this the PC game folder?')
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_bytes(b'CPL1' + struct.pack('<I4f', len(recs), *params) + b''.join(recs))
    print(f'light pools: {len(recs)} lights of {len(n)} lamp classes -> {out}')


if __name__ == '__main__':
    main()
