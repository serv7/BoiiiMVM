#pragma once
#include <assets/memory.h>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>
#include <XorStr.h>

namespace utils
{
	struct patch_base
	{
		std::string name;
		uintptr_t address;
		size_t size;

		unsigned char* data;
	};

	class patch
	{
	public:
		patch(const std::string& _name, uintptr_t _address, size_t _size, unsigned char* _bytes) : m_name(_name), m_address(_address), m_size(_size), m_state(false)
		{
			m_original = new unsigned char[_size];
			memcpy((void*)m_original, (void*)m_address, _size);

			m_patched = new unsigned char[_size];
			if (_bytes) memcpy((void*)m_patched, (void*)_bytes, _size);
			else memset((void*)m_patched, 0x90, _size);
		}

		const std::string& name() const { return m_name; }
		const size_t size() const { return m_size; }

		bool state() const { return m_state; }

		void apply()  {  if (m_state) return; write_memory(m_patched); m_state = true; }
		void restore() {  if (!m_state) return;  write_memory(m_original); m_state = false; }

	private:
		void write_memory(unsigned char* _memory)
		{
			unsigned long _protect;
			VirtualProtect((void*)m_address, m_size, PAGE_EXECUTE_READWRITE, &_protect);
			memcpy((void*)m_address, _memory, m_size);
			VirtualProtect((void*)m_address, m_size, _protect, NULL);
		}

		std::string m_name;

		uintptr_t m_address;
		size_t m_size;

		unsigned char* m_original;
		unsigned char* m_patched;

		bool m_state;
	};

	inline std::vector<utils::patch> g_patchesStore;

	inline void register_patch(const std::string& _name, uintptr_t _address, size_t _size, unsigned char* _bytes = nullptr)
	{
		if (_name.empty() || !_address)
			return;

		g_patchesStore.push_back(utils::patch(_name, _address, _size, _bytes));
	}

	inline utils::patch* find_patch(const std::string& _name)
	{
		const auto& _it = std::find_if(g_patchesStore.begin(), g_patchesStore.end(), [&](const utils::patch& _patch) { return _patch.name() == _name; });
		return _it != g_patchesStore.end() ? _it._Ptr : nullptr;
	}
}