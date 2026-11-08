#include "Camera.h"

#include <Assets/Utils.h>

using namespace mvm;

camera_manager::camera_manager() : m_camera(std::make_shared<camera>())
{
	m_enabled = m_loaded = false;
	m_frameTime = 1.0f;

	m_attached = false;
	m_parent.identify();
	m_relativeTransform.identify();

	m_input = true;
	m_movementInput = m_rotationInput = m_fovInput = 0.0f;
	m_inputScale = 1.0f;

	m_trackPlayer = std::make_shared<mvm::track_player>(camera::node::size());
}

camera_manager::~camera_manager()
{

}

void camera_manager::update(camera* _camera, const matrix44* _parent, bool _relative, float _frameTime)
{
	if (!m_loaded)
		*m_camera = *_camera, m_loaded = true;

	if (_parent)
		m_parent = *_parent;

	m_frameTime = _frameTime;

	if (!m_trackPlayer->playing()) {
		if (_relative) {
			if (!m_attached) {
				matrix44 _inverted;
				_inverted.invert(m_parent);
				m_relativeTransform = m_camera->m_transform * _inverted;
				m_attached = true;
			}
			m_camera->m_transform = m_relativeTransform * m_parent;
		}

		if (m_input && !_relative) {
			auto& _transform = m_camera->m_transform;

			vector3 _lr = m_camera->m_transform.row(0) * m_movementInput.x;
			vector3 _ud = m_camera->m_transform.row(1) * m_movementInput.y;
			vector3 _fb = m_camera->m_transform.row(2) * m_movementInput.z;
			_transform.translate(_transform.translation() + (_lr + _ud + _fb));

			matrix44 _orientation;
			_orientation.identify();
			_orientation.rotation_yaw_pitch_roll(m_rotationInput.x, m_rotationInput.y, m_rotationInput.z);
			_transform = _orientation * _transform;

			m_camera->m_fov = clamp(m_camera->m_fov + deg(m_fovInput), 0.1f, 120.f);

			clear_input();
		}
	}

	*_camera = *m_camera;
}

void camera_manager::update_input(const vector3& movement, const vector3& rotation, const float fov)
{
	m_movementInput = movement * m_inputScale * m_frameTime;
	m_rotationInput = rotation;
	m_rotationInput.z *= m_inputScale * m_frameTime;
	m_fovInput = -fov * m_inputScale * m_frameTime;

	rescale_input();
}

void camera_manager::reset()
{
	m_enabled = false;
	m_loaded = false;

	m_frameTime = 1.0f;

	m_camera->m_fov = 60.0f;
	m_camera->m_near = 0.15f;
	m_camera->m_far = 1000.0f;

	detach();

	clear_input();
}

void mvm::camera_manager::assign_state(bool _state)
{
	m_enabled = _state;
	utils::center_mouse();
}

void mvm::camera_manager::detach()
{
	m_attached = false;
}

void mvm::camera_manager::assign_input(bool _state)
{
	m_input = _state;
}

void mvm::camera_manager::update_track_buffer()
{
	m_trackBuffer.clear();
	if (m_trackPlayer->size() < 2)	
		return;

	float _beginTime = m_trackPlayer->time_of(0);
	float _endTime = m_trackPlayer->time_of(m_trackPlayer->size() - 1);

	for (float _currTime = _beginTime; _currTime < _endTime; _currTime += m_frameTime * 10.0f)
		m_trackBuffer.push_back(*(vector3*)(m_trackPlayer->value(_currTime).data()));
}

void mvm::camera_manager::rescale_input()
{
	m_movementInput *= m_inputScale;
	m_fovInput *= m_inputScale;
}