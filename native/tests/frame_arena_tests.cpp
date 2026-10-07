#include "../../kit/mods/pop_mod_api.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>
#include "../../mods/core/nfsmw/frame_arena.h"
static std::map<uint32_t,uint32_t> mem;
static std::vector<std::string> logs;
static unsigned checks,failures,installed;
#define CHECK(x) do {++checks;if(!(x)){++failures;printf("FAIL %d %s\n",__LINE__,#x);}}while(0)
static PopModApi api{};
static void init(uint32_t cap) {
 mem.clear();logs.clear();fa_renders=fa_swaps=fa_bad=fa_reads=fa_fail_frames=fa_failed_bytes=0;
 fa_buffers[0]=fa_buffers[1]=fa_max_used=fa_capacity=0;fa_min_remaining=UINT32_MAX;fa_active_render=0;
 mem[0x915128]=0x1000000;mem[0x91512c]=0x2000000;mem[0x915f7c]=cap;
 mem[0x915f84]=0x1000000;mem[0x915f88]=0x1000000+cap;
}
static void frame(uint32_t used,uint32_t failed,bool bad_swap=false) {
 fa_render(&api,nullptr,nullptr,nullptr);
 mem[0x915f84]=mem[0x915128]+used;mem[0x915f94]=failed?1:0;mem[0x915f98]=failed;
 fa_swap(&api,nullptr,nullptr,nullptr);
 uint32_t a=mem[0x915128];mem[0x915128]=mem[0x91512c];mem[0x91512c]=a;
 mem[0x915f84]=mem[0x915128]+(bad_swap?16:0);mem[0x915f88]=mem[0x915128]+mem[0x915f7c];mem[0x915f94]=mem[0x915f98]=0;
 fa_swap(&api,nullptr,nullptr,(void*)1);++mem[0x982b78];fa_render(&api,nullptr,nullptr,(void*)1);
}
int main(){
 api.guest_read_u32=[](const PopModApi*,uint32_t a,uint32_t*v){*v=mem[a];return POP_OK;};
 api.log=[](const PopModApi*,const char*s){logs.emplace_back(s);return POP_OK;};
 api.hook_install_ex=[](const PopModApi*,uint32_t a,uint32_t ret,PopHookFn,int32_t mode,uint32_t flags,void*,uint32_t*h){CHECK(a==0x6e7220||a==0x4fad80);CHECK(ret==0);CHECK(mode==POP_HOOK_BEFORE||mode==POP_HOOK_AFTER);CHECK(flags==POP_HOOK_NO_GAME_VIEW);*h=++installed;return POP_OK;};
 api.hook_remove=[](const PopModApi*,uint32_t){return POP_OK;};
 unsetenv("NFSMW_FRAME_ARENA_DIAGNOSTICS");CHECK(fa_install(&api)==POP_OK);CHECK(!installed);
 setenv("NFSMW_FRAME_ARENA_DIAGNOSTICS","frames",1);CHECK(fa_install(&api)==POP_OK);CHECK(installed==4);
 for(uint32_t cap:{0x64000u,0x400000u}) {
  init(cap);frame(cap-64,560);frame(16000,0);
  CHECK(fa_renders==2&&fa_swaps==2);CHECK(fa_bad==0);CHECK(fa_reads==0);CHECK(fa_max_used==cap-64);
  CHECK(fa_min_remaining==64);CHECK(fa_fail_frames==1&&fa_failed_bytes==560);CHECK(mem[0x915128]==0x1000000);
  CHECK(logs.size()==4);CHECK(mem[0x915f84]==mem[0x915128]);CHECK(!mem[0x915f94]&&!mem[0x915f98]);
 }
 init(0x400000);frame(500000,0);CHECK(!fa_bad&&fa_max_used==500000&&fa_fail_frames==0);
 init(0x400000);frame(128,0,true);CHECK(fa_bad==1);
 init(0x400000);fa_render(&api,nullptr,nullptr,nullptr);++mem[0x982b78];fa_render(&api,nullptr,nullptr,(void*)1);CHECK(fa_bad==1);
 init(0x400000);mem[0x915f84]=mem[0x915f88]+16;fa_swap(&api,nullptr,nullptr,nullptr);CHECK(fa_bad==1);
 // Test-only ballast fixture: advances the fresh buffer's position through the
 // original bump pointer; refused when it would not fit.
 api.guest_write_u32=[](const PopModApi*,uint32_t a,uint32_t v){mem[a]=v;return POP_OK;};
 setenv("NFSMW_FRAME_ARENA_BALLAST","300008",1);CHECK(fa_install(&api)==POP_OK);CHECK(fa_ballast==300000);
 CHECK(logs.back().find("FA_BALLAST 300000")!=std::string::npos);
 init(0x400000);frame(0,0);CHECK(mem[0x915f84]==mem[0x915128]+300000);CHECK(fa_ballast_frames==1&&!fa_bad);
 init(0x64000);fa_ballast=0x64000;frame(0,0);CHECK(mem[0x915f84]==mem[0x915128]&&fa_ballast_errors==1);
 fa_ballast=0;unsetenv("NFSMW_FRAME_ARENA_BALLAST");
 fa_stop(&api);CHECK(logs.back().find("FA_END")!=std::string::npos);CHECK(logs.back().find("ballast_errors=1")!=std::string::npos);CHECK(!fa_hooks[0]);
 printf("frame arena: %u checks, %u failures\n",checks,failures);return failures?1:0;
}
