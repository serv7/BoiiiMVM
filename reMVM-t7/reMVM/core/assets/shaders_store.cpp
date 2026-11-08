#include "shaders_store.h"

#include <assets/recources.h>
#include <assets/utils.h>

#include "../../mvm.h"
#include "../render.h"

#include <fstream>
#include <filesystem>
#include <d3dcompiler.h>

static void print_error_blob(ID3D10Blob* _err)
{
	if (!_err)
		return;

	char* _errStr = static_cast<char*>(_err->GetBufferPointer());
	mvm::_core->logs()->print(mvm::err, "%s", _errStr);
}

static void dump_shader_code(const std::string& _name, ID3D10Blob* _code)
{
	std::ofstream _f;
	_f.open(_name.c_str(), std::ios::binary | std::ios::out);
	_f.write((char*)_code->GetBufferPointer(), _code->GetBufferSize());
	_f.close();
}

mvm::shaders_store::shaders_store(mvm::render* _render) : m_render(_render)
{

}

mvm::shaders_store::~shaders_store()
{

}

void mvm::shaders_store::create_shader(const std::string& _name, const std::wstring& _path, int _types)
{
	if (!m_render || !m_render->device())
		return;

	HRESULT _hr;
	ID3D10Blob* _code, *_error;
	shader_instance _instance = { _path, _types};

	if (_types & shader_type::vertex) {
		_hr = D3DCompileFromFile(_instance.m_path.c_str(), NULL, NULL, "mainVS", "vs_5_0", NULL, NULL, &_code, &_error);
		if (SUCCEEDED(_hr)) {
			dump_shader_code(_name, _code);
			_hr = m_render->device()->CreateVertexShader(_code->GetBufferPointer(), _code->GetBufferSize(), NULL, _instance.m_vertex.GetAddressOf());
			if (SUCCEEDED(_hr)) {
				D3D11_INPUT_ELEMENT_DESC _inputDesc[2];
				_inputDesc[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
				_inputDesc[1] = { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0xC, D3D11_INPUT_PER_VERTEX_DATA, 0 };

				m_render->device()->CreateInputLayout(_inputDesc, 2, _code->GetBufferPointer(), _code->GetBufferSize(), _instance.m_inputLayout.GetAddressOf());
			}
		}
	}

	if (_types & shader_type::geometry) {
		_hr = D3DCompileFromFile(_instance.m_path.c_str(), NULL, NULL, "mainGS", "gs_5_0", NULL, NULL, &_code, &_error);
		if (SUCCEEDED(_hr)) {
			dump_shader_code(_name, _code);
			_hr = m_render->device()->CreateGeometryShader(_code->GetBufferPointer(), _code->GetBufferSize(), NULL, _instance.m_geometry.GetAddressOf());
		}
	}

	if (_types & shader_type::pixel) {
		_hr = D3DCompileFromFile(_instance.m_path.c_str(), NULL, NULL, "mainPS", "ps_5_0", NULL, NULL, &_code, &_error);
		if (SUCCEEDED(_hr)) {
			dump_shader_code(_name, _code);
			_hr = m_render->device()->CreatePixelShader(_code->GetBufferPointer(), _code->GetBufferSize(), NULL, _instance.m_pixel.GetAddressOf());
		}
	}

	m_shaders.emplace(_name, _instance);
}

void mvm::shaders_store::create_shader(const std::string& _name, int _vs, int _gs, int _ps, int _types)
{
	if (!m_render || !m_render->device())
		return;

	HRESULT _hr;
	shader_instance _instance = { L"", _types };

	if ((_types & shader_type::vertex)) {
		mvm::load_resource(_vs, std::format("{}_{}", "vs_", _name), mvm::_core->logs());
		auto _shaderRes = mvm::find_resource(_vs);
		if (_shaderRes) {
			_hr = m_render->device()->CreateVertexShader(_shaderRes->data, _shaderRes->size, NULL, _instance.m_vertex.GetAddressOf());
			if (FAILED(_hr))
				_core->logs()->print(mvm::wrn, "Failed to create vertex shader. HRESULT 0x%X", _hr);
		}
		else _core->logs()->print(mvm::err, "vertex shader resource data is empty or resource not found. %d, %s", _vs, _name.c_str());
	}

	if ((_types & shader_type::geometry)) {
		mvm::load_resource(_gs, std::format("{}_{}", "gs_", _name), mvm::_core->logs());
		auto _shaderRes = mvm::find_resource(_gs);
		if (_shaderRes) {
			_hr = m_render->device()->CreateGeometryShader(_shaderRes->data, _shaderRes->size, NULL, _instance.m_geometry.GetAddressOf());
			if (FAILED(_hr))
				_core->logs()->print(mvm::wrn, "Failed to create geometry shader. HRESULT 0x%X", _hr);
		}
		else _core->logs()->print(mvm::err, "geometry shader resource data is empty or resource not found. %d, %s", _gs, _name.c_str());
	}

	if ((_types & shader_type::pixel)) {
		mvm::load_resource(_ps, std::format("{}_{}", "ps_", _name), mvm::_core->logs());
		auto _shaderRes = mvm::find_resource(_ps);
		if (_shaderRes) {
			_hr = m_render->device()->CreatePixelShader(_shaderRes->data, _shaderRes->size, NULL, _instance.m_pixel.GetAddressOf());
			if (FAILED(_hr))
				_core->logs()->print(mvm::wrn, "Failed to create pixel shader. HRESULT 0x%X", _hr);
		}
		else _core->logs()->print(mvm::err, "pixel shader resource data is empty or resource not found. %d, %s", _ps, _name.c_str());
	}

	m_shaders.emplace(_name, _instance);
}

void mvm::shaders_store::use_shader(const std::string& _name)
{
	if (!m_render || !m_render->device_context())
		return;

	auto _shader = m_shaders.find(_name);
	if (_shader != m_shaders.end()) {
		shader_instance& _instance = _shader->second;
		if (_instance.m_types & shader_type::vertex)
			m_render->device_context()->VSSetShader(_instance.m_vertex.Get(), NULL, NULL), m_render->device_context()->IASetInputLayout(_instance.m_inputLayout.Get());
		if (_instance.m_types & shader_type::geometry)
			m_render->device_context()->GSSetShader(_instance.m_geometry.Get(), NULL, NULL);
		if (_instance.m_types & shader_type::pixel)
			m_render->device_context()->PSSetShader(_instance.m_pixel.Get(), NULL, NULL);
	}
}

void mvm::shaders_store::recompile()
{
	std::vector<std::pair<std::string, shader_instance>> _instances;
	for (auto& _instance : m_shaders)
		_instances.push_back({ _instance.first, _instance.second });

	m_shaders.clear();
	for (auto& _instance : _instances) {
		if (!_instance.second.m_path.empty())
			create_shader(_instance.first, _instance.second.m_path, _instance.second.m_types);
		else m_shaders.emplace(_instance.first, _instance.second);
	}
}

void mvm::shaders_store::recompile(const std::string& _name)
{
	if (m_shaders.find(_name) == m_shaders.end())
		return;

	const auto& _instance = m_shaders.find(_name)->second;
	if (!_instance.m_path.empty()) {
		std::wstring _path = _instance.m_path;
		int _types = _instance.m_types;
		m_shaders.erase(_name);

		create_shader(_name, _path, _types);
	}
}