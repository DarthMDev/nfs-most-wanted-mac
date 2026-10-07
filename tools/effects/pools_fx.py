#!/usr/bin/env python3
"""Build clean-world.fx and clean-worldreflect.fx (the light pools' shading) from the player's own speed.exe: copies
of the PC game's own WORLD and WORLDREFLECT effects with one added pass 'clean_pools' per technique, whose programs
are effects/clean-pools.*.asm (from effects/clean-pools.hlsl). Added parameters CleanPoolCount (float4),
CleanPoolParams (float4), CleanPools (float4[16]). The base pass is untouched. Added pass states: blending ONE/ONE,
RGB writes, ZFUNC LESSEQUAL; alpha test as pass 0.
usage: pools_fx.py <effects folder> <speed.exe> <out folder>"""
import hashlib, re, struct, sys
from pathlib import Path
HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from d3d9asm import assemble


def resource_entry(pe, name):
    """(VA of the resource data entry, data RVA, size, bytes) of the named RCDATA resource."""
    for t in pe.DIRECTORY_ENTRY_RESOURCE.entries:
        for e in t.directory.entries:
            if e.name and str(e.name) == name:
                de = e.directory.entries[0].data.struct
                va = pe.OPTIONAL_HEADER.ImageBase + pe.get_rva_from_offset(de.get_file_offset())
                return va, de.OffsetToData, de.Size, pe.get_data(de.OffsetToData, de.Size)
    raise SystemExit(f'{name} not found in speed.exe')

def consts_from_hlsl(src):
    out = []
    for m in re.finditer(r'^\s*float(4x4|4)?\s+(\w+)\s*(?:\[(\d+)\])?\s*:\s*register\(c(\d+)\)\s*;', src, re.M):
        kind, name, n, reg = m.group(1), m.group(2), int(m.group(3) or 1), int(m.group(4))
        if kind == '4x4': out.append((name, 2, reg, 4, 3, 4, 4, 1))
        else: out.append((name, 2, reg, n, 1, 1, 4 if kind else 1, n))
    for m in re.finditer(r'^\s*sampler2D\s+(\w+)\s*:\s*register\(s(\d+)\)\s*;', src, re.M):
        out.append((m.group(1), 3, int(m.group(2)), 1, 4, 1, 1, 1))
    return out

def asm(folder, entry):
    t = (folder / f'clean-pools.{entry}.asm').read_text().splitlines()
    return '\n'.join(l for l in t if l.strip() and not re.match(r'^\s*[vp]s_\d_\d\s*$', l))

NEW = [('CleanPoolCount', 1), ('CleanPoolParams', 1), ('CleanPools', 16)]

