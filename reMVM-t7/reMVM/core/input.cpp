#include "input.h"

mvm::input::input(const std::string& _path) : m_uMsg(NULL), m_wParam(NULL), m_lParam(NULL)
{
	m_mouseState = std::make_shared<mvm::mouse_state>();
	m_bindingStore = std::make_shared<mvm::key_binding_store>(_path);
}

mvm::input::~input()
{

}

void mvm::input::update(bool _forceUpdate, UINT _uMsg, WPARAM _wParam, LPARAM _lParam)
{
	if (_forceUpdate) {
		m_uMsg = _uMsg;
		m_wParam = _wParam;
		m_lParam = _lParam;
	}
	else m_uMsg = m_wParam = m_lParam = NULL;

	if (m_bindingStore->capturing()) {
		if (m_uMsg == WM_KEYUP)
			m_bindingStore->capture_key(m_wParam);
	}
	else handle_mouse();
}

bool mvm::input::key_down(int _key) { return (GetAsyncKeyState(_key) & 0x8000) ? true : false; }
bool mvm::input::key_up(int _key) { return (m_uMsg == WM_KEYUP && m_wParam == _key) ? true : false; }

bool mvm::input::handle_key(const std::string& _name, bool _holding, std::function<void(void)> _function)
{
	if (!m_bindingStore)
		return false;

	if (auto _instance = m_bindingStore->find(_name)) {
		bool _state = _holding ? key_down(_instance->key) : key_up(_instance->key);
		if (_state && _function)
			_function();
		return _state;
	}
	return false;
}

bool mvm::input::handle_mouse()
{
	if (!m_mouseState)
		return false;

	if (m_uMsg == WM_MOUSEMOVE) {
		RECT rect; POINT pt;

		if (!GetWindowRect(GetForegroundWindow(), &rect) || !GetCursorPos(&pt))
			return false;

		m_mouseState->x = ((rect.right + rect.left) / 2) - pt.x;
		m_mouseState->y = ((rect.bottom + rect.top) / 2) - pt.y;
	}

	if (m_uMsg == WM_MOUSEWHEEL)
		m_mouseState->wheel += (float)GET_WHEEL_DELTA_WPARAM(m_wParam) / (float)WHEEL_DELTA;

	return true;
}

int mvm::input::find_key(const std::string& _name)
{
	if (!m_bindingStore)
		return -1;
	auto _instance = m_bindingStore->find(_name);
	return _instance ? _instance->key : -1;
}