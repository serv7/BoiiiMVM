#pragma once

#include <memory>
#include <string>
#include <vector>

namespace utils
{
    class function
    {
    public:
        function() { m_pointer = NULL; }
        function(unsigned long long _ptr) { m_pointer = _ptr; }

        template <typename _ty = void, typename... _args>
        _ty invoke(_args... _a) { return static_cast<_ty(*)(_args ...)>((void*)m_pointer)(_a...); }
        void** address_of() { return (void**)&m_pointer; }

        function& operator=(unsigned long long _ptr) { m_pointer = _ptr; return *this; }

        operator unsigned long long() const { return m_pointer; }
        operator void*() const { return (void*)m_pointer; }

    private:
        unsigned long long m_pointer;
    };

    template <typename _ty>
    class function_t
    {
    public:
        function_t() { m_pointer = NULL; }
        function_t(void* _ptr) { m_pointer = (unsigned long long)_ptr; }
        function_t(unsigned long long _ptr) { m_pointer = _ptr; }

        void** address_of() { return (void**)&m_pointer; }

        function_t<_ty>& operator=(void* _ptr) { m_pointer = (unsigned long long)_ptr; return *this; }
        function_t<_ty>& operator=(unsigned long long _ptr) { m_pointer = _ptr; return *this; }

        operator void* () const { return (void*)m_pointer; }
        operator unsigned long long() const { return m_pointer; }

        template <class... _args>
        _ty operator()(_args... _a) { return static_cast<_ty(*)(_args ...)>((void*)m_pointer)(_a...); }

    private:
        unsigned long long m_pointer;
    };
}