#ifndef __H_INPUT_H
#define __H_INPUT_H
#include <Windows.h>

#include <functional>
#include <memory>
#include <string>

#include <assets/key_binding_store.h>

namespace mvm
{
	struct mouse_state
	{
		float x, y, wheel;

		mouse_state() : x(0.0f), y(0.0f), wheel(0.0f) {}
		mouse_state(float _x, float _y, float _wheel) { x = _x; y = _y; wheel = _wheel; }

		void reset() { x = y = wheel = 0.0f; }
		std::string string() const { return std::format("{}, {}, {}", x, y, wheel); }
	};

	class input
	{
	public:
		input(const std::string& _path = "");
		~input();

		void update(bool, UINT, WPARAM, LPARAM);

		bool key_down(int);
		bool key_up(int);

		bool handle_key(const std::string&, bool _holding = false, std::function<void(void)> _function = nullptr);
		bool handle_mouse();

		mvm::mouse_state* mouse_state() { return m_mouseState.get(); }
		mvm::key_binding_store* binding_store() { return m_bindingStore.get(); }

		int find_key(const std::string&);

	private:
		unsigned int m_uMsg;
		WPARAM m_wParam;
		LPARAM m_lParam;

		std::shared_ptr<mvm::mouse_state> m_mouseState;
		std::shared_ptr<mvm::key_binding_store> m_bindingStore;
	};
}
#endif