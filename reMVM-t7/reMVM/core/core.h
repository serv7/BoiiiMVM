#pragma once
#include "Assets/utils.h"
#include "Assets/logs.h"

#include "input.h"
#include "render.h"
#include "ui_wrapper.h"

#include <thread_pool.h>
#define THREAD_POOL_SIZE 1

#include "../Game/game_context.h"

namespace mvm
{
	class core
	{
	public:
		core();
		~core();

		void initialize();
		void shutdown();

		void complete_initialization() { m_initialized = true; }
		bool initialized() { return m_initialized; }

		HWND hwnd() { return m_hWnd; }
		void assign_hwnd(HWND hWnd) { m_hWnd = hWnd; }
		void assign_wnd_proc(HWND hWnd) { if (!m_wndProc) m_wndProc = (WNDPROC)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)core::wnd_proc); }

		bool focus() { return m_hWnd == GetForegroundWindow(); }

		BS::thread_pool* thread_pool() { return m_threadPool.get(); }

		mvm::logs* logs() { return m_logs.get(); }
		mvm::input* input() { return m_input.get(); }
		mvm::render* render() { return m_render.get(); }
		mvm::ui_wrapper* ui_wrapper() { return m_uiWrapper.get(); }
		gc::game_context* game_context() { return m_gameContext.get(); }

		static LRESULT __stdcall wnd_proc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	private:
		bool m_initialized;

		HWND m_hWnd;
		WNDPROC m_wndProc;

		std::shared_ptr<BS::thread_pool> m_threadPool;

		std::shared_ptr<mvm::logs> m_logs;
		std::shared_ptr<mvm::input> m_input;
		std::shared_ptr<mvm::render> m_render;
		std::shared_ptr<mvm::ui_wrapper> m_uiWrapper;
		std::shared_ptr<gc::game_context> m_gameContext;

	};
}