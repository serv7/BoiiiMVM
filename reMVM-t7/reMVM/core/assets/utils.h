#pragma once
#include <windows.h>

#include <string>
#include <vector>

#define MODULE_HANDLE GetModuleHandle(NULL)
inline HMODULE g_moduleHandle;

#ifdef _WIN64
template <typename _ty, typename ...args>
inline auto virtual_call(void* __ptr64 _ptr, uintptr_t _offset, args... _args) { return ((*reinterpret_cast<_ty(__thiscall***)(void*, args...)>(_ptr))[_offset / 8])(_ptr, _args...); }
#else
template <typename _ty, typename ...args>
inline auto virtual_call(void* __ptr64 _ptr, uintptr_t _offset, args... _args) { return ((*reinterpret_cast<_ty(__thiscall***)(void*, args...)>(_ptr))[_offset / 4])(_ptr, _args...); }
#endif
template <typename _ty> inline _ty v_table(_ty _object, int _index) { return (_ty)((_ty*)*(_ty*)_object)[_index]; }

template <typename _ty = void, typename... args>
inline _ty direct_invoke(unsigned long long _ptr, args ... _args) { return static_cast<_ty(*)(args ...)>((void*)_ptr)(_args...); }

#define ADD_FLAG(v, f) (v |= f)
#define DEL_FLAG(v, f) (v &= ~f)
#define HAS_FLAG(v, f) (v & f)

#ifdef _UNICODE
# define STR(str) L##str
# define TO_STR(e) L###e
#else
# define STR(str) str
# define TO_STR(e) #x
#endif

namespace utils
{
    namespace process
    {
        unsigned long process(const char*);
        std::string process_name(unsigned long);

        bool inject(unsigned long, const char*);
    }

    namespace signature
    {
        namespace _local
        {
            uintptr_t find(HMODULE, uintptr_t, const char*);
            uintptr_t address_of(uintptr_t, uintptr_t, long);
        };

        uintptr_t find(const char*);
        uintptr_t find(const char*, int);
        uintptr_t find(HMODULE, const char*);
        uintptr_t find(HMODULE, const char*, int);

        uintptr_t address_of(uintptr_t);
    }

    namespace string
    {
        void split(std::vector<std::string>*, const std::string&, char _delim = ' ');
        std::string vk_to_string(DWORD);
        std::string lparam_to_string(LPARAM);

        bool is_numeric(const std::string&);
        bool is_numeric(const std::string&, int);
        bool is_numeric(const std::string&, int, int);

        std::string hex_to_string(int);

        std::wstring to_wstring(const std::string&);
        std::string to_string(const std::wstring&);

        namespace clipboard
        {
            bool copy(const std::string&);
            std::string paste();
        }
    }

    namespace flags
    {
        inline bool exists(unsigned int _val, unsigned __int8 _flags) { return _val & _flags; }

        inline void set(unsigned int& _val, unsigned __int8 _flags) { _val = _flags; }
        inline void assign(unsigned int& _val, unsigned __int8 _flags) { if (!(_val & _flags)) _val |= _flags; }
        inline void remove(unsigned int& _val, unsigned __int8 _flags) { if (_val & _flags) _val &= ~_flags; }
        inline void clear(unsigned int& _val) { _val = NULL; }
    }

    std::string date();

    void center_mouse();

    constexpr unsigned __int8 bit(unsigned __int8 _n) { return 1 << _n; }

    bool resource(int, void*, size_t&);
}