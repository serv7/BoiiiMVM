#include <Windows.h>

#include <assets/utils.h>
#include "mvm.h"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
            g_moduleHandle = hModule, mvm::initialize();
        case DLL_PROCESS_DETACH:
            mvm::shutdown();
            break;
    }
    return TRUE;
}