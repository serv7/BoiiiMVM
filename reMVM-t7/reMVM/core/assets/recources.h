#pragma once
#include <assets/logs.h>
#include <assets/utils.h>

#include <string>
#include <vector>

namespace mvm
{
    struct resource
    {
        int id;
        std::string name;
        
        void* data;
        size_t size;

        bool empty() { return data == nullptr || size == NULL; }
    };

    inline std::vector<mvm::resource> g_resourcesStore;

    inline bool load_resource(int _id, const std::string& _name, mvm::logs* _logsHandler = nullptr)
    {
        HRSRC _rc = FindResource(g_moduleHandle, MAKEINTRESOURCE(_id), RT_RCDATA);
        if (!_rc) {
            if (_logsHandler)
                _logsHandler->print(mvm::err, "Failed to find resource '%s': %d, 0x%X", _name.c_str(), _id, GetLastError());
            return false;
        }

        HGLOBAL _hGlobal = LoadResource(g_moduleHandle, _rc);
        if (!_hGlobal) {
            if (_logsHandler)
                _logsHandler->print(mvm::err, "LoadResource '%s' failed: %d, 0x%X", _name.c_str(), _id, GetLastError());
            return false;
        }

        mvm::resource _rsrc;
        _rsrc.id = _id;
        _rsrc.name = _name;
        _rsrc.data = LockResource(_hGlobal);
        _rsrc.size = SizeofResource(g_moduleHandle, _rc);
        if (_rsrc.empty()) {
            if (_logsHandler)
                _logsHandler->print(mvm::err, "Resource '%s' data is empty: %d, 0x%X", _name.c_str(), _id, GetLastError());
            return false;
        }

        g_resourcesStore.push_back(_rsrc);
        return true;
    }

    inline mvm::resource* find_resource(int _id)
    {
        const auto& _it = std::find_if(g_resourcesStore.begin(), g_resourcesStore.end(), [&](const mvm::resource& _rsrc) { return _rsrc.id == _id; });
        return _it != g_resourcesStore.end() ? _it._Ptr : nullptr;
    }

    inline mvm::resource* find_resource(const std::string& _name)
    {
        const auto& _it = std::find_if(g_resourcesStore.begin(), g_resourcesStore.end(), [&](const mvm::resource& _rsrc) { return _rsrc.name == _name; });
        return _it != g_resourcesStore.end() ? _it._Ptr : nullptr;
    }
}