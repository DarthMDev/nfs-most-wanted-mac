"""Arena backing allocations, initial bound and later swap size must agree."""
from pathlib import Path
import re,tomllib

def test_frame_arena_backing_and_bounds_agree():
    root=Path(__file__).resolve().parents[1]
    cfg=tomllib.loads((root/'game.toml').read_text())
    patches={p['at']:p['text'] for p in cfg['translate']['instruction_patches']}
    addresses=(0x5009d1,0x5009db,0x500a00,0x500a0c)
    sizes=[int(re.search(r'0x([0-9a-f]+)$',patches[a]).group(1),16) for a in addresses]
    assert len(set(sizes))==1, 'allocations, initial end and subsequent swap capacity diverge'
    assert sizes[0]==4*1024*1024 and sizes[0]%16==0
    assert sizes[0]*2<cfg['game']['guest_size']
    assert patches[0x5009d1].startswith('PUSH ') and patches[0x5009db].startswith('PUSH ')
    assert patches[0x500a00].startswith('ADD EAX,')
    assert patches[0x500a0c].startswith('MOV dword ptr [0x00915f7c],')
    assert not any(0x4fad80<=a<=0x4fae17 or 0x405ed0<=a<=0x405f15 for a in patches), 'swap or exact-fit policy changed'