def rewrite(fx, vs, ps, tech):
    """Adds the float4 parameters NEW and, to every technique `tech` accepts, a pass clean_pools."""
    tag, start = struct.unpack_from('<II', fx, 0); assert tag == 0xFEFF0901
    body = bytearray(fx[8:]); p = start
    nparams, ntech, unused, nobjects = struct.unpack_from('<4I', body, p); p += 16
    param_recs = []
    for _ in range(nparams):
        s = p; toff, voff, flags, nann = struct.unpack_from('<4I', body, p); p += 16 + 8 * nann
        param_recs.append(bytes(body[s:p]))
    techs = []
    for _ in range(ntech):
        s = p; name, nann, npass = struct.unpack_from('<3I', body, p); p += 12 + 8 * nann
        head = bytes(body[s:p]); passes = []
        for _ in range(npass):
            ps0 = p; pname, pann, nstate = struct.unpack_from('<3I', body, p); p += 12 + 8 * pann
            phead = bytes(body[ps0:p]); states = []
            for _ in range(nstate): states.append(struct.unpack_from('<4I', body, p)); p += 16
            passes.append([phead, states])
        techs.append([head, passes])
    rest = bytes(body[p:]); nstrings, nres = struct.unpack_from('<II', rest, 0)
    un = bytearray(body[:start])
    def add(b):
        off = len(un); un.extend(b)
        while len(un) % 4: un.append(0)
        return off
    def string(s): return add(struct.pack('<I', len(s) + 1) + s.encode() + b'\0')
    for pname_, elements in NEW:
        ptype = add(struct.pack('<7I', 3, 1, string(pname_), string(pname_.upper()), elements if elements > 1 else 0, 4, 1))
        param_recs.append(struct.pack('<4I', ptype, add(bytes(elements * 16)), 0, 0))
    def tech_name(h):
        off = struct.unpack_from('<I', h, 0)[0]; n = struct.unpack_from('<I', body, off)[0]
        return bytes(body[off + 4:off + 4 + n]).split(b'\0')[0].decode()
    added = []; next_obj = nobjects; new_res = []
    for ti, (head, passes) in enumerate(techs):
        st0 = passes[0][1]
        if not tech(tech_name(head)) or not any(x[0] == 146 and struct.unpack_from('<I', body, x[2])[0] == 16 for x in st0): continue
        t_vs = next(s[2] for s in st0 if s[0] == 146); t_ps = next(s[2] for s in st0 if s[0] == 147)
        t_int = next(s[2] for s in st0 if s[0] == 9)
        assert {4, 6, 7, 9, 13, 73} <= {s[0] for s in st0}, 'pass 0 must set every state the added pass changes'
        vs_id, ps_id = next_obj, next_obj + 1; next_obj += 2
        states = [(146, 0, t_vs, add(struct.pack('<I', vs_id))), (147, 0, t_ps, add(struct.pack('<I', ps_id))),
                  (13, 0, t_int, add(struct.pack('<I', 1))), (6, 0, t_int, add(struct.pack('<I', 2))),
                  (7, 0, t_int, add(struct.pack('<I', 2))), (73, 0, t_int, add(struct.pack('<I', 7))),
                  (9, 0, t_int, add(struct.pack('<I', 4)))]
        passes.append([struct.pack('<3I', string('clean_pools'), 0, len(states)), states])
        new_res += [(ti, len(passes) - 1, 0xffffffff, 0, 0, vs), (ti, len(passes) - 1, 0xffffffff, 1, 0, ps)]
        added.append(tech_name(head))
    out = bytearray(un); new_start = len(out)
    out += struct.pack('<4I', nparams + len(NEW), ntech, unused, next_obj)
    for r in param_recs: out += r
    for head, passes in techs:
        out += struct.pack('<3I', struct.unpack_from('<I', head, 0)[0], struct.unpack_from('<I', head, 4)[0], len(passes)) + head[12:]
        for phead, states in passes:
            out += struct.pack('<3I', struct.unpack_from('<I', phead, 0)[0], struct.unpack_from('<I', phead, 4)[0], len(states)) + phead[12:]
            for s in states: out += struct.pack('<4I', *s)
    out += struct.pack('<II', nstrings, nres + len(new_res)) + rest[8:]
    for t, idx, elem, state, usage, data in new_res:
        out += struct.pack('<6I', t, idx, elem, state, usage, len(data)) + data
        while len(out) % 4: out.append(0)
    return struct.pack('<II', tag, new_start) + bytes(out), added

def main():
    import pefile
    folder, exe, priv = Path(sys.argv[1]), sys.argv[2], Path(sys.argv[3])
    src = (folder / 'clean-pools.hlsl').read_text(); consts = consts_from_hlsl(src)
    fl = [c for c in consts if c[1] == 2]
    pe = pefile.PE(exe); priv.mkdir(parents=True, exist_ok=True)
    for res, out, vse, pse, tech in (('IDI_WORLD_FX', 'clean-world.fx', 'VS_Pools', 'PS_Pools', lambda n: n == 'world'),
                                     ('IDI_WORLDREFLECT_FX', 'clean-worldreflect.fx', 'VS_PoolsReflect', 'PS_PoolsReflect', lambda n: n != 'world_fixed')):
        vs = assemble('vs_3_0', fl, asm(folder, vse)); ps = assemble('ps_3_0', consts, asm(folder, pse))
        va, rva, size, data = resource_entry(pe, res)
        fx, added = rewrite(data, vs, ps, tech)
        (priv / out).write_bytes(fx)
        print(f'{res}: entry {va:#010x} rva {rva:#x} size {size} -> {out} {len(fx)} bytes, vs {len(vs)} ps {len(ps)}, pass added to {len(added)} techniques {sorted(set(added))}, sha256 {hashlib.sha256(fx).hexdigest()[:16]}')

if __name__ == '__main__':
    main()
