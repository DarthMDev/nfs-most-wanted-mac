#include "../../kit/mods/pop_mod_api.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>
#include "../../mods/core/nfsmw/vehicle_admission.h"
static std::map<uint32_t,uint32_t> mem;
static std::vector<std::string> logs;
static unsigned checks, failures, originals, installs;
static int scenario;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; printf("FAIL %d: %s\n",__LINE__,#x); } } while(0)
static PopModApi api{};
static PopModStatus original(const PopModApi *a,uint32_t addr,pop_cpu_v1 *c) {
    CHECK(addr==0x74e160); ++originals;
    uint32_t f=((c->esp-4)&~15u)-0x600;
    mem[f+0x10]=100;
    if(scenario==1) mem[f+0x10]=1;
    else if(scenario==2) {mem[0x915f98]+=0x40;mem[0x915f84]+=0x1f0;}
    else {
        c->eax=scenario==3 ? 0:2; va_result(a,c,nullptr,nullptr);
        if(scenario==5) {c->eax=1;va_result(a,c,nullptr,(void*)1);}
    }
    c->eax=(scenario==0 ? 0x12340001:0x12340000);
    if(scenario==0) mem[0x982cdc]++;
    c->esp+=52; c->eip=0x7511ba;
    return POP_OK;
}
static void fixture(int n) {
    scenario=n;mem.clear();logs.clear();va_read_errors=va_dropped=va_bytes=va_calls=0;
    mem[0x90000]=0x7511ba;mem[0x90004]=0x2000;
    mem[0x2004]=1;mem[0x2024]=10;mem[0x915f84]=0x800000;mem[0x915f88]=0x864000;
    mem[0x93e878]=0x4000;mem[0x93e880]=0x5000;mem[0x5000]=0x6000;
    mem[0x4054]=0xdeadbeef;mem[0x6028]=0xabcdef;
    pop_cpu_v1 c{};c.target=0x74e160;c.ecx=0x1000;c.esp=0x90000;c.edi=0x7000;c.ebx=77;
    va_observe(&api,&c,nullptr,nullptr);
    CHECK(c.eax==(n==0?0x12340001u:0x12340000u));CHECK(c.esp==0x90034);CHECK(c.ebx==77);CHECK(c.ecx==0x1000);CHECK(!va_active);
}
int main() {
    api.size=sizeof api;
    api.guest_read_u32=[](const PopModApi*,uint32_t a,uint32_t*v){*v=mem[a];return POP_OK;};
    api.log=[](const PopModApi*,const char*s){logs.emplace_back(s);return POP_OK;};
    api.call_original=original;
    api.hook_install_ex=[](const PopModApi*,uint32_t a,uint32_t r,PopHookFn,int32_t m,uint32_t f,void*,uint32_t*out){
        CHECK(f==POP_HOOK_NO_GAME_VIEW);
        CHECK((a==0x74e160&&r==0&&m==POP_HOOK_REPLACE)||(a==0x4fca70&&r==0x74e34b&&m==POP_HOOK_AFTER)||(a==0x45fde0&&r==0x74e691&&m==POP_HOOK_AFTER));
        *out=++installs;return POP_OK;};
    api.hook_remove=[](const PopModApi*,uint32_t){return POP_OK;};
    unsetenv("NFSMW_VEHICLE_ADMISSION");CHECK(va_install(&api)==POP_OK);CHECK(installs==0);
    setenv("NFSMW_VEHICLE_ADMISSION","1",1);CHECK(va_install(&api)==POP_OK);CHECK(installs==3);
    const char*reasons[]={"returned_true","projected_size","initial_scratch_allocation","frustum","both_lods_unavailable","main_camera_inside_bounds"};
    for(int n=0;n<6;++n){fixture(n);CHECK(logs.size()==1);CHECK(logs[0].find(reasons[n])!=std::string::npos);CHECK(va_read_errors==0);}
    fixture(0);CHECK(logs[0].find("3735928559,11259375")!=std::string::npos);CHECK(logs[0].find("\"render_connection\":28672")!=std::string::npos);
    VAContext c{};CHECK(!strcmp(va_reason(0,&c,0,100,10,1),"unexplained_pre_frustum"));
    CHECK(!strcmp(va_reason(0,&c,0,1,10,15),"unexplained_pre_frustum"));
    c.frustum_seen=1;c.frustum=2;c.inside_seen=1;c.inside=0;CHECK(!strcmp(va_reason(0,&c,0,100,10,1),"unexplained_after_frustum"));
    c.frustum_seen=0;c.inside_seen=0;pop_cpu_v1 cpu{};cpu.eax=7;va_active=&c;va_result(&api,&cpu,nullptr,nullptr);CHECK(c.frustum==7&&cpu.eax==7);va_active=nullptr;
    fixture(0);va_bytes=VA_LOG_LIMIT;cpu.target=0x74e160;cpu.ecx=0x1000;cpu.esp=0x90000;va_observe(&api,&cpu,nullptr,nullptr);CHECK(va_dropped==1);CHECK(logs.back().find("VA_OVERFLOW")!=std::string::npos);
    va_stop(&api);CHECK(logs.back().find("VA_END")!=std::string::npos);CHECK(!va_hooks[0]);
    printf("vehicle admission: %u checks, %u failures; original calls=%u\n",checks,failures,originals);return failures?1:0;
}
