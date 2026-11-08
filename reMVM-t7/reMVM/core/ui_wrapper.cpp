#include "ui_wrapper.h"

using namespace mvm;
#include "../mvm.h"

#include <imgui.h>
#include <imguizmo.h>
#include <imgui_ext.h>
#include <imgui_icons.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>

#include <algorithm>
#include <string>
#include <format>

#if not MVM_RELEASE
namespace mvm
{
	class registered_console
	{
	public:
		registered_console() : m_title(), m_scrollDown(false) { }
		registered_console(const std::string& _title) : m_title(_title), m_scrollDown(false) { }
		~registered_console() {}

		void print(const std::string& _fmt)
		{
			m_buffer.append(_fmt.c_str());
			m_scrollDown = true;
		}

		void clear() { m_buffer.clear(); }

		void render()
		{
			ImGui::Begin(m_title.c_str(), nullptr);
			{
				ImGui::TextUnformatted(m_buffer.begin());
				if (m_scrollDown)
					ImGui::SetScrollHereY(1.0f);
				m_scrollDown = false;
			}
			ImGui::End();
		}

	private:
		std::string m_title;

		ImGuiTextBuffer m_buffer;
		bool m_scrollDown;

		friend class ui_wrapper;
	};
}
#endif

ui_wrapper::ui_wrapper() : m_initialized(false), m_forceUpdate(false)
{

}

ui_wrapper::~ui_wrapper()
{
}

bool ui_wrapper::initialize()
{
	ImGui::CreateContext();

	if (!ImGui_ImplWin32_Init(mvm::_core->hwnd()))
		return false;

	if (!ImGui_ImplDX11_Init(mvm::_core->render()->device(), mvm::_core->render()->device_context()))
		return false;

	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = io.LogFilename = nullptr;

	if (std::filesystem::exists(mvm::_path / "mvm_font.ttf")) {
		auto _mainFont = io.Fonts->AddFontFromFileTTF((mvm::_path / "mvm_font.ttf").string().c_str(), 20.0f);
		m_fonts.insert({ "main_font ", _mainFont });
	}
	else m_fonts.insert({ "main_font", io.Fonts->AddFontDefault() });

	static const ImWchar font_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
	ImFontConfig font_cfg;
	font_cfg.MergeMode = true;
	font_cfg.PixelSnapH = true;
	io.Fonts->AddFontFromMemoryTTF(g_fontAwesomeFive, sizeof(g_fontAwesomeFive), 16, &font_cfg, font_ranges);

	ImGui::GetStyle().WindowBorderSize = 0.0f;

#if not MVM_RELEASE
	initialize_console("MVM CONSOLE");
#endif

	return true;
}

void ui_wrapper::shutdown()
{
	reset();
}

void ui_wrapper::reset()
{
	if (!m_initialized)
		return;

	if (m_forceUpdate)
		toggle();

	m_initialized = false;

	ImGui_ImplDX11_InvalidateDeviceObjects();
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
}

void ui_wrapper::update()
{
	if (!m_initialized) {
		m_initialized = initialize();
		return;
	}

	ImGuiIO& io = ImGui::GetIO();
	io.MouseDrawCursor = m_forceUpdate;

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	{
		ImGuizmo::SetOrthographic(NULL);
		ImGuizmo::BeginFrame();
		ImGuizmo::SetRect(NULL, NULL, io.DisplaySize.x, io.DisplaySize.y);

#if not MVM_RELEASE
		if (m_forceUpdate && m_consoleInitialized)
			m_console->render();
#endif

		for (const auto& _regWnd : m_windows)
			_regWnd.m_window->render(_regWnd.m_manual ? (m_forceUpdate && _regWnd.m_forceUpdate) : m_forceUpdate, _regWnd.m_function);

		for (const auto& _regOverlay : m_overlays)
			_regOverlay.m_overlay->render(_regOverlay.m_forceUpdate, _regOverlay.m_function);
	}
	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void ui_wrapper::toggle()
{
	m_forceUpdate = !m_forceUpdate;

	if (!m_forceUpdate && mvm::_core->focus())
		utils::center_mouse();
}

bool mvm::ui_wrapper::register_window(const std::string& _name, unsigned int _flags, std::function<void(void)> _function, bool _manual)
{
	if (find_window(_name))
		return false;

	m_windows.push_back({ std::make_shared<mvm::window>(_name, _flags), _function, _manual, false });
	return (find_window(_name) != nullptr);
}

mvm::window* mvm::ui_wrapper::find_window(const std::string& _name)
{
	const auto& _it = std::find_if(m_windows.begin(), m_windows.end(), [&](const registered_window& _wnd) { return _wnd.m_window->name() == _name; });
	return _it != m_windows.end() ? _it->m_window.get() : nullptr;
}

bool mvm::ui_wrapper::manual_control(const mvm::window* _window)
{
	const auto& _it = std::find_if(m_windows.begin(), m_windows.end(), [&](const registered_window& _wnd) { return _wnd.m_window.get() == _window; });
	return _it != m_windows.end() ? _it->m_manual : false;
}

void mvm::ui_wrapper::force_window_update(const mvm::window* _window, bool _state)
{
	if (!manual_control(_window))
		return;

	for (auto& _it : m_windows) {
		if (_it.m_window.get() == _window)
			_it.m_forceUpdate = _state;
	}
}

bool mvm::ui_wrapper::register_overlay(const std::string& _name, unsigned int _flags, std::function<void(void)>_function)
{
	if (find_overlay(_name))
		return false;

	m_overlays.push_back({ std::make_shared<mvm::overlay>(_name, _flags), _function, false });
	return (find_window(_name) != nullptr);
}

mvm::overlay* mvm::ui_wrapper::find_overlay(const std::string& _name)
{
	const auto& _it = std::find_if(m_overlays.begin(), m_overlays.end(), [&](const registered_overlay& _overlay) { return _overlay.m_overlay->name() == _name; });
	return _it != m_overlays.end() ? _it->m_overlay.get() : nullptr;
}

void mvm::ui_wrapper::force_overlay_update(const mvm::overlay* _overlay, bool _state)
{
	for (auto& _it : m_overlays) {
		if (_it.m_overlay.get() == _overlay)
			_it.m_forceUpdate = _state;
	}
}

#if not MVM_RELEASE
void mvm::ui_wrapper::initialize_console(const std::string& _conTitle)
{
	m_console = std::make_shared<mvm::registered_console>(_conTitle);
	m_consoleInitialized = true;
}

void mvm::ui_wrapper::push_console_line(const std::string& _fmt)
{
	if (!m_console || !m_consoleInitialized)
		return;

	m_console->print(_fmt);
}
#endif