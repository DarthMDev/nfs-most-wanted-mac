/* Retail PC 1.3 frame-arena diagnostics. Dormant unless explicitly enabled.
 * 006e7220 calls 004fad80 once, then increments00982b78. Observe only; never
 * change arena pointers, allocation comparisons or swap/reset timing.
 * The single exception is the test-only ballast fixture: with diagnostics on
 * AND NFSMW_FRAME_ARENA_BALLAST=<bytes>, each swap advances the fresh buffer's
 * position by that many bytes, so the game's real allocations must fit above
 * it. This adds demand through the original bump pointer and strict end
 * comparison (Test47). Never set by any launcher or packaged default. */
typedef struct FAState {
    uint32_t frame, start, other, pos, end, capacity, failed, failed_bytes;
} FAState;
static uint32_t fa_hooks[4], fa_buffers[2];
static FAState fa_before;
static unsigned long long fa_renders, fa_swaps, fa_bad, fa_reads, fa_fail_frames, fa_failed_bytes;
static uint32_t fa_max_used, fa_min_remaining = UINT32_MAX, fa_capacity, fa_render_frame;
static unsigned long long fa_render_swaps;
static int fa_active_render, fa_rows;
static uint32_t fa_ballast;
static unsigned long long fa_ballast_frames, fa_ballast_errors;
static uint32_t fa_read(const PopModApi *api, uint32_t address) {
    uint32_t value = 0;
    if (api->guest_read_u32(api, address, &value) != POP_OK) ++fa_reads;
    return value;
}
static FAState fa_state(const PopModApi *api) {
    FAState s = {fa_read(api,0x00982b78),fa_read(api,0x00915128),fa_read(api,0x0091512c),
        fa_read(api,0x00915f84),fa_read(api,0x00915f88),fa_read(api,0x00915f7c),
        fa_read(api,0x00915f94),fa_read(api,0x00915f98)};
    return s;
}
static void fa_row(const PopModApi *api, const FAState *s) {
    if (!fa_rows) return;
    char text[320];
    snprintf(text,sizeof text,"FA %u,%u,%u,%u,%u,%u,%u,%u,%u",s->frame,
             s->start == fa_buffers[0] ? 0u : 1u,s->start,s->pos,s->end,
             s->pos-s->start,s->end-s->pos,s->failed,s->failed_bytes);
    api->log(api,text);
}
static void fa_render(const PopModApi *api, pop_cpu_v1 *cpu, PopHookInvocation *inv, void *after) {
    (void)cpu; (void)inv;
    uint32_t frame=fa_read(api,0x00982b78);
    if (!after) {
        if (fa_active_render) ++fa_bad;
        fa_active_render=1;fa_render_frame=frame;fa_render_swaps=fa_swaps;++fa_renders;
    } else {
        if (!fa_active_render || fa_swaps!=fa_render_swaps+1 || frame!=fa_render_frame+1) ++fa_bad;
        fa_active_render=0;
    }
}
static void fa_swap(const PopModApi *api, pop_cpu_v1 *cpu, PopHookInvocation *inv, void *after) {
    (void)cpu; (void)inv;
    FAState s=fa_state(api);
    if (!after) {
        fa_before=s;
        if (!fa_buffers[0]) {fa_buffers[0]=s.start;fa_buffers[1]=s.other;fa_capacity=s.capacity;}
        if (!s.start || !s.other || s.start==s.other || (s.start&15) || (s.other&15) ||
            s.pos<s.start || s.pos>s.end || s.end-s.start!=s.capacity || s.capacity!=fa_capacity ||
            !((s.start==fa_buffers[0]&&s.other==fa_buffers[1]) ||
              (s.start==fa_buffers[1]&&s.other==fa_buffers[0]))) ++fa_bad;
        uint32_t used=s.pos-s.start, remaining=s.end-s.pos;
        if (used>fa_max_used) fa_max_used=used;
        if (remaining<fa_min_remaining) fa_min_remaining=remaining;
        if (s.failed) ++fa_fail_frames;
        fa_failed_bytes+=s.failed_bytes;
    } else {
        ++fa_swaps;
        if (!fa_active_render || s.frame!=fa_before.frame || s.start!=fa_before.other ||
            s.other!=fa_before.start || s.pos!=s.start || s.end-s.start!=s.capacity ||
            s.failed || s.failed_bytes) ++fa_bad;
    }
    fa_row(api,&s);
    if (after && fa_ballast) {
        if (fa_ballast < s.capacity && s.pos == s.start &&
            api->guest_write_u32(api,0x00915f84,s.start+fa_ballast) == POP_OK) ++fa_ballast_frames;
        else ++fa_ballast_errors;
    }
}
static PopModStatus fa_install(const PopModApi *api) {
    const char *v=getenv("NFSMW_FRAME_ARENA_DIAGNOSTICS");
    if (!v || (strcmp(v,"1") && strcmp(v,"frames"))) return POP_OK;
    fa_rows=!strcmp(v,"frames");
    const char *b=getenv("NFSMW_FRAME_ARENA_BALLAST");
    if (b && *b) fa_ballast=(uint32_t)strtoul(b,NULL,10)&~15u;
    if (!api->hook_install_ex) return POP_E_STATE;
    const uint32_t addresses[4]={0x006e7220,0x006e7220,0x004fad80,0x004fad80};
    for (unsigned i=0;i<4;++i) {
        PopModStatus status=api->hook_install_ex(api,addresses[i],0,i<2?fa_render:fa_swap,
            i&1?POP_HOOK_AFTER:POP_HOOK_BEFORE,POP_HOOK_NO_GAME_VIEW,
            i&1?(void*)1:NULL,&fa_hooks[i]);
        if (status!=POP_OK) {
            for(unsigned j=0;j<i;++j) {api->hook_remove(api,fa_hooks[j]);fa_hooks[j]=0;}
            return status;
        }
    }
    api->log(api,"FA_START render_frame,active_buffer,buffer_start,current_pos,buffer_end,used_bytes,remaining_bytes,FrameMallocFailed,FrameMallocFailAmount; paired before/after swap");
    if (fa_ballast) {
        char text[160];
        snprintf(text,sizeof text,"FA_BALLAST %u bytes per frame (TEST FIXTURE: used_bytes include it)",fa_ballast);
        api->log(api,text);
    }
    return POP_OK;
}
static void fa_stop(const PopModApi *api) {
    if (!fa_hooks[0]) return;
    char text[480];
    snprintf(text,sizeof text,"FA_END capacity=%u maximum_bytes_used=%u minimum_bytes_remaining=%u failure_frames=%llu failed_byte_total=%llu render_calls=%llu swaps=%llu cadence_or_range_errors=%llu read_errors=%llu active_render=%d ballast=%u ballast_frames=%llu ballast_errors=%llu",
        fa_capacity,fa_max_used,fa_min_remaining,fa_fail_frames,fa_failed_bytes,
        fa_renders,fa_swaps,fa_bad,fa_reads,fa_active_render,fa_ballast,fa_ballast_frames,fa_ballast_errors);
    api->log(api,text);
    for(unsigned i=0;i<4;++i) if(fa_hooks[i]) {api->hook_remove(api,fa_hooks[i]);fa_hooks[i]=0;}
}
