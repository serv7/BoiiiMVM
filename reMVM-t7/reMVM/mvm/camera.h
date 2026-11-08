#ifndef _H__CAMERA_H
#define _H__CAMERA_H
#include <assets/math.h>

#include <track_player.h>

namespace mvm
{
	struct camera
	{
		struct node
		{
			node(const float* _data)														{ *this = _data; }
			node(const matrix44& _transform, const float _fov)		{ pos = _transform; rot = _transform; fov = _fov;}

			vector3 pos;
			quaternion rot;
			float fov;

			float* data()											{ return &pos.x; }
			static size_t size()									{ return sizeof(camera::node) / sizeof(float); }

			float operator[](int _index) const						{ const float* _data = &pos.x; return _data[_index]; }

			void operator=(const float* _val)						{ float* _data = &pos.x; for (int i = 0; i < this->size(); i++) _data[i] = _val[i]; }
			void operator=(const node& _val)						{ float* _data = &pos.x; for (int i = 0; i < this->size(); i++) _data[i] = _val[i]; }
		};

		camera() : m_fov(60.f), m_near(0.15f), m_far(1000.f) { m_transform.identificated(); }
		camera(const matrix44& m, float f, float np = 0.15f, float fp = 1000.f) : m_transform(m), m_fov(f), m_near(np), m_far(fp) {}

		vector3 position() const { return m_transform.translation(); }
		quaternion rotation() const { quaternion q = m_transform; return q; }

		node extract_node() { return node(m_transform, m_fov); }

		void operator=(const node& _val)  { m_transform.translate(_val.pos); m_transform.rotation_quaternion(_val.rot.normalized()); m_fov = _val.fov; }

		matrix44 m_transform;
		float m_fov, m_near, m_far;
	};

	extern class render;
	class camera_manager
	{
	public:
		camera_manager();
		~camera_manager();

		void update(mvm::camera*, const matrix44*, bool, float);
		void update_input(const vector3&, const vector3&, const float);

		void reset();

		mvm::camera* camera_source() { return m_camera.get(); }

		bool enabled() { return m_enabled; }
		void assign_state(bool);

		void detach();

		void assign_input(bool);
		bool has_input() { return m_input; }
		void toggle_input() { m_input = !m_input; }
		void clear_input() { m_movementInput = m_rotationInput = m_fovInput = 0.0f; }

		float* input_scale() { return &m_inputScale; }

		mvm::track_player* track_player() { return m_trackPlayer.get(); }
		void update_track_buffer();
		const std::vector<vector3>& track_buffer() { return m_trackBuffer; }

	private:
		void rescale_input();

		std::shared_ptr<mvm::camera> m_camera;

		bool m_enabled, m_loaded;
		float m_frameTime;

		bool m_attached;
		matrix44 m_parent, m_relativeTransform;

		bool m_input;
		vector3 m_movementInput;
		vector3 m_rotationInput;
		float m_fovInput;
		float m_inputScale;

		std::shared_ptr<mvm::track_player> m_trackPlayer;
		std::vector<vector3> m_trackBuffer;
	};
};
#endif