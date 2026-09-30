#include "backend.hpp"
#include <cstdio>
#include <atomic>
namespace theater {
namespace {
thread_local const char* stage="idle";
PVOID handler=nullptr;
std::atomic<int> recorded=0;
LONG CALLBACK exception_trace(EXCEPTION_POINTERS* info) {
    auto code=info->ExceptionRecord->ExceptionCode;
    if(code!=EXCEPTION_ACCESS_VIOLATION && code!=EXCEPTION_ILLEGAL_INSTRUCTION && code!=EXCEPTION_STACK_OVERFLOW)
        return EXCEPTION_CONTINUE_SEARCH;
    auto active=stage;
    if(std::strcmp(active,"idle")==0 || recorded.fetch_add(1)>=8)return EXCEPTION_CONTINUE_SEARCH;
    HMODULE module=nullptr;char path[MAX_PATH]="unknown";
    auto pc=info->ExceptionRecord->ExceptionAddress;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<const char*>(pc),&module);
    if(module)GetModuleFileNameA(module,path,MAX_PATH);
    char line[1200];auto& ctx=*info->ContextRecord;
    int length=std::snprintf(line,sizeof(line),"%llu stage=%s exception=%08lx module=%s rva=%llx RIP=%llx RAX=%llx RBX=%llx RCX=%llx RDX=%llx RSP=%llx RBP=%llx access=%llu address=%llx\r\n",
        GetTickCount64(),active,code,path,reinterpret_cast<uintptr_t>(pc)-reinterpret_cast<uintptr_t>(module),
        ctx.Rip,ctx.Rax,ctx.Rbx,ctx.Rcx,ctx.Rdx,ctx.Rsp,ctx.Rbp,
        info->ExceptionRecord->NumberParameters>0?info->ExceptionRecord->ExceptionInformation[0]:0,
        info->ExceptionRecord->NumberParameters>1?info->ExceptionRecord->ExceptionInformation[1]:0);
    HANDLE file=CreateFileA("MVM/theater-crash.log",FILE_APPEND_DATA,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file!=INVALID_HANDLE_VALUE) {DWORD written;WriteFile(file,line,DWORD(std::min(length,int(sizeof(line)-1))),&written,nullptr);FlushFileBuffers(file);CloseHandle(file);}
    return EXCEPTION_CONTINUE_SEARCH;
}
}
void set_diagnostic_stage(const char* value){stage=value;}
void initialize_diagnostics(){handler=AddVectoredExceptionHandler(1,exception_trace);}
void shutdown_diagnostics(){if(handler)RemoveVectoredExceptionHandler(handler);handler=nullptr;}
}
