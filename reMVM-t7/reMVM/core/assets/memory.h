#pragma once
#include <Windows.h>
#include <Psapi.h>
#include <vector>

namespace utils
{
	namespace memory
	{
		inline size_t module_size(HMODULE _hModule)
		{
			MODULEINFO _info = MODULEINFO();
			K32GetModuleInformation(GetCurrentProcess(), _hModule, &_info, sizeof(MODULEINFO));
			return _info.SizeOfImage;
		}

		inline uintptr_t base_address(HMODULE _hModule)
		{
			MODULEINFO _info = MODULEINFO();
			K32GetModuleInformation(GetCurrentProcess(), _hModule, &_info, sizeof(MODULEINFO));
			return (uintptr_t)_info.lpBaseOfDll;
		}

		inline uintptr_t pointer(uintptr_t _base, uintptr_t _offset) { return *(uintptr_t*)_base + _offset; }
		inline uintptr_t pointer(uintptr_t _base, const std::vector<uintptr_t>& _offsets)
		{
			uintptr_t _address = _base;
			for (auto _offset : _offsets)
				_address = *(uintptr_t*)_address, _address += _offset;
			return _address;
		}

		inline void read(uintptr_t _address, void* _buffer, size_t _size = 1)
		{
			DWORD _protect;
			VirtualProtect((LPVOID)_address, _size, PAGE_EXECUTE_READWRITE, &_protect);
			ReadProcessMemory(GetCurrentProcess(), (LPVOID)_address, _buffer, _size, NULL);
			VirtualProtect((LPVOID)_address, _size, _protect, NULL);
		}

		inline void write(uintptr_t _address, void* _val, size_t _size = 1)
		{
			DWORD _protect;
			VirtualProtect((LPVOID)_address, _size, PAGE_EXECUTE_READWRITE, &_protect);
			WriteProcessMemory(GetCurrentProcess(), (LPVOID)_address, _val, _size, NULL);
			VirtualProtect((LPVOID)_address, _size, _protect, NULL);
		}

		inline void nop(uintptr_t _address, size_t _size = 1) 
		{
			unsigned char* _bytes = new unsigned char[_size];
			memset(_bytes, 0x90, _size);

			unsigned long _protect;
			VirtualProtect((void*)_address, _size, PAGE_EXECUTE_READWRITE, &_protect);
			WriteProcessMemory(GetCurrentProcess(), (void*)_address, _bytes, _size, NULL);
			VirtualProtect((LPVOID)_address, _size, _protect, NULL);
		}
	}
}