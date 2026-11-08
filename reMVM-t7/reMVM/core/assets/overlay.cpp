#include "overlay.h"

namespace _private
{
	inline float clamp(float _x, float _min, float _max)
	{
		_x = (_x + _max - fabsf(_x - _max)) * 0.5f;
		_x = (_x + _min + fabsf(_x - _min)) * 0.5f;
		return _x;
	}

	inline float linear(const float& _y0, const float& _y1, float _mu) { return _y0 + (_y1 - _y0) * _mu; }
}

mvm::overlay::overlay(const std::string& _name, unsigned int _flags)
{
	m_name = _name;

	m_flags = _flags;
	m_forceUpdate = false;

	push_opacity(animated_value<float>(0.0f, 1.0f, 0.0f));
	m_animationProcessor = std::make_shared<timer_ms>();
}

mvm::overlay::~overlay()
{

}

void mvm::overlay::render(bool _forceUpdate, std::function<void(void)> _function)
{
	if (m_forceUpdate != _forceUpdate)
		m_forceUpdate = _forceUpdate, reset_animation();

	process_animation(m_forceUpdate);
	if ((m_forceUpdate || m_animationProcessor->is_running()) && _function)
		_function();
}

void mvm::overlay::process_animation(bool _forward)
{
	if (!animated()) {
		m_opacity.process(1.0f, _forward);
		return;
	}

	if (m_animationProcessor->is_running()) {
		float _time = _private::clamp(m_animationProcessor->elapsed() / 1000.0f, 0.0f, 1.0f);
		m_opacity.process(_time, _forward);

		if (_time >= 1.0f)
			m_animationProcessor->stop();
	}
}

void mvm::overlay::reset_animation()
{
	m_animationProcessor->reset();
	m_animationProcessor->start();
}