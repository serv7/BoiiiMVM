#ifndef _H__GAME_CONTEXT_H
#define _H__GAME_CONTEXT_H
#include <memory>
#include <unordered_map>

#include <assets/function.h>
#include <assets/utils.h>

#include "features/lightman.h"
#include "features/director_camera.h"

namespace gc
{
	namespace game
	{
		struct joint
		{
			std::string name;
			int tag;
		};
		inline std::vector<joint> _joints;
	}

	class game_context
	{
	public:
		bool initialize();
		bool shutdown();

		void initialize_features();
		void update();

		bool initialized() { return m_initialized; }

		bool features_state() { return m_featuresState; }
		bool in_theater() { return m_inTheater; }

		gc::director_camera* director_camera() { return m_directorCamera.get(); }
		gc::lightman* lightman() { return m_lightman.get(); }

	private:
		bool m_initialized;

		bool m_featuresState, m_inTheater;

		std::shared_ptr<gc::director_camera> m_directorCamera;
		std::shared_ptr<gc::lightman> m_lightman;

		static inline gc::game_context* m_gameContextInstance;
		static inline utils::function m_switchPlayer, m_freeCameraMove;
		static __int64 __stdcall switch_player();
		static __int64 __stdcall free_camera_move();
	};
}
#endif