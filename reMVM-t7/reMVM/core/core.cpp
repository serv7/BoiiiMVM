#include "core.h"

#include "../mvm.h"

#include <assets/math.h>

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT __stdcall mvm::core::wnd_proc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	mvm::_core->m_input->update(mvm::_core->focus(), uMsg, wParam, lParam);
	mvm::handle_keys();

	if (mvm::_core->m_uiWrapper->initialized()) {
		auto imgui_handler = ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
		if (!mvm::_core->m_uiWrapper->enabled() && uMsg == WM_SETFOCUS)
			utils::center_mouse();
		if (mvm::_core->m_uiWrapper->enabled() && imgui_handler)
			return TRUE;
	}

	return CallWindowProc(mvm::_core->m_wndProc, hWnd, uMsg, wParam, lParam);
}

mvm::core::core()
{
	m_initialized = false;

	m_hWnd = NULL;
	m_wndProc = NULL;
}

mvm::core::~core()
{

}

void mvm::core::initialize()
{
	m_logs = std::make_shared<mvm::logs>();
	m_logs->initialize("reMVM", (mvm::_path / "log.mvm_log").string().c_str());

	m_input = std::make_shared<mvm::input>((mvm::_path / "binding.mvm_binding").string());
	if (m_input->binding_store()->empty())
		mvm::create_input_binding();

	m_render = std::make_shared<mvm::render>();
	m_uiWrapper = std::make_shared<mvm::ui_wrapper>();

	m_gameContext = std::make_shared<gc::game_context>();

	m_threadPool = std::make_shared<BS::thread_pool>(1);
	m_threadPool->push_task([&] {
		m_render->initialize();
		while (!m_render->initialized())
			continue;

		while (!m_uiWrapper->initialized())
			continue;
		mvm::register_windows();

		m_gameContext->initialize();

		complete_initialization();
	});
}

void mvm::core::shutdown()
{
	m_render->shutdown();
	m_uiWrapper->shutdown();

	m_gameContext->shutdown();

	m_initialized = false;
}