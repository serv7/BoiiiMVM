#pragma once
#include <assets/function.h>
#include <assets/math.h>

#include "../assets/addresses.h"
#include "../assets/structs.h"

#include <camera.h>
#include <track_player.h>
#include "custom_dolly_marker.h"

namespace gc
{
	class director_camera
	{
	public:
		director_camera();
		~director_camera();

		void initialize();
		void update(float, float);

		mvm::camera_manager* camera_manager() { return m_cameraManager.get(); }
		mvm::track_player* custom_dolly() { return m_customDolly.get(); }

		bool* auto_focus() { return &m_autoFocus; }
		vector3* focus_offset() { return &m_focusOffset; }
		int* focus_target() { return &m_focusTarget; }
		int* focus_joint() { return &m_focusJoint; }

	private:
		gc::custom_dolly_marker create_custom_marker();

		bool m_initialized;

		std::shared_ptr<mvm::camera_manager> m_cameraManager;
		std::shared_ptr<mvm::track_player> m_customDolly;

		bool m_autoFocus;
		vector3 m_focusOffset;
		int m_focusTarget, m_focusJoint;

		static inline gc::director_camera* m_directorCameraInstance;
		static inline utils::function m_addDollyCamMarker, m_removeDollyCamMarker, m_applyNewDollyCamMarkerPosition;
		static __int64 __stdcall add_dolly_cam_marker();
		static __int64 __stdcall remove_dolly_cam_marker();
		static __int64 __stdcall apply_new_dolly_cam_marker_position();

	};
}