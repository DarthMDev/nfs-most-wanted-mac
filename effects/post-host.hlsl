// post-host.hlsl - the post-process host's own technique (mods/core/nfsmw/post_fx.h).
// alpha_one writes alpha 1 over the frame and leaves colour alone (the host sets COLORWRITEENABLE to alpha only):
// the HDR-output scene mark. No constants, no samplers.
// Built by tools/effects/fxgen.py from the shader-model-3 assembly beside this file:
//   python3 tools/effects/fxgen.py effects/post-host.hlsl effects mods/core/nfsmw/post-host.fx
// technique alpha_one: vs=VS_Pass ps=PS_AlphaOne
