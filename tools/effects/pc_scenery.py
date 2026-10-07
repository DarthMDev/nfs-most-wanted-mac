"""Read the scenery sections of the PC game's own track data (TRACKS/STREAML2RA.BUN): each section's number, its
SceneryInfo names and its scenery instances. Read only; used by pools_bin.py."""
import struct
from pathlib import Path


def walk_sections(data):
    out = []

    def walk(s, e, depth, cur):
        p = s
        while p + 8 <= e:
            cid, sz = struct.unpack_from('<II', data, p)
            if p + 8 + sz > e or depth > 20:
                return
            if cid == 0x80034100:
                sec = dict(chunks={})
                out.append(sec)
                walk(p + 8, p + 8 + sz, depth + 1, sec)
            else:
                if cur is not None and not (cid & 0x80000000):
                    cur['chunks'].setdefault(cid, data[p + 8:p + 8 + sz])
                if cid & 0x80000000:
                    walk(p + 8, p + 8 + sz, depth + 1, cur)
            p += 8 + sz

    walk(0, len(data), 0, None)
    return out


def pc_sections(game_dir):
    """The scenery sections of <game_dir>/TRACKS/STREAML2RA.BUN."""
    return walk_sections((Path(game_dir) / 'TRACKS' / 'STREAML2RA.BUN').read_bytes())


def section_number(sec):
    h = sec['chunks'].get(0x34101)
    return struct.unpack_from('<i', h, 0x0c)[0] if h and len(h) >= 16 else None


def infos(sec):
    """SceneryInfo names (0x34102: 0x48-byte records, the name is the first 24 bytes)."""
    d = sec['chunks'].get(0x34102, b'')
    return [d[i:i + 24].split(b'\0')[0].decode('latin-1') for i in range(0, len(d) - 0x47, 0x48)]


def instances(sec):
    """Instance records (0x34103, 0x40 bytes): bmin[3] 0x00, bmax[3] 0x0c, flags 0x18, preculler 0x1c, light context
    0x1e, position[3] 0x20, rotation[9] int16 0x2c, SceneryInfo number 0x3e."""
    d = sec['chunks'].get(0x34103, b'')
    q = 0
    while q < len(d) and d[q:q + 4] == b'\x11\x11\x11\x11':
        q += 4
    out = []
    for o in range(q, len(d) - 0x3f, 0x40):
        bmin = struct.unpack_from('<3f', d, o)
        bmax = struct.unpack_from('<3f', d, o + 0xc)
        pos = struct.unpack_from('<3f', d, o + 0x20)
        rot = struct.unpack_from('<9h', d, o + 0x2c)
        info, = struct.unpack_from('<h', d, o + 0x3e)
        out.append(dict(bmin=bmin, bmax=bmax, pos=pos, rot=rot, info=info))
    return out
