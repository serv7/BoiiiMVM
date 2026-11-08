#pragma once
#include <functional>
#include <memory>
#include <string>

#include <assets/animated_value.h>
#include <assets/math.h>
#include <assets/timer.h>
#include <assets/utils.h>

namespace mvm
{
	class dpi
	{
	public:
		void initialize(float _ratioX, float _ratioY)
		{
			m_ratio.x = _ratioX;
			m_ratio.y = _ratioY;

			m_scale = 1.0f;
		}

		void rescale(float _vpX, float _vpY, float _scalableX, float _scalableY)
		{
			m_scale.x = _vpX / (_scalableX * m_ratio.x);
			m_scale.y = _vpY / (_scalableY * m_ratio.y);
		}

		const vector2& scale() { return m_scale; }
		float x() { return m_scale.x; }
		float y() { return m_scale.y; }

	private:
		vector2 m_ratio, m_scale;

	};

	constexpr static unsigned __int8 mvm_window_flags_none = utils::bit(0);
	constexpr static unsigned __int8 mvm_window_flags_disabled = utils::bit(1);
	constexpr static unsigned __int8 mvm_window_flags_animated = utils::bit(2);
	constexpr static unsigned __int8 mvm_window_flags_no_input = utils::bit(3);

	class window
	{
	public:
		window(const std::string&, unsigned int);
		~window();

		void render(bool, std::function<void(void)>);

		unsigned int id() { return m_id; }
		const std::string& name() { return m_name; }

		unsigned int flags() { return m_flags; }
		bool disabled() { return utils::flags::exists(m_flags, mvm_window_flags_disabled); }
		bool animated() { return utils::flags::exists(m_flags, mvm_window_flags_animated); }

		void update_state(bool _state) { m_forceUpdate = _state; }
		bool force_update() { return m_forceUpdate; }

		float width() { return m_size.x; }
		float height() { return m_size.y; }
		void resize(const vector2&);

		float item_width() { return m_itemSize.x; }
		float item_height() { return m_itemSize.y; }
		void resize_items(const vector2&);

		mvm::dpi* dpi() { return m_dpi.get(); }

		void push_position(const animated_value<vector2>& _position) { m_position = _position; }
		void push_position(const vector2& _start, const vector2& _end) { m_position = animated_value<vector2>(_start, _end, _start); }

		void push_opacity(const animated_value<float>& _opacity) { m_opacity = _opacity; }
		void push_opacity(float _start, float _end) { m_opacity = animated_value<float>(_start, _end, _start); }

		vector2 position() { return m_position.value(); }
		float opacity() { return m_opacity.value(); }

		void process_animation(bool);
		void reset_animation();

	private:
		unsigned int m_id;
		std::string m_name;

		unsigned int m_flags;
		bool m_forceUpdate;

		vector2 m_size, m_itemSize;
		std::shared_ptr<mvm::dpi> m_dpi;

		animated_value<vector2> m_position;
		animated_value<float> m_opacity;
		std::shared_ptr<timer_ms> m_animationProcessor;
	};
}