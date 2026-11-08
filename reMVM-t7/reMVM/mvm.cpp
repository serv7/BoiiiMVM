#include "mvm.h"

#include "game/assets/addresses.h"
#include <assets/patch.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_card.h>
#include <imgui_ext.h>
#include <imgui_internal.h>

#include "ui/main_window.h"

void mvm::initialize()
{
	_path = std::filesystem::current_path() / "MVM";
	std::filesystem::create_directories(_path);

	_core = std::make_shared<mvm::core>();
	_core->initialize();
}

void mvm::shutdown()
{
	_core->shutdown();
}

void mvm::update_render()
{
	if (!_core || !_core->game_context())
		return;

}

void mvm::create_input_binding(bool _override)
{
	if (!_core->input())
		return;

	auto _bindingStore = _core->input()->binding_store();

	_bindingStore->bind("Toggle UI", VK_TAB);
	_bindingStore->bind("Toggle HUD", VK_DELETE);

	_bindingStore->bind("Add Key", 'F');
	_bindingStore->bind("Remove Key", 'G');

	_bindingStore->save();
}

void mvm::handle_keys()
{
	if (_core->input()->binding_store()->capturing())
		return;

	if (_core->game_context()->in_theater()) {
		_core->input()->handle_key("Toggle UI", false, std::bind(&ui_wrapper::toggle, _core->ui_wrapper()));
		_core->input()->handle_key("Toggle HUD", false, [&]{ if (gc::game::_hideHud) *gc::game::_hideHud = !(*gc::game::_hideHud); });

		auto _directorCamera = _core->game_context()->director_camera();
		auto _lightman = _core->game_context()->lightman();

		if (_core->input()->handle_key("Add Key", false)) {
			if (*gc::game::_theaterMode == gc::theater_mode::lighter && _lightman->dolly_mode()) {
				if (auto _light = _lightman->light_dolly(*_lightman->curr_light()); _light)
					_light->insert_marker();
			}
		}

		if (_core->input()->handle_key("Remove Key", false)) {
			if (*gc::game::_theaterMode == gc::theater_mode::lighter && _lightman->dolly_mode()) {
				if (auto _light = _lightman->light_dolly(*_lightman->curr_light()); _light) {
					int _removeMethod = *_lightman->remove_method();
					if (_removeMethod == 0)
						_light->remove_marker(_light->dolly->size() - 1);
					if (_removeMethod == 1)
						_light->remove_marker(_light->nearest_of(gc::game::_theaterCamera->pos));
				}
			}
		}
	}
}

void mvm::register_windows()
{
	auto _uiWrapper = _core->ui_wrapper();
	if (!_uiWrapper)
		return;

	if (_uiWrapper->register_window("main_window", mvm_window_flags_animated, &mvm::main_window)) {
		auto _window = _uiWrapper->find_window("main_window");
		_window->resize({ 360.f, 360.f });
		_window->resize_items({ 360.f, 27.0f });
		_window->push_position({ { (1920.0f - _window->width()) * 0.5f, (1080.0f + _window->height()) * 0.5f },
			{ (1920.0f - _window->width()) * 0.5f, (1080.0f - _window->height()) * 0.5f },
			{ (1920.0f - _window->width()) * 0.5f, (1080.0f + _window->height()) * 0.5f }
		});
		_window->push_opacity({ 0.0f, 0.9f, 0.0f });
		_window->dpi()->initialize(1920.0f / _window->width(), 1080.0f / _window->height());
	}
}

void mvm::update_windows_state()
{

}

void mvm::update_overlays_state()
{

}

void mvm::main_window()
{
	auto _window = _core->ui_wrapper()->find_window("main_window");
	if (!_window)
		return;

	_window->dpi()->rescale((float)_core->render()->width(), (float)_core->render()->height(), _window->width(), _window->height());
	_window->resize_items({ _window->width() - ImGui::GetStyle().WindowPadding.x * 2.0f, 27.0f});
	_window->dpi()->rescale((float)_core->render()->width(), (float)_core->render()->height(), _window->width(), _window->height());

	main_window_layout(_window, _core.get());
}