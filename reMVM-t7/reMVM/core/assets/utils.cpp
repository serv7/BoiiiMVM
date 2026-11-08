#include "utils.h"
#include "memory.h"
#include <Psapi.h>

uintptr_t utils::signature::address_of(uintptr_t _instruction_addr)
{
    if (_instruction_addr < 0x1000)
        return 0;

    uint16_t instruction_size = 6;
    uint16_t offset_to_relative = 2;

    auto opcode = (unsigned char*)(_instruction_addr);

    if (opcode[0] >= 0x48 && opcode[0] <= 0x4F)
    {
        instruction_size++;
        offset_to_relative++;
    }
    else if (opcode[0] == 0x80)
    {
        instruction_size = 7;
        offset_to_relative = 2;
    }
    else if (opcode[0] == 0xE8 || opcode[0] == 0xE9)
    {
        instruction_size = 5;
        offset_to_relative = 1;
    }

    return _local::address_of(_instruction_addr, instruction_size, *reinterpret_cast<long*>(_instruction_addr + offset_to_relative));
}

uintptr_t utils::signature::_local::find(HMODULE _hModule, uintptr_t _start_address, const char* _signature)
{
#define INRANGE(x,a,b)  (x >= a && x <= b) 
#define GET_BITS( x )    (INRANGE((x&(~0x20)),'A','F') ? ((x&(~0x20)) - 'A' + 0xa) : (INRANGE(x,'0','9') ? x - '0' : 0))
#define GET_BYTE( x )    (GET_BITS(x[0]) << 4 | GET_BITS(x[1]))

    uintptr_t end_eddress = (_start_address + uintptr_t(memory::module_size(_hModule)));

    const char* pattern = _signature;
    uintptr_t first_match = 0;

    for (uintptr_t current_address = _start_address; current_address < end_eddress; current_address++)
    {
        if (!*pattern)
            return first_match;

        if (*(BYTE*)pattern == ('\?') || *(BYTE*)current_address == GET_BYTE(pattern))
        {
            if (!first_match)
                first_match = current_address;

            if (!pattern[2])
                return first_match;

            if (*(uint16_t*)pattern == ('\?\?') || *(BYTE*)pattern != ('\?'))
                pattern += 3;
            else
                pattern += 2;
        }
        else
        {
            pattern = _signature;
            first_match = 0;
        }
    }

    return 0;

#undef INRANGE
#undef GET_BITS
#undef GET_BYTE
}

uintptr_t utils::signature::_local::address_of(uintptr_t instruction_addr, uintptr_t instruction_size, long relative)
{
    return instruction_addr + relative + instruction_size;
}

uintptr_t utils::signature::find(const char* _signature) { return find(MODULE_HANDLE, _signature); }
uintptr_t utils::signature::find(const char* _signature, int _index) { return find(MODULE_HANDLE, _signature, _index); }

uintptr_t utils::signature::find(HMODULE _hModule, const char* _signature)
{
#define INRANGE(x,a,b)  (x >= a && x <= b) 
#define GET_BITS( x )    (INRANGE((x&(~0x20)),'A','F') ? ((x&(~0x20)) - 'A' + 0xa) : (INRANGE(x,'0','9') ? x - '0' : 0))
#define GET_BYTE( x )    (GET_BITS(x[0]) << 4 | GET_BITS(x[1]))

    MODULEINFO module_info;
    K32GetModuleInformation(GetCurrentProcess(), _hModule, &module_info, sizeof(MODULEINFO));

    uintptr_t start_address = uintptr_t(_hModule);
    uintptr_t end_eddress = (start_address + uintptr_t(module_info.SizeOfImage));

    const char* pattern = _signature;
    uintptr_t first_match = 0;

    for (uintptr_t current_address = start_address; current_address < end_eddress; current_address++)
    {
        if (!*pattern)
            return first_match;

        if (*(BYTE*)pattern == ('\?') || *(BYTE*)current_address == GET_BYTE(pattern))
        {
            if (!first_match)
                first_match = current_address;

            if (!pattern[2])
                return first_match;

            if (*(uint16_t*)pattern == ('\?\?') || *(BYTE*)pattern != ('\?'))
                pattern += 3;
            else
                pattern += 2;
        }
        else
        {
            pattern = _signature;
            first_match = 0;
        }
    }

    return 0;

#undef INRANGE
#undef GET_BITS
#undef GET_BYTE
}

uintptr_t utils::signature::find(HMODULE hModule, const char* signature, int index)
{
    int i = 0;

    uintptr_t start_address = uintptr_t(hModule);
    uintptr_t address = 0;

    while (i < index)
    {
        start_address = _local::find(hModule, start_address + 1, signature);
        i++;
    }

    if (i == index)
        return _local::find(hModule, start_address + 1, signature);

    return 0;
}

