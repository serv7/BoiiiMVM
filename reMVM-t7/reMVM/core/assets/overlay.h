#pragma once
#include <assets/window.h>

namespace mvm
{
	constexpr static unsigned __int8 mvm_overlay_flags_none = utils::bit(0);
	constexpr static unsigned __int8 mvm_overlay_flags_animated = utils::bit(1);

	class overlay
	{
	public:
		overlay(const std::string&, unsigned int);
		~overlay();

		void render(bool, std::function<void(void)>);

		unsigned int id() { return m_id; }
		const std::string& name() { return m_name; }

		unsigned int flags() { return m_flags; }
		bool animated() { return utils::flags::exists(m_flags, mvm_overlay_flags_animated); }

		void update_state(bool _state) { m_forceUpdate = _state; }
		bool force_update() { return m_forceUpdate; }

		void push_opacity(const animated_value<float>& _opacity) { m_opacity = _opacity; }
		void push_opacity(float _start, float _end) { m_opacity = animated_value<float>(_start, _end, _start); }
		float opacity() { return m_opacity.value(); }

		void process_animation(bool);
		void reset_animation();

	private:
		unsigned int m_id;
		std::string m_name;

		unsigned int m_flags;
		bool m_forceUpdate;

		animated_value<float> m_opacity;
		std::shared_ptr<timer_ms> m_animationProcessor;
	};
}