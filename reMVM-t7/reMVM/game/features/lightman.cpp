#include "lightman.h"

#include <assets/memory.h>
#include <assets/utils.h>

#include <MH/MinHook.h>

#include "../../mvm.h"

gc::lightman::lightman() : m_initialized(false), m_lightCount(NULL), m_currLight(-1), m_dollyMode(false), m_removeMethod(1)
{
	lightman::m_lightmanInstance = this;
}

gc::lightman::~lightman()
{

}

void gc::lightman::initialize()
{
	HMODULE _hModule = GetModuleHandle(L"BlackOps3.exe");

	game::_lightsCount = (int*)utils::memory::pointer(game::_theater, 0x984C4C);
	game::_currLight = (int*)utils::memory::pointer(game::_theater, 0x984C50);
	game::_lights = (gc::light*)utils::memory::pointer(game::_theater, 0x98460C);

	m_addLightmanMarker = game::_base + 0x25FD800;
	m_removeLightmanMarker = game::_base + 0x2604970;
	m_editLightmanMarker = game::_base + 0x25FF880;

	MH_Initialize();
	MH_CreateHook(m_addLightmanMarker, add_lightman_marker, m_addLightmanMarker.address_of());
	MH_CreateHook(m_removeLightmanMarker, remove_lightman_marker, m_removeLightmanMarker.address_of());
	MH_CreateHook(m_editLightmanMarker, edit_lightman_marker, m_editLightmanMarker.address_of());
	MH_EnableHook(MH_ALL_HOOKS);

	m_initialized = true;
}

void gc::lightman::update(float _time, float _frameTime)
{
	if (!m_initialized)
		return;

	if (*game::_lightsCount < 1 && m_lightCount > NULL)
		m_lights.clear();
	m_lightCount = *game::_lightsCount;

	m_frameTime = _frameTime;

	for (auto& _light : m_lights) {
		_light.dolly->playback(!m_dollyMode);
		if (_light.dolly->playing()) {
			light_node _node = _light.dolly->value(_time).data();
			light& _instance = game::_lights[_light.instance];

			_instance.position = _node.position;
			_instance.lookAt = _node.lookAt;
			_instance.color = _node.color;
			_instance.intensity = _node.intensity;
			_instance.range = _node.range;
		}
	}
}

int* gc::lightman::curr_light()
{
	return &m_currLight;
}

gc::light_dolly* gc::lightman::light_dolly(int _index)
{
	const auto& _it = std::find_if(m_lights.begin(), m_lights.end(), [&](const gc::light_dolly& _lDolly) { return _lDolly.instance == _index; });
	return _it != m_lights.end() ? _it._Ptr : nullptr;
}

const std::vector<gc::light_dolly>& gc::lightman::lights_buffer()
{
	return m_lights;
}

void gc::lightman::push_dolly_mode(bool _state)
{
	m_dollyMode = _state;
}

bool gc::lightman::dolly_mode()
{
	return m_dollyMode;
}

int* gc::lightman::remove_method()
{
	return &m_removeMethod;
}

void gc::lightman::relocate(int _index)
{
	if (_index < NULL || _index >= *gc::game::_lightsCount)
		return;

	game::_lights[_index].position = game::_theaterCamera->pos;
}

void gc::lightman::redirect(int _index)
{
	if (_index < NULL || _index >= *gc::game::_lightsCount)
		return;

	game::_lights[_index].lookAt = game::_theaterCamera->pos;
}

void gc::lightman::push_variable(gc::light& _light, light_variable _var, const std::any& _val)
{
	if (_var == gc::light_variable_type)
		_light.type = std::any_cast<int>(_val);

	if (_var == gc::light_variable_intensity)
		_light.intensity = std::any_cast<float>(_val);

	if (_var == gc::light_variable_type)
		_light.color = std::any_cast<vector3>(_val);

	if (_var == gc::light_variable_type)
		_light.range = std::any_cast<float>(_val);

	if (_var == gc::light_variable_type)
		_light.instansityScale = std::any_cast<float>(_val);

	if (_var == gc::light_variable_type)
		_light.position = std::any_cast<vector3>(_val);

	if (_var == gc::light_variable_type)
		_light.lookAt = std::any_cast<vector3>(_val);

	if (_var == gc::light_variable_type)
		_light.u = std::any_cast<vector3>(_val);
}

void gc::lightman::create()
{
	m_lightCount = *game::_lightsCount;
	m_currLight = m_lights.size();
	m_lights.push_back({ m_currLight, std::make_shared<mvm::track_player>(light_node::size_of()) });
}

void gc::lightman::remove(int _currLight)
{
	const auto& _it = std::find_if(m_lights.begin(), m_lights.end(), [&](const gc::light_dolly& _lDolly) { return _lDolly.instance == _currLight; });
	m_lights.erase(_it);
	m_lightCount = *game::_lightsCount;
	m_currLight = m_lights.size() - 1;
}

__int64 __stdcall gc::lightman::add_lightman_marker()
{
	if (m_lightmanInstance->m_dollyMode)
		return NULL;

	__int64 _result = m_addLightmanMarker.invoke<__int64>();
	m_lightmanInstance->create();
	return _result;
}

__int64 __stdcall gc::lightman::remove_lightman_marker()
{
	if (m_lightmanInstance->m_dollyMode)
		return NULL;

	m_lightmanInstance->remove(*game::_currLight);
	return m_removeLightmanMarker.invoke<__int64>();
}

__int64 __stdcall gc::lightman::edit_lightman_marker()
{
	if (!m_lightmanInstance->m_dollyMode)
		return m_editLightmanMarker.invoke<__int64>();

	m_lightmanInstance->m_currLight = *game::_currLight;
	return NULL;
}

void gc::light_dolly::update_buffer()
{
	buffer.clear();
	if (dolly->size() < 2)
		return;

	float _beginTime = dolly->time_of(0);
	float _endTime = dolly->time_of(dolly->size() - 1);

	for (float _currTime = _beginTime; _currTime < _endTime; _currTime += lightman::m_lightmanInstance->m_frameTime * 10.0f)
		buffer.push_back(gc::light_node(dolly->value(_currTime).data()));
}

void gc::light_dolly::insert_marker()
{
	float _time = (float)*game::_tick / 1000.0f;

	light_node _node = game::_lights[instance];
	dolly->insert(_time, _node.c_data());
	update_buffer();
}

void gc::light_dolly::remove_marker(int _index)
{
	dolly->remove(dolly->time_of(_index));
	update_buffer();
}

void gc::light_dolly::replace_marker(float _nodeTime)
{
	light_node _node = game::_lights[instance];
	dolly->replace(_nodeTime, mvm::node::value(_node.c_data(), light_node::size_of()));
	update_buffer();
}

#include <ranges>
int gc::light_dolly::nearest_of(const vector3& _pos)
{
	if (dolly->size() < 2)
		return NULL;

	std::vector<float> _distances;
	size_t _size = dolly->size();
	for (int i = 0; i < _size - 1; i++) {
		gc::light_node _node = dolly->node(i).val.data();
		_distances.push_back(_node.position.distance(_pos));
	}

	const auto& _it = std::ranges::min_element(_distances.begin(), _distances.end());
	return std::ranges::distance(_distances.begin(), _it);
}