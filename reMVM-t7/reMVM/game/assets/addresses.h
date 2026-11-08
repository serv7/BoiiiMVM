#pragma once
#include <assets/utils.h>

#include "structs.h"

namespace gc
{
	namespace game
	{
		inline uintptr_t _base = NULL;

		inline bool* _blockInput = NULL;
		inline bool* _hideHud = NULL;

		inline matrix* _viewMatrix = NULL;
		inline camera* _camera = NULL;
		inline double* _fov = NULL;

		inline uintptr_t _theater = NULL;
		inline int* _tick = NULL;

		inline theater_camera* _theaterCamera = NULL;
		inline int* _theaterMode = NULL;
		inline int* _dollyMarkersCount = NULL;
		inline dolly_marker* _dollyMarkers = NULL;

		inline int* _lightsCount = NULL;
		inline int* _currLight = NULL;
		inline light* _lights = NULL;

		inline std::vector<const char*> _playerNames;
		inline uintptr_t _playerPosList;

	}
}