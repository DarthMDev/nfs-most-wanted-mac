# Contributing

This port targets **Apple Silicon Macs only**. It is built on [recomp-kit](https://github.com/veritr1x/recomp-kit)
(included in `kit/`, with this port's changes) and veritr1x's [nfsmw-recomp](https://github.com/veritr1x/nfsmw-recomp) configuration.
Runtime, translator and host changes that are not specific to this game belong upstream in recomp-kit.

## Rules

- **No game data, ever.** Never commit or attach `speed.exe`, game files, extracted textures, movies, saves, logs
  that contain game data, or anything the setup generates from your game (`build/`, `analysis/`, `original/`,
  `build/generated/`). `.gitignore` covers these; keep it that way.
- **No code or data from console executables.** Behaviour is implemented against the PC 1.3 executable. Community
  mods may be used as references for behaviour and addresses only when their licence allows; record them in NOTICE.
- **One executable.** The translation (`game.toml`) is tied to one `speed.exe` build; the setup's check keeps other
  builds out because they would not work. Do not add anything that removes or works around copy protection.

## Build for development

Run `./setup.sh "/path/to/your game"` once. Afterwards:

```sh
.venv/bin/python tools/build.py            # rebuild after a change (add --regenerate after editing game.toml)
.venv/bin/python tools/build.py --target plugins
.venv/bin/python -m pytest -q tests        # this repository's config tests
.venv/bin/python tools/test.py             # the kit's portable suites; no game files required
.venv/bin/python tools/setup_kit/settings_doc.py   # after changing mod.toml: regenerate docs/SETTINGS.md
```

The core mod lives in `mods/core/nfsmw/` (`nfsmw.c` and its headers; settings in `mod.toml`). The rebuilt effects'
sources are in `effects/`; `tools/effects/fxgen.py` turns their shader assembly into the `.fx` files the mod loads.
After changing `effects/clean-chain.*` or `effects/post-host.*`, rebuild the `.fx` files in `mods/core/nfsmw/` and
commit them together with their sources.

## Reporting bugs

Open an issue with your Mac model, macOS version, what you did and what you expected, and the run's log from
`~/Library/Application Support/NFS Most Wanted Native/Logs/`.
