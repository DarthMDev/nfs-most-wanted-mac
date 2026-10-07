/* Test136 (3 Oct 2026): "Log my position" diagnostic, so a player can report where something happened (pop-in at the
 * highway gas station and the stadium trees, the Rosewood College sign flicker) by the clock time alone. With
 * position_log on, every 250 ms in the game world the log gets the local time (ms), the player car's world position
 * in the x,y,z (z up) form the QA fixture's QA_FIXTURE_TELEPORT takes, the heading and the speed (both from the
 * movement since the last line). 250 ms, not 2 s: at 100 mph two seconds is ~90 m, too coarse to place a pop-in
 * boundary (external review checklist review, 3 Oct). Read only: the position comes from the player's own getter (vtable +0x0c
 * of [[0092d87c]], as qa_fixture.c's QA_FIXTURE_POSLOG reads it). Called from game_device_update (once per frame; a
 * hook, so guest_call is allowed). */
#include <time.h>
#include <sys/time.h>
static void position_log(const PopModApi *api) {
    if (!setting("position_log", 0) || !api->guest_call) return;
    static double last_t, last_x, last_y;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    const double t = (double)tv.tv_sec + tv.tv_usec / 1e6;
    if (t - last_t < 0.25) return;
    const uint32_t player = get_u32(get_u32(0x0092d87cu));
    if (!player) return;
    const uint32_t vtable = get_u32(player);
    if (!vtable) return;
    uint32_t pos = 0;
    if (api->guest_call(api, get_u32(vtable + 0x0c), player, NULL, 0, &pos) != POP_OK || !pos) return;
    const double x = get_float(pos + 8), y = -get_float(pos), z = get_float(pos + 4);
    const double dt = t - last_t, dx = x - last_x, dy = y - last_y;
    double kmh = -1, heading = -1;
    if (last_t > 0 && dt < 1.0) {
        kmh = sqrt(dx * dx + dy * dy) / dt * 3.6;
        if (dx * dx + dy * dy > 0.01) { heading = atan2(dx, dy) * 57.29577951308232; if (heading < 0) heading += 360; }
    }
    last_t = t; last_x = x; last_y = y;
    struct tm tm;
    const time_t sec = tv.tv_sec;
    localtime_r(&sec, &tm);
    char line[200];
    snprintf(line, sizeof line, "core.nfsmw: position %02d:%02d:%02d.%03d world %.1f,%.1f,%.1f heading %.0f speed %.0f km/h",
             tm.tm_hour, tm.tm_min, tm.tm_sec, (int)(tv.tv_usec / 1000), x, y, z, heading, kmh);
    api->log(api, line);
}
