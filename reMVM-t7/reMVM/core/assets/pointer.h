#pragma once
#include <cstdint>

namespace utils
{
	class pointer
	{
	public:
		pointer() { m_pointer = nullptr; }
		pointer(std::nullptr_t) { m_pointer = nullptr; }
		pointer(void* _pointer) { m_pointer = _pointer; }
		pointer(uintptr_t _pointer) { m_pointer = (void*)_pointer; }
		~pointer() {}

		operator void* () { return m_pointer; }
		operator unsigned long long() { return (unsigned long long)m_pointer; }

		template <typename _ty>
		_ty& value() { return *static_cast<_ty*>(m_pointer); }

	private:
		void* m_pointer;

	};

	template <typename _ty>
	class pointer_t
	{
	public:
		pointer() { m_pointer = nullptr; }
		pointer(std::nullptr_t) { m_pointer = nullptr; }
		pointer(void* _pointer) { m_pointer = static_cast<_ty*>(_pointer); }
		pointer(uintptr_t _pointer) { m_pointer = static_cast<_ty*>(_pointer); }
		pointer(_ty* _pointer) { m_pointer = _pointer; }
		~pointer() {}

		operator void* () { return m_pointer; }
		operator unsigned long long() { return (unsigned long long)m_pointer; }

		operator _ty& () const { return *m_pointer; }
		_ty& operator[](int _index) { return m_pointer[_index]; }

		const size_t size() const { return sizeof(void*); }
		const size_t size_of() const { return sizeof(_ty); }

	private:
		_ty* m_pointer;

	};
}