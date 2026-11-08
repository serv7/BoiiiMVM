#pragma once
#include <assets/function.h>
#include <assets/math.h>
#include <assets/utils.h>

#include "addresses.h"
#include "structs.h"

namespace gc
{
	inline __int64 register_tag(const char* _tagName)
	{
		static utils::function _function = (uintptr_t)(game::_base + 0x1A83520);
		return _function.invoke<__int64>(_tagName);
	}

	inline __int64 get_tag_pos(entity* _entity, int _tag, vector3* _pos)
	{
		static utils::function _function = (uintptr_t)(game::_base + 0x7C480);
		return _function.invoke<__int64>(_entity, _tag, _pos);
	}

	inline client_info* get_client_info(int localClientNum, int index)
	{
		static utils::function _function = (uintptr_t)(game::_base + 0x9541E0);
		return _function.invoke<client_info*>(localClientNum, index);
	}

	inline entity* get_entity(int _localClientNum, int _index)
	{
		static utils::function _function = (uintptr_t)(game::_base + 0x1C4D70);
		return _function.invoke<entity*>(_localClientNum, _index);
	}

	inline char* get_client_d_obj(int _handle, int _localClientNum)
	{
		static utils::function _function = (uintptr_t)(game::_base + 0x214E140);
		return _function.invoke<char*>(_handle, _localClientNum);
	}

	inline __int64 get_world_tag_matrix(entity* _entity, __int64 _dObj, unsigned int _tag, float* _mat, vector3* _pos)
	{
		static utils::function _function = (uintptr_t)(game::_base + 0x1ABCF0);
		return _function.invoke<__int64>(_entity, _dObj, _tag, _mat, _pos);
	}

	inline __int64 world_to_screen(int _localClientNum, float* _worldPos, float* _screenPos)
	{
		static utils::function _function = (uintptr_t)(game::_base + 0x574140);
		return _function.invoke<__int64>(_localClientNum, _worldPos, _screenPos);
	}

	inline player_state* get_player_state(int _index)
	{
		static utils::function _function = (uintptr_t)(game::_base + 0x8DF6A0);
		return _function.invoke<player_state*>(_index);
	}

	inline client_globals* GetLocalClientGlobals()
	{
		auto _playerState = (uintptr_t)get_player_state(NULL);
		if (!_playerState)
			return nullptr;
		return (client_globals*)(_playerState - 0x11A8B0);
	}
}