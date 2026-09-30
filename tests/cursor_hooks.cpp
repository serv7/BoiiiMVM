#include <Windows.h>
#include <MH/MinHook.h>
#include <cassert>
#include <iostream>
static int recenter_calls=0,clip_calls=0;
BOOL WINAPI blocked_recenter(int,int){++recenter_calls;return TRUE;}
BOOL WINAPI blocked_clip(const RECT*){++clip_calls;return TRUE;}
int main() {
    // Test the real Windows entry-point hooks without moving or confining the desktop cursor.
    assert(MH_Initialize()==MH_OK);
    void* recenter=reinterpret_cast<void*>(SetCursorPos);void* clip=reinterpret_cast<void*>(ClipCursor);
    void* original_recenter=nullptr;void* original_clip=nullptr;
    assert(MH_CreateHook(recenter,blocked_recenter,&original_recenter)==MH_OK);
    assert(MH_CreateHook(clip,blocked_clip,&original_clip)==MH_OK);
    assert(MH_EnableHook(recenter)==MH_OK && MH_EnableHook(clip)==MH_OK);
    assert(SetCursorPos(0,0));RECT rect{0,0,1,1};assert(ClipCursor(&rect));
    assert(recenter_calls==1 && clip_calls==1);
    assert(MH_DisableHook(recenter)==MH_OK && MH_DisableHook(clip)==MH_OK);
    assert(MH_RemoveHook(recenter)==MH_OK && MH_RemoveHook(clip)==MH_OK);
    MH_Uninitialize();std::cout<<"Cursor recenter / clip hook smoke test passed\n";
}
