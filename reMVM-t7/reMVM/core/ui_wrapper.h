#ifndef _H__UI_WRAPPER_H
#define _H__UI_WRAPPER_H
#include <vector>
#include <unordered_map>

#include <assets/utils.h>
#include <assets/window.h>
#include <assets/overlay.h>

#include "../mvm_build.h"

namespace mvm
{
	struct registered_window
	{
		std::shared_ptr<mvm::window> m_window;
		std::function<void(void)> m_function;

		bool m_manual, m_forceUpdate;
	};

	struct registered_overlay
	{
		std::shared_ptr<mvm::overlay> m_overlay;
		std::function<void(void)> m_function;

		bool m_forceUpdate;
	};

#if not MVM_RELEASE
	class registered_console;
#endif

	class ui_wrapper
	{
	public:
		ui_wrapper();
		~ui_wrapper();

		bool initialize();
		void shutdown();
		void reset();

		void update();
		void toggle();

		void override_state(bool _state) { m_forceUpdate = _state; }
		bool enabled() { return m_forceUpdate; }
		bool initialized() { return m_initialized; }

		void* font(const std::string& _fontName) { return m_fonts[_fontName]; }

		bool register_window(const std::string&, unsigned int, std::function<void(void)>, bool _manual = false);
		mvm::window* find_window(const std::string&);
		bool manual_control(const mvm::window*);
		void force_window_update(const mvm::window*, bool);

		bool register_overlay(const std::string&, unsigned int, std::function<void(void)>);
		mvm::overlay* find_overlay(const std::string&);
		void force_overlay_update(const mvm::overlay*, bool);

#if not MVM_RELEASE
		void initialize_console(const std::string&);
		void push_console_line(const std::string&);
#endif

	private:
		bool m_initialized, m_forceUpdate;

		std::unordered_map<std::string, void*> m_fonts;

		std::vector<registered_window> m_windows;
		std::vector<registered_overlay> m_overlays;

#if not MVM_RELEASE
		bool m_consoleInitialized;
		std::shared_ptr<registered_console> m_console;
#endif
	};
};
#endif