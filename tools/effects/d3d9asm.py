"""A minimal Direct3D 9 shader model 3.0 assembler (text -> bytecode with a CTAB), enough for the rebuilt effects. Syntax: one instruction per line, 'op dst, src, ...'; registers r#, v#, c#, o#, s#,
oC0, oPos; write masks .xyzw; swizzles .xyzw/.x; source negation '-'; '_sat' suffix on the opcode.
Declarations: 'dcl_<usage><index> reg' (usage: position normal texcoord tangent color) and
'dcl_2d s#'. 'def c#, a, b, c, d' (or vkd3d's 'def c# = a, b, c, d'). Constant table entries are given to assemble() as a list."""
import struct
OPS = {'mov':1,'add':2,'sub':3,'mad':4,'mul':5,'rcp':6,'rsq':7,'dp3':8,'dp4':9,'min':10,'max':11,'slt':12,'sge':13,
       'exp':14,'log':15,'frc':19,'pow':32,'crs':33,'abs':35,'nrm':36,'texld':66,'cmp':88,'dp2add':90,'dcl':31,'def':81}   # dp2add: vkd3d output
USAGE = {'position':0,'normal':3,'texcoord':5,'tangent':6,'color':10}
TYPES = {'r':0,'v':1,'c':2,'o':6,'s':10}
def reg_bits(t, n):
    return (n & 0x7ff) | ((t & 7) << 28) | (((t >> 3) & 3) << 11)
def parse_reg(tok):
    tok = tok.strip()
    if tok.startswith('oC0'): return 8, 0, tok.partition('.')[2]
    if tok.startswith('oPos'): raise ValueError('use an o# with dcl_position in vs_3_0')
    base, _, comp = tok.partition('.')
    t = TYPES[base[0]]; n = int(base[1:])
    return t, n, comp
def dst(tok):
    t, n, comp = parse_reg(tok)
    mask = 15 if not comp else sum(1 << 'xyzw'.index(ch) for ch in comp)
    return 0x80000000 | reg_bits(t, n) | (mask << 16)
def src(tok):
    tok = tok.strip(); neg = tok.startswith('-')
    if neg: tok = tok[1:]
    t, n, comp = parse_reg(tok)
    comp = comp or 'xyzw'
    comp = (comp + comp[-1] * 4)[:4]
    sw = sum('xyzw'.index(ch) << (2 * i) for i, ch in enumerate(comp))
    return 0x80000000 | reg_bits(t, n) | (sw << 16) | ((1 if neg else 0) << 24)
def ins(op, toks, sat=False):
    if op == 'def':
        body = [dst(toks[0])] + [struct.unpack('<I', struct.pack('<f', float(x)))[0] for x in toks[1:5]]
    else:
        d = dst(toks[0]) | ((1 << 20) if sat else 0)
        body = [d] + [src(x) for x in toks[1:]]
    return [OPS[op] | (len(body) << 24)] + body
def dcl(kind, reg):
    if kind == '2d':
        return [31 | (2 << 24), 0x80000000 | (2 << 27), dst(reg)]
    usage = next(u for u in USAGE if kind.startswith(u)); idx = int(kind[len(usage):] or 0)
    return [31 | (2 << 24), 0x80000000 | USAGE[usage] | (idx << 16), dst(reg)]
def ctab(target, consts):
    """consts: (name, set, index, count, cls, rows, cols, elements). set: 2 float4, 3 sampler.
    Returns the comment block words (Wine/fxc D3DXSHADER_CONSTANTTABLE layout)."""
    head = bytearray(28); infos = bytearray(20 * len(consts)); tail = bytearray()
    def add(b):
        nonlocal tail
        off = 28 + len(infos) + len(tail); tail += b
        while len(tail) % 4: tail += b'\0'
        return off
    creator = add(b'NFSMW native Test67\0')   # kept: the shipped effects were built with this creator string
    tgt = add(target.encode() + b'\0')
    for i, (name, rset, idx, cnt, cls, rows, cols, elems) in enumerate(consts):
        noff = add(name.encode() + b'\0')
        typ = 3 if rset == 2 else 12   # D3DXPT_FLOAT / D3DXPT_SAMPLER2D
        toff = add(struct.pack('<HHHHHHI', cls, typ, rows, cols, elems, 0, 0))
        struct.pack_into('<IHHHHII', infos, 20 * i, noff, rset, idx, cnt, 0, toff, 0)
    struct.pack_into('<IIIIIII', head, 0, 28, creator, 0xfffe0300 if target.startswith('vs') else 0xffff0300,
                     len(consts), 28, 0, tgt)
    data = b'CTAB' + bytes(head) + bytes(infos) + bytes(tail)
    while len(data) % 4: data += b'\0'
    n = len(data) // 4
    return [0xfffe | (n << 16)] + list(struct.unpack('<%dI' % n, data))
def assemble(target, consts, text):
    words = [0xfffe0300 if target == 'vs_3_0' else 0xffff0300] + ctab(target, consts)
    for line in text.strip().splitlines():
        line = line.split(';')[0].strip()
        if not line: continue
        op, _, rest = line.partition(' ')
        if op == 'def': rest = rest.replace('=', ',', 1)   # vkd3d-compiler writes 'def c# = a, b, c, d'
        toks = [t.strip() for t in rest.split(',')] if rest else []
        if op.startswith('dcl_'):
            words += dcl(op[4:], toks[0]); continue
        sat = op.endswith('_sat'); op = op[:-4] if sat else op
        words += ins(op, toks, sat)
    words.append(0x0000ffff)
    return struct.pack('<%dI' % len(words), *words)
