#ifndef _H__RENDER_H
#define _H__RENDER_H
#include <assets/math.h>
#include <assets/shaders_store.h>
#include <assets/timer.h>

#include <d3d11.h>
#include <memory>

namespace mvm
{
	class metrics
	{
	public:
		metrics();
		~metrics();

		void begin();
		void end();

		int fps() { return m_fps; }
		double frame_time() { return m_frameTime; }

	private:
		std::shared_ptr<timer_ms> m_timeHandler;

		int m_fps;
		double m_frameTime;

		double m_interval, m_skipedTime;

	};

	class render
	{
	public:
		render();
		~render();

		bool initialize();
		bool shutdown();

		bool initialized() { return m_initialized; }

		const RECT render_rect();

		unsigned int width() { return m_width; }
		unsigned int height() { return m_height; }

		bool world_to_screen(vector3&, const vector3&, const matrix44&, const matrix44&);

		void update(IDXGISwapChain*);
		void on_resize(IDXGISwapChain*, unsigned int, unsigned int);

		IDXGISwapChain* swap_chain() { return m_swapChain; }
		ID3D11Device* device() { return m_device; }
		ID3D11DeviceContext* device_context() { return m_context; }
		ID3D11RenderTargetView* render_target() { return m_renderTargetView; }

		mvm::shaders_store* shaders_store() { return m_shadersStore.get(); }
		mvm::metrics* metrics() { return m_metrics.get(); }

	private:
		static long __stdcall present(IDXGISwapChain*, unsigned long, unsigned long);
		static long __stdcall resize_buffers(IDXGISwapChain*, unsigned long, unsigned long, unsigned long, DXGI_FORMAT, unsigned long);

		bool m_initialized;

		unsigned int m_width, m_height;

		IDXGISwapChain* m_swapChain;
		ID3D11Device* m_device;
		ID3D11DeviceContext* m_context;
		ID3D11RenderTargetView* m_renderTargetView;

		std::shared_ptr<mvm::shaders_store> m_shadersStore;
		std::shared_ptr<mvm::metrics> m_metrics;
	};
};
#endif