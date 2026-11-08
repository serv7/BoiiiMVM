#include "render.h"

#include <assets/function.h>

#include "../mvm.h"
#include <kiero/kiero.h>
#include <DirectXMath.h>

static utils::function _present = NULL, _resizeBuffers = NULL;
long __stdcall mvm::render::present(IDXGISwapChain* pSwapChain, unsigned long SyncInterval, unsigned long Flags)
{
	mvm::_core->render()->m_metrics->end();

	if (!mvm::_core->hwnd()) {
		DXGI_SWAP_CHAIN_DESC desc;
		pSwapChain->GetDesc(&desc);
		_core->assign_hwnd(desc.OutputWindow);
		_core->assign_wnd_proc(desc.OutputWindow);
	}

	mvm::_core->render()->update(pSwapChain);
	if (mvm::_core->ui_wrapper())
		mvm::_core->ui_wrapper()->update();
	mvm::update_render();

	mvm::_core->render()->m_metrics->begin();
	return _present.invoke<long>(pSwapChain, SyncInterval, Flags);
}

long __stdcall mvm::render::resize_buffers(IDXGISwapChain* pSwapChain, unsigned long BufferCount, unsigned long Width, unsigned long Height, DXGI_FORMAT NewFormat, unsigned long SwapChainFlags)
{
	if (mvm::_core->ui_wrapper())
		mvm::_core->ui_wrapper()->reset();
	mvm::_core->render()->on_resize(pSwapChain, Width, Height);

	return _resizeBuffers.invoke<long>(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
}

mvm::render::render() : m_initialized(false)
{
	m_width = m_height = NULL;

	m_swapChain = nullptr;
	m_device = nullptr;
	m_context = nullptr;
	m_renderTargetView = nullptr;
}

mvm::render::~render()
{

}

bool mvm::render::initialize()
{
	m_metrics = std::make_shared<mvm::metrics>();
	while (!_present || !_resizeBuffers) {
		if (kiero::init(kiero::RenderType::D3D11) == kiero::Status::Success) {
			kiero::bind(8, _present.address_of(), render::present);
			kiero::bind(13, _resizeBuffers.address_of(), render::resize_buffers);
		}
	}

	while (!m_swapChain || !m_device || !m_context)
		continue;

	m_shadersStore = std::make_shared<mvm::shaders_store>(this);

	m_initialized = true;
	return m_initialized;
}

bool mvm::render::shutdown()
{
	kiero::unbind(8);
	kiero::unbind(13);
	kiero::shutdown();

	m_initialized = false;
    return true;
}

const RECT mvm::render::render_rect()
{
	DXGI_SWAP_CHAIN_DESC _desc;
	m_swapChain->GetDesc(&_desc);

	RECT _vp;
	if (!GetClientRect(_desc.OutputWindow, &_vp))
		return { 0, 0, 0, 0 };

	return _vp;
}

bool mvm::render::world_to_screen(vector3& _out, const vector3& _vec, const matrix44& _view, const matrix44& _proj)
{
	const RECT _vp = render_rect();
	
	matrix44 _viewProj = _view * _proj;
	vector4 _point = _viewProj.transform_point(_vec);

	_point *= 0.5f / _point.w;
	_point += vector4(0.5f, 0.5f, 0.0f, 0.0f);
	_point.y = 1.0f - _point.y;
	
	_point.x = (_point.x * (float)(_vp.right - _vp.left)) + (float)_vp.left;
	_point.y = (_point.y * (float)(_vp.bottom - _vp.top)) + (float)_vp.top;

	_out.x = _point.x;
	_out.y = _point.y;
	_out.z = _point.z;

	return true;
}

void mvm::render::update(IDXGISwapChain* swap_chain)
{
	m_swapChain = swap_chain;

	if (!m_device)
	{
		m_swapChain->GetDevice(__uuidof(m_device), reinterpret_cast<void**>(&m_device));
		m_device->GetImmediateContext(&m_context);

		ID3D11Texture2D* render_target = nullptr;
		m_swapChain->GetBuffer(NULL, __uuidof(render_target), reinterpret_cast<void**>(&render_target));
		if (render_target) {
			m_device->CreateRenderTargetView(render_target, nullptr, &m_renderTargetView);
			render_target->Release();
		}

		DXGI_SWAP_CHAIN_DESC desc;
		m_swapChain->GetDesc(&desc);
		m_height = desc.BufferDesc.Height;
		m_width = desc.BufferDesc.Width;
	}
	m_context->OMSetRenderTargets(1, &m_renderTargetView, nullptr);
}

void mvm::render::on_resize(IDXGISwapChain* swap_chain, unsigned int width, unsigned int height)
{
	if (m_renderTargetView)
	{
		if (m_context)
			m_context->OMSetRenderTargets(NULL, NULL, NULL);
		m_renderTargetView->Release();
		m_renderTargetView = nullptr;
	}

	if (m_context)
		m_context->OMSetRenderTargets(1, &m_renderTargetView, NULL);

	D3D11_VIEWPORT vp;
	vp.Width = width;
	vp.Height = height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = NULL;
	vp.TopLeftY = NULL;

	m_width = width;
	m_height = height;

	if (m_context)
		m_context->RSSetViewports(1, &vp), m_context->Release();

	if (m_device)
		m_device->Release(), m_device = nullptr;
}

mvm::metrics::metrics()
{
	m_timeHandler = std::make_shared<timer_ms>();

	m_interval = 1.0;
	m_skipedTime = 0.0;
}

mvm::metrics::~metrics()
{

}

void mvm::metrics::begin()
{
	m_timeHandler->start();
}

void mvm::metrics::end()
{
	m_timeHandler->stop();

	m_frameTime = m_timeHandler->elapsed() / 1000.0;
	m_skipedTime -= m_timeHandler->elapsed();
	if (m_skipedTime <= 0.0)
		m_fps = (int)(1000.0 / m_timeHandler->elapsed()), m_skipedTime = m_interval * 1000.0;
}