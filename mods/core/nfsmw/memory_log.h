/* Test151 (3 Oct 2026): once a minute, the game's memory and the Mac's swap in the session log.
 * Owner sessions 3 Oct ended twice with no fault line and no crash report (07:24:57, 09:52:41). At 09:52:41 macOS
 * killed several other processes with the jetsam reason "low-swap" in the same second the game died (unified log),
 * and swap stood at 4.2 of 5.1 GB afterwards. This line puts the evidence in our own log: the game's footprint
 * (task_info TASK_VM_INFO phys_footprint, what Activity Monitor calls Memory), its resident size, and swap used/total
 * (sysctl vm.swapusage). Cheap (one line a minute; every 5 s above 6 GB); setting memory_log (diagnostics, on). Called from
 * game_device_update; host-side reads only, no guest state. */
#ifdef __APPLE__
#include <mach/mach.h>
#include <sys/sysctl.h>
#endif
#ifdef __APPLE__
#include <sys/time.h>
#include <unistd.h>
#include <malloc/malloc.h>
/* Test155 (4 Oct): the 20:49 event grew 3.3 -> 326 GB between two minute lines. Four times a second (one task_info
 * call), a growth of more than 1 GB since the previous sample is logged at once with the time to the millisecond, so
 * the jump can be placed against the frame log, the rain lines and a pause. */
static void memory_jump_watch(const PopModApi *api) {
    static double last_t, last_mb;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    const double t = (double)tv.tv_sec + tv.tv_usec / 1e6;
    if (t - last_t < 0.25) return;
    task_vm_info_data_t vm;
    mach_msg_type_number_t count = TASK_VM_INFO_COUNT;
    if (task_info(mach_task_self(), TASK_VM_INFO, (task_info_t)&vm, &count) != KERN_SUCCESS) return;
    const double mb = vm.phys_footprint / 1048576.0;
    if (last_t > 0 && mb - last_mb > 1024.0) {
        struct tm tm;
        const time_t sec = tv.tv_sec;
        localtime_r(&sec, &tm);
        /* Only on a jump: the malloc heaps' bytes in use, so a heap runaway can be told from GPU/IOKit memory. */
        malloc_statistics_t ms = {0};
        malloc_zone_statistics(NULL, &ms);
        char line[240];
        snprintf(line, sizeof line, "core.nfsmw: memory JUMP %02d:%02d:%02d.%03d game %.0f -> %.0f MB in %.0f ms (resident %.0f MB, "
                 "malloc in use %.0f MB)",
                 tm.tm_hour, tm.tm_min, tm.tm_sec, (int)(tv.tv_usec / 1000), last_mb, mb, (t - last_t) * 1000.0,
                 vm.resident_size / 1048576.0, ms.size_in_use / 1048576.0);
        api->log(api, line);
    }
    /* Development safety stop (Test155, external review 4 Oct): the game normally stays near 3 GB. Past 8 GB of footprint
     * (external review: 16 GB adds no evidence at 13 GB/s) it is in the rain runaway (21:14: +13 GB/s), and macOS would soon kill it and other apps for low swap. Stop
     * the game first, saying so; the kit's "ALLOCATION GROWTH" lines are already in the log by then. */
    if (mb > 8192.0) {
        char line[200];
        snprintf(line, sizeof line, "core.nfsmw: SAFETY STOP (Test155 diagnostic): game footprint %.0f MB (resident %.0f MB); "
                 "ending the game before macOS runs out of swap", mb, vm.resident_size / 1048576.0);
        api->log(api, line);
        fflush(stdout);
        fflush(stderr);
        usleep(300000);
        _exit(75);
    }
    last_t = t;
    last_mb = mb;
}
#endif
static void memory_log(const PopModApi *api) {
#ifdef __APPLE__
    if (!setting("memory_log", 1)) return;
    memory_jump_watch(api);
    static time_t last;
    static double last_foot;
    const time_t now = time(NULL);
    /* Once a minute; every 5 s once the footprint is above 6 GB or grew by 2 GB since the last line (Test155:
     * 3.1 GB -> 361 GB within one minute before the 19:54 exit). */
    const int fast = last_foot > 6144.0;
    if (last && now - last < (fast ? 5 : 60)) return;
    last = now;
    task_vm_info_data_t vm;
    mach_msg_type_number_t count = TASK_VM_INFO_COUNT;
    double foot = -1, res = -1;
    if (task_info(mach_task_self(), TASK_VM_INFO, (task_info_t)&vm, &count) == KERN_SUCCESS) {
        foot = vm.phys_footprint / 1048576.0;
        res = vm.resident_size / 1048576.0;
    }
    const int jump = last_foot > 0 && foot - last_foot > 2048.0;
    last_foot = foot;
    struct xsw_usage sw;
    size_t len = sizeof sw;
    double used = -1, total = -1;
    if (sysctlbyname("vm.swapusage", &sw, &len, NULL, 0) == 0) {
        used = sw.xsu_used / 1048576.0;
        total = sw.xsu_total / 1048576.0;
    }
    struct tm tm;
    localtime_r(&now, &tm);
    char line[200];
    snprintf(line, sizeof line, "core.nfsmw: memory %02d:%02d:%02d game %.0f MB (resident %.0f MB), Mac swap %.0f of %.0f MB used%s",
             tm.tm_hour, tm.tm_min, tm.tm_sec, foot, res, used, total, jump ? "  WARNING: grew by more than 2 GB" : "");
    api->log(api, line);
#else
    (void)api;
#endif
}
