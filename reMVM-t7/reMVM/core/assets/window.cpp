#include "window.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>

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

mvm::window::window(const std::string& _name, unsigned int _flags)
{
	m_name = _name;

	m_flags = _flags;
	m_forceUpdate = false;

	m_dpi = std::make_shared<mvm::dpi>();

	push_opacity(animated_value<float>(0.0f, 1.00f));
	m_animationProcessor = std::make_shared<timer_ms>();
}

mvm::window::~window()
{

}

void mvm::window::render(bool _forceUpdate, std::function<void(void)> _function)
{
	if (m_forceUpdate != _forceUpdate)
		m_forceUpdate = _forceUpdate, reset_animation();

	process_animation(m_forceUpdate);
	if (m_forceUpdate || m_animationProcessor->is_running()) {
		ImGui::SetNextWindowPos({ m_position.value().x * m_dpi->scale().x,  m_position.value().y * m_dpi->scale().y });
		ImGui::SetNextWindowSize({ m_size.x * m_dpi->scale().x,  m_size.y * m_dpi->scale().y });
		ImGui::SetNextWindowBgAlpha(_private::clamp(m_opacity.value(), 0.0f, 1.0f));

		ImGuiWindowFlags _flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar;
		if (utils::flags::exists(m_flags, mvm_window_flags_no_input))
			_flags |= ImGuiWindowFlags_NoInputs;

		ImGui::Begin(std::format("##mvm_window_{}", m_name.c_str()).c_str(), nullptr, _flags);

		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, _private::clamp(m_opacity.value(), 0.0f, 1.0f));
		m_id = ImGui::GetItemID();
		ImGui::BeginDisabled(disabled());
		if (_function)
			_function();
		ImGui::EndDisabled();
		ImGui::PopStyleVar();

		ImGui::End();
	}
}

void mvm::window::resize(const vector2& _size)
{
	m_size = _size;
}

void mvm::window::resize_items(const vector2& _itemSize)
{
	m_itemSize = _itemSize;
}

void mvm::window::process_animation(bool _forward)
{
	if (!animated()) {
		m_position.process(1.0f, _forward);
		m_opacity.process(1.0f, _forward);
		return;
	}

	if (m_animationProcessor->is_running()) {
		float _time = _private::clamp(m_animationProcessor->elapsed() / 1000.0f, 0.0f, 1.0f);
		m_position.process(_time, _forward);
		m_opacity.process(_time, _forward);

		if (_time >= 1.0f)
			m_animationProcessor->stop();
	}
}

void mvm::window::reset_animation()
{
	m_animationProcessor->reset();
	m_animationProcessor->start();
}