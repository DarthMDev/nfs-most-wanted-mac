# Changelog

## 1.0.0 — first public release

The native Mac port of Need for Speed: Most Wanted (2005), with the setup kit that builds it from the player's own
PC game folder. See the README for the feature list.

- **Setup app:** a small Mac app (on the release page) that builds the game from your own game folder: choose the
  folder, click Build, then open the game. It installs Apple's Command Line Tools if needed, shows the progress and
  keeps the Mac awake. `./setup.sh` in Terminal does the same.
- Rebuilt post-process effects (glow, auto brightness, motion blur, depth of field, edge darkening, shadow lift)
  run on the port's own effects host and need only the PC game.
- Light pools are built at setup from the PC game's own lamps and effects.
- Optional: TexWizard texture packs (for example the Xbox 360 Stuff Pack's textures) and the XenonEffects sparks
  and light trails, converted by the setup from the player's own copies.
- Skip the music track while driving (T, or L3 on a controller), as Extra Options' SkipTrackAnywhere; both on by
  default.
- Extra Options camera and screenshot keys (all off by default): freeze camera (F9 or Pause), free camera
  (Backspace), light keys (H, O), save/load positions; plus unlock everything and barrier removal.
- Fix: a draw using a volume texture with a level-of-detail clamp no longer shares its state version with the
  pipeline, so the render queue can no longer carry its raised mip level into the next draw (or reuse an older
  table for it).
