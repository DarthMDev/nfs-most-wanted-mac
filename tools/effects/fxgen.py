"""Package shader-model-3 assembly into an fx_2_0 effect the game's D3DX loader accepts (the rebuilt effects of
effects/). Generic tooling: D3D9 shader bytecode via d3d9asm.py beside this file, and the D3DX effect container.
The HLSL holds the manifest; the .asm files (vkd3d-compiler output, or written by hand) are the programs.
Manifest = the HLSL file itself:
  - constants: lines 'float4 Name : register(cN);' or 'float Name : register(cN);' (arrays: 'float4 Name[K] : register(cN);')
  - samplers:  lines 'sampler2D Name : register(sN);'  (bound by the host through its stage; listed in each CTAB)
  - techniques: comment lines '// technique NAME: vs=ENTRY ps=ENTRY'; the .asm files are '<base>.<ENTRY>.asm'
usage: fxgen.py <file.hlsl> <asm dir> <out.fx>"""
import re, struct, sys
from pathlib import Path
HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import d3d9asm
from d3d9asm import assemble
hlsl, asmdir, outp = Path(sys.argv[1]), Path(sys.argv[2]), Path(sys.argv[3])
src = hlsl.read_text()
consts, params = [], []
for m in re.finditer(r'^\s*float(4)?\s+(\w+)\s*(?:\[(\d+)\])?\s*:\s*register\(c(\d+)\)\s*;', src, re.M):
    vec, name, n, reg = m.group(1), m.group(2), int(m.group(3) or 1), int(m.group(4))
    consts.append((name, 2, reg, n, 1, 1, 4 if vec else 1, n)); params.append((name, 1 if vec else 0, n))
for m in re.finditer(r'^\s*sampler2D\s+(\w+)\s*:\s*register\(s(\d+)\)\s*;', src, re.M):
    consts.append((m.group(1), 3, int(m.group(2)), 1, 4, 1, 1, 1))
techs = re.findall(r'^\s*//\s*technique\s+(\w+)\s*:\s*vs=(\w+)\s+ps=(\w+)', src, re.M)
if not techs: sys.exit('no "// technique NAME: vs=ENTRY ps=ENTRY" lines')
def asm_text(entry):
    t = (asmdir / f'{hlsl.stem}.{entry}.asm').read_text().splitlines()
    return '\n'.join(l for l in t if l.strip() and not re.match(r'^\s*[vp]s_\d_\d\s*$', l))
body = bytearray()
def add(b):
    off = len(body); body.extend(b)
    while len(body) % 4: body.append(0)
    return off
def string(s): return add(struct.pack('<I', len(s) + 1) + s.encode() + b'\0')
empty = string('')
pblk = []
for name, cls, n in params:
    cols = 4 if cls == 1 else 1
    t = add(struct.pack('<7I', 3, cls, string(name), empty, n if n > 1 else 0, cols, 1))
    v = add(bytes(4 * cols * n))
    pblk.append(struct.pack('<4I', t, v, 0, 0))
t_vs = add(struct.pack('<5I', 16, 4, empty, empty, 0))
t_ps = add(struct.pack('<5I', 15, 4, empty, empty, 0))
tblk, res, obj = [], [], 1
for ti, (tname, vse, pse) in enumerate(techs):
    vs = assemble('vs_3_0', [c for c in consts if c[1] == 2], asm_text(vse))
    ps = assemble('ps_3_0', consts, asm_text(pse))
    vs_id, ps_id = obj, obj + 1; obj += 2
    states = [(146, 0, t_vs, add(struct.pack('<I', vs_id))), (147, 0, t_ps, add(struct.pack('<I', ps_id)))]
    tblk.append((string(tname), [(string('p0'), states)]))
    res += [(ti, 0, 0xffffffff, 0, 0, vs), (ti, 0, 0xffffffff, 1, 0, ps)]
start = len(body); out = bytearray(body)
out += struct.pack('<4I', len(pblk), len(tblk), 0, obj)
for p in pblk: out += p
for tn, passes in tblk:
    out += struct.pack('<3I', tn, 0, len(passes))
    for pn, states in passes:
        out += struct.pack('<3I', pn, 0, len(states))
        for s in states: out += struct.pack('<4I', *s)
out += struct.pack('<II', 0, len(res))
for tech, idx, elem, state, usage, data in res:
    out += struct.pack('<6I', tech, idx, elem, state, usage, len(data)) + data
    while len(out) % 4: out.append(0)
fx = struct.pack('<II', 0xFEFF0901, start) + bytes(out)
outp.write_bytes(fx)
print(outp, len(fx), 'bytes;', len(techs), 'technique(s);', 'constants', [c[0] for c in consts])
