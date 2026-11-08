#include "key_binding_store.h"

#include <algorithm>
#include <filesystem>
#include <fstream>

mvm::key_binding_store::key_binding_store(const std::string& _path) : m_capturing(false), m_capturedKey(NULL)
{
	m_file = _path;
	if (std::filesystem::exists(m_file))
		load();
}

mvm::key_binding_store::~key_binding_store()
{

}

void mvm::key_binding_store::bind(const std::string& _name, int _key, bool _override)
{
	if (find(_name) && _override)
		override_key(_name, _key);

	m_instances.push_back({ _name, _key });
}

void mvm::key_binding_store::override_key(const std::string& _name, int _key)
{
	auto _target = find(_name);
	if (!_target)
		return;

	if (auto _existing = find(_key))
		_existing->key = _target->key;
	_target->key = _key;
}

mvm::bind_instance* mvm::key_binding_store::find(const int _key)
{
	const auto& _it = std::find_if(m_instances.begin(), m_instances.end(), [&](const mvm::bind_instance& _instance) { return _instance.key == _key; });
	if (_it != m_instances.end() && _it->valid())
		return _it._Ptr;
	return nullptr;
}

mvm::bind_instance* mvm::key_binding_store::find(const std::string& _name)
{
	const auto& _it = std::find_if(m_instances.begin(), m_instances.end(), [&](const mvm::bind_instance& _instance) { return _instance.name == _name; });
	if (_it != m_instances.end() && _it->valid())
		return _it._Ptr;
	return nullptr;
}

void mvm::key_binding_store::save()
{
	int size = m_instances.size();
	std::ofstream f(m_file, std::ios::binary | std::ios::out);
	f.write((char*)&size, sizeof(int));

	for (int i = 0; i < size; i++) {
		size_t length = m_instances[i].name.length() + 1;
		f.write((char*)&length, sizeof(size_t));
		f.write((char*)m_instances[i].name.c_str(), length);
		f.write((char*)&m_instances[i].key, sizeof(int));
	}
	f.close();
}

void mvm::key_binding_store::load()
{
	int size = NULL;
	std::ifstream f(m_file, std::ios::binary | std::ios::in);
	f.read((char*)&size, sizeof(int));

	m_instances.clear();
	bind_instance _instance = bind_instance();
	for (int i = NULL; i < size; i++) {
		size_t length = NULL;
		f.read((char*)&length, sizeof(size_t));
		char* buffer = new char[length];
		f.read((char*)buffer, length);
		_instance.name = buffer;
		f.read((char*)&_instance.key, sizeof(int));
		m_instances.push_back(_instance);
	}
	f.close();
}

void mvm::key_binding_store::begin_capturing(const std::string& _capturingTarget)
{
	m_capturing = true;
	m_capturingTarget = _capturingTarget;
	m_capturedKey = NULL;
}

void mvm::key_binding_store::complete_capturing()
{
	m_capturing = false;
	m_capturingTarget.clear();
	m_capturedKey = NULL;
}

void mvm::key_binding_store::capture_key(int _key)
{
	m_capturedKey = _key;
}