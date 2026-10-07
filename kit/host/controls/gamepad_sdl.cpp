// gamepad_sdl.cpp - see gamepad_sdl.h.
#include "gamepad_sdl.h"

#include "vpad.h"

#include <stdio.h>
#include <stdlib.h>

namespace controls {

namespace {

SDL_Gamepad *g_pad = nullptr;
SDL_JoystickID g_id = 0;
bool g_buttons[kSdlPadButtonCount] = {};
int16_t g_axes[kSdlAxisCount] = {};

bool (*g_suppressed)() = nullptr;
bool g_masked[kSdlPadButtonCount] = {}; // pressed for the host (the page toggle); hidden until released

// While something of the host's owns the pad (the settings page), the game sees a centred pad
// with nothing held, so a press that drives the page does not also drive the car. The real
// state is still tracked, and the next poll after the page closes hands it back.
void publish() {
    if (g_suppressed && g_suppressed()) {
        static const bool none[kSdlPadButtonCount] = {};
        static const int16_t centred[kSdlAxisCount] = {};
        vpad().set_source(kPadSourceController, pad_from_sdl(none, centred));
        return;
    }
    bool shown[kSdlPadButtonCount];
    for (int i = 0; i < kSdlPadButtonCount; ++i) {
        if (!g_buttons[i])
            g_masked[i] = false; // released, however the release was learnt
        shown[i] = g_buttons[i] && !g_masked[i];
    }
    vpad().set_source(kPadSourceController, pad_from_sdl(shown, g_axes));
}

// Reads every mapped button and axis from SDL's current state.
void read_all() {
    for (int i = 0; i < kSdlPadButtonCount; ++i)
        g_buttons[i] = SDL_GetGamepadButton(g_pad, SDL_GamepadButton(i));
    for (int i = 0; i < kSdlAxisCount; ++i)
        g_axes[i] = SDL_GetGamepadAxis(g_pad, SDL_GamepadAxis(i));
}

void open_pad(SDL_JoystickID id) {
    SDL_Gamepad *pad = SDL_OpenGamepad(id);
    if (!pad) {
        fprintf(stderr, "[controls] could not open gamepad %u: %s\n", unsigned(id), SDL_GetError());
        return;
    }
    g_pad = pad;
    g_id = id;
    const char *name = SDL_GetGamepadName(pad);
    const SDL_JoystickConnectionState connection = SDL_GetGamepadConnectionState(pad);
    const char *transport = connection == SDL_JOYSTICK_CONNECTION_WIRELESS ? "wireless"
                            : connection == SDL_JOYSTICK_CONNECTION_WIRED  ? "wired"
                                                                           : "unknown";
    fprintf(stderr, "[controls] gamepad connected: %s (%s, instance %u)\n",
            name ? name : "(unnamed)", transport, unsigned(id));
    // Which SDL driver serves it decides how rumble reaches the motors (HIDAPI
    // writes reports itself; MFi goes through Apple's GameController haptics).
    // The GUID's driver byte says which (SDL_joystick.c: 'h' HIDAPI, 'm' MFi,
    // 'v' virtual; otherwise the platform's IOKit driver).
    {
        char guid[33] = "";
        const SDL_GUID g = SDL_GetGamepadGUIDForID(id);
        SDL_GUIDToString(g, guid, sizeof guid);
        const char *backend = g.data[14] == 'h'   ? "HIDAPI"
                              : g.data[14] == 'm' ? "MFi/GameController"
                              : g.data[14] == 'v' ? "virtual"
                                                  : "IOKit";
        const char *path = SDL_GetGamepadPath(pad);
        fprintf(stderr, "[controls] gamepad backend: %s (guid %s%s%s)\n", backend, guid,
                path ? ", path " : "", path ? path : "");
    }
    const SDL_PropertiesID properties = SDL_GetGamepadProperties(pad);
    const bool rumble =
        SDL_GetBooleanProperty(properties, SDL_PROP_GAMEPAD_CAP_RUMBLE_BOOLEAN, false);
    fprintf(stderr, "[controls] gamepad rumble capability: %s; trigger rumble: %s\n",
            rumble ? "yes" : "no",
            SDL_GetBooleanProperty(properties, SDL_PROP_GAMEPAD_CAP_TRIGGER_RUMBLE_BOOLEAN, false)
                ? "yes"
                : "no");
    // An explicitly requested diagnostic pulse separates physical motor output
    // from guest force-feedback delivery. Never synthesize gameplay feedback.
    // Run at most once per process, even if a transport reconnects repeatedly.
    static bool tested_rumble = false;
    const char *test_rumble = getenv("RECOMP_GAMEPAD_RUMBLE_TEST");
    if (!tested_rumble && test_rumble && test_rumble[0] == '1' && test_rumble[1] == '\0') {
        tested_rumble = true;
        const bool ok = SDL_RumbleGamepad(pad, 16384, 16384, 250);
        fprintf(stderr, "[controls] diagnostic 250ms rumble pulse: %s%s%s\n",
                ok ? "sent" : "failed", ok ? "" : ": ", ok ? "" : SDL_GetError());
    }
    read_all();
    publish();
    vpad().set_controller_connected(true);
}

// Opens the first attached pad, if any, when none is open. `skip` is an
// instance SDL may still list although it is going away (the one whose
// SDL_EVENT_GAMEPAD_REMOVED is being handled); 0 skips nothing.
void open_any(SDL_JoystickID skip) {
    if (g_pad)
        return;
    int count = 0;
    SDL_JoystickID *ids = SDL_GetGamepads(&count);
    if (!ids)
        return;
    for (int i = 0; i < count && !g_pad; ++i)
        if (ids[i] != skip)
            open_pad(ids[i]);
    SDL_free(ids);
}

void close_pad() {
    // Zero first, so a mapped binding or a buffered reader sees every release
    // before the pad is reported gone.
    for (bool &b : g_buttons)
        b = false;
    for (int16_t &a : g_axes)
        a = 0;
    publish();
    vpad().set_controller_connected(false);
    // Best effort: a pad closed while still attached (another pad takes over,
    // or shutdown) must not keep running a motor it was last told to run.
    if (SDL_GamepadConnected(g_pad))
        SDL_RumbleGamepad(g_pad, 0, 0, 0);
    SDL_CloseGamepad(g_pad);
    g_pad = nullptr;
    g_id = 0;
    fprintf(stderr, "[controls] gamepad disconnected\n");
}

} // namespace

void gamepad_handle_event(const SDL_Event &e) {
    switch (e.type) {
    case SDL_EVENT_GAMEPAD_ADDED:
        if (!g_pad)
            open_pad(e.gdevice.which);
        break;
    case SDL_EVENT_GAMEPAD_REMOVED:
        if (g_pad && e.gdevice.which == g_id) {
            const SDL_JoystickID gone = g_id;
            close_pad();
            open_any(gone);
        }
        break;
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
    case SDL_EVENT_GAMEPAD_BUTTON_UP:
        if (g_pad && e.gbutton.which == g_id && e.gbutton.button < kSdlPadButtonCount) {
            g_buttons[e.gbutton.button] = e.gbutton.down;
            if (!e.gbutton.down)
                g_masked[e.gbutton.button] = false;
            publish();
        }
        break;
    case SDL_EVENT_GAMEPAD_AXIS_MOTION:
        if (g_pad && e.gaxis.which == g_id && e.gaxis.axis < kSdlAxisCount) {
            g_axes[e.gaxis.axis] = e.gaxis.value;
            publish();
        }
        break;
    default:
        break;
    }
}

void gamepad_poll() {
    // A transport change can detach an existing handle while another event
    // consumer drains its removal event. Do not keep polling a dead handle.
    if (g_pad && !SDL_GamepadConnected(g_pad))
        close_pad();
    // A pad whose SDL_EVENT_GAMEPAD_ADDED went somewhere else (the launcher
    // drains its own events) never reaches gamepad_handle_event, so while
    // nothing is open, look for one every kScanIntervalMs.
    if (!g_pad) {
        constexpr uint64_t kScanIntervalMs = 2000;
        static uint64_t next_scan_ms = 0;
        const uint64_t now_ms = SDL_GetTicks();
        if (now_ms >= next_scan_ms) {
            next_scan_ms = now_ms + kScanIntervalMs;
            open_any(0);
        }
    }
    if (!g_pad)
        return;
    read_all();
    publish();
}

void gamepad_mask_until_release(int button) {
    if (button >= 0 && button < kSdlPadButtonCount)
        g_masked[button] = true;
}

void gamepad_set_suppressed_query(bool (*suppressed)()) {
    g_suppressed = suppressed;
}

int gamepad_menu_key(const SDL_Event &e, bool *down) {
    if (e.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN || e.type == SDL_EVENT_GAMEPAD_BUTTON_UP) {
        if (g_pad && e.gbutton.which != g_id)
            return 0;
        *down = e.gbutton.down;
        switch (e.gbutton.button) {
        case SDL_GAMEPAD_BUTTON_DPAD_UP:
            return 0xc8;
        case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
            return 0xd0;
        case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
            return 0xcb;
        case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
            return 0xcd;
        case SDL_GAMEPAD_BUTTON_SOUTH:
            return 0x1c; // Enter
        case SDL_GAMEPAD_BUTTON_EAST:
            return 0x01; // Escape
        case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:
            return 0xc9; // Page Up: previous page
        case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:
            return 0xd1; // Page Down: next page
        default:
            return 0;
        }
    }
    if (e.type == SDL_EVENT_GAMEPAD_AXIS_MOTION &&
        (e.gaxis.axis == SDL_GAMEPAD_AXIS_LEFTX || e.gaxis.axis == SDL_GAMEPAD_AXIS_LEFTY)) {
        if (g_pad && e.gaxis.which != g_id)
            return 0;
        // The left stick as a D-pad, with hysteresis so a stick resting near the edge does not chatter.
        static int held[2] = {0, 0}; // -1, 0, +1 per axis
        const int a = e.gaxis.axis == SDL_GAMEPAD_AXIS_LEFTX ? 0 : 1;
        const int v = e.gaxis.value;
        const int dir = v > 20000 ? 1 : v < -20000 ? -1 : (v > -12000 && v < 12000) ? 0 : held[a];
        if (dir == held[a])
            return 0;
        const int was = held[a];
        // Straight from one side to the other: release the old side first; the next motion
        // event presses the new one. Every press the page saw gets its release.
        held[a] = was && dir ? 0 : dir;
        static const int keys[2][2] = {{0xcb, 0xcd}, {0xc8, 0xd0}};
        if (!was) {
            *down = true;
            return keys[a][dir > 0];
        }
        *down = false;
        return keys[a][was > 0];
    }
    return 0;
}

bool gamepad_connected() {
    return g_pad != nullptr;
}

bool gamepad_rumble(uint16_t low, uint16_t high, uint32_t ms) {
    return g_pad && SDL_RumbleGamepad(g_pad, low, high, ms);
}

} // namespace controls