#include <codecvt>
#include <sstream>
void utils::string::split(std::vector<std::string>* _out, const std::string& _str, char _delim)
{
    std::size_t current, previous = 0;
    current = _str.find(_delim);
    while (current != std::string::npos) {
        _out->push_back(_str.substr(previous, current - previous));
        previous = current + 1;
        current = _str.find(_delim, previous);
    }
    _out->push_back(_str.substr(previous, current - previous));
}

std::string utils::string::vk_to_string(unsigned long key)
{
    unsigned int scan_code = MapVirtualKey(key, MAPVK_VK_TO_VSC);

    switch (key)
    {
    case VK_LEFT: case VK_UP: case VK_RIGHT: case VK_DOWN: // arrow keys
    case VK_PRIOR: case VK_NEXT: // page up and page down
    case VK_END: case VK_HOME:
    case VK_INSERT: case VK_DELETE:
    case VK_DIVIDE: // numpad slash
    case VK_NUMLOCK:
    {
        scan_code |= 0x100; // set extended bit
        break;
    }
    }

    wchar_t key_name[50];
    memset(key_name, 0, 100);

    int length = GetKeyNameTextW(scan_code << 16, (LPWSTR)&key_name, 50);
    key_name[length] = L'\0';

    std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8_conv;
    return utf8_conv.to_bytes(key_name);
}

std::string utils::string::lparam_to_string(LPARAM lparam)
{
    unsigned int scanCode = (lparam >> 16) & 0xFF;
    if ((lparam >> 24) & 1)
        scanCode |= 0x100;

    wchar_t wKeyName[50];
    char cKeyName[50];
    memset(wKeyName, 0, 100);

    int length = GetKeyNameTextW(scanCode << 16, (LPWSTR)&wKeyName, 50);
    wKeyName[length] = L'\0';

    std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8_conv;
    return utf8_conv.to_bytes(wKeyName);
}

bool utils::string::is_numeric(const std::string& str)
{
    bool numeric = false;
    for (int i = NULL; i < str.size() - 1; i++)
        numeric = is_numeric(str, i);
    return numeric;
}

bool utils::string::is_numeric(const std::string& str, int location)
{
    if (location >= str.size()) return false;
    return std::isdigit(str[location]);
}

bool utils::string::is_numeric(const std::string& str, int location, int count)
{
    if (location >= str.size() || location + count >= str.size()) return false;
    for (int c = 0; c < count - 1; c++) {
        if (!is_numeric(str, location + c))
            return false;
    }
    return true;
}

std::string utils::string::hex_to_string(int _val)
{
    std::stringstream ss;
    ss << _val;
    return ss.str();
}

std::wstring utils::string::to_wstring(const std::string& _str)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> _conv;
    return _conv.from_bytes(_str);
}

std::string utils::string::to_string(const std::wstring& _wstr)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> _conv;
    return _conv.to_bytes(_wstr);
}

bool utils::string::clipboard::copy(const std::string& _str)
{
    if (_str.empty()) return false;
    HGLOBAL _hMem = GlobalAlloc(GMEM_MOVEABLE, _str.length() + 1);
    memcpy(GlobalLock(_hMem), _str.c_str(), _str.length() + 1);
    GlobalUnlock(_hMem);
    if (OpenClipboard(NULL)) {
        EmptyClipboard();
        SetClipboardData(CF_TEXT, _hMem);
        CloseClipboard();
        return true;
    }
    return false;
}

std::string utils::string::clipboard::paste()
{
    std::string _cb;
    if (OpenClipboard(nullptr)) {
        HANDLE _handle = GetClipboardData(CF_TEXT);
        char* _buffer = static_cast<char*>(GlobalLock(_handle));
        _cb = _buffer;
        GlobalUnlock(_handle);
        CloseClipboard();
    }
    return _cb;
}

#include <time.h>
#include <TlHelp32.h>

std::string utils::date()
{
    time_t raw_time;
    struct tm* time_info;
    char buffer[80];

    time(&raw_time);
    time_info = localtime(&raw_time);

    strftime(buffer, sizeof(buffer), "%d.%m.%Y %H:%M:%S", time_info);
    return buffer;
}

void utils::center_mouse()
{
    RECT _rect;
    if (GetClientRect(GetForegroundWindow(), &_rect))
        SetCursorPos((_rect.right - _rect.left) / 2, (_rect.bottom - _rect.top) / 2);
}

bool utils::resource(int _id, void* _data, size_t& _sizeOf)
{
    HRSRC _rc = FindResource(g_moduleHandle, MAKEINTRESOURCE(_id), RT_RCDATA);
    if (!_rc)
        return false;

    HGLOBAL _hGlobal = LoadResource(g_moduleHandle, _rc);
    if (!_hGlobal)
        return false;

    _data = LockResource(_hGlobal);
    _sizeOf = SizeofResource(g_moduleHandle, _rc);

    if (_data == nullptr || _sizeOf <= 0)
        return false;
    return true;
}
