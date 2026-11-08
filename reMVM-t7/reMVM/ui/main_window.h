#pragma once
#include <assets/utils.h>
#include <assets/window.h>

#include "../mvm.h"

#include <imgui.h>
#include <imgui_card.h>
#include <imgui_ext.h>
#include <imgui_icons.h>

#include "../game/game_context.h"
#include "../game/assets/functions.h"

namespace mvm
{
	inline bool _uiSettingsPage = false;

	inline void main_window_settings_layout(mvm::window* _owner, mvm::core* _core)
	{
		ImGui::Separator();

		auto _input = _core->input();
		if (!_input || !_input->binding_store())
			return;

		const auto& _style = ImGui::GetStyle();
		const ImVec2 _itemsSize = { _owner->item_width(), _owner->item_height() };

		auto _bindingStore = _input->binding_store();
		if (!_bindingStore->capturing()) {
			if (ImGui::Button("Reload Binding", _itemsSize))
				_bindingStore->load();

			if (ImGui::Button("Restore Binding", _itemsSize))
				_bindingStore->instances().clear(), mvm::create_input_binding();

			ImGui::BeginCard("Key Binding");
			{
				const ImVec2 _cardItemsSize = { _itemsSize.x - _style.WindowPadding.x * 2.0f, _itemsSize.y };
				for (auto _it : _bindingStore->instances()) {
					if (ImGui::Button(std::format("{} | {}", _it.name, utils::string::vk_to_string(_it.key)).c_str(), _cardItemsSize))
						_bindingStore->begin_capturing(_it.name);
				}
			}
			ImGui::EndCard();
		}

		if (_bindingStore->capturing()) {
			ImGui::TextAligned("INPUT BINDING");
			ImGui::TextAligned(!_bindingStore->captured_key() ? "No captured key..." : std::format("Captured key: {}", utils::string::vk_to_string(_bindingStore->captured_key())));

			ImGui::SetCursorPosY(ImGui::GetCurrentWindow()->Size.y - _itemsSize.y * 4.0f);
			if (ImGui::Button("Accept", { _itemsSize.x, _itemsSize.y }))
				_bindingStore->override_key(_bindingStore->capturing_target(), _bindingStore->captured_key()), _bindingStore->complete_capturing(), _bindingStore->save();
			ImGui::SetItemDefaultFocus();
			ImGui::PushStyleColor(ImGuiCol_Button, { 1.00, 0.00, 0.00, 0.40f });
			if (ImGui::Button("Cancel", { _itemsSize.x, _itemsSize.y }))
				_bindingStore->complete_capturing();
			ImGui::PopStyleColor();
		}
	}

	inline void main_window_layout(mvm::window* _owner, mvm::core* _core)
	{
		const auto& _style = ImGui::GetStyle();
		const ImVec2 _itemsSize = { _owner->item_width(), _owner->item_height() };

		ImGui::BeginDisabled(!_uiSettingsPage);
		if (ImGui::Button((const char*)ICON_FA_ARROW_LEFT, { _itemsSize.y, _itemsSize.y }))
			_uiSettingsPage = false;
		ImGui::EndDisabled();

		ImGui::SameLine((ImGui::GetWindowSize().x - ImGui::CalcTextSize("reMVM").x) * 0.5f);
		ImGui::Text("reMVM");
		ImGui::SameLine(ImGui::GetWindowSize().x - ImGui::GetStyle().WindowPadding.x - _itemsSize.y);
		if (ImGui::Button((const char*)ICON_FA_TIMES, { _itemsSize.y, _itemsSize.y })) {
			_core->ui_wrapper()->toggle();
			_core->input()->binding_store()->complete_capturing();
		}

		if (_uiSettingsPage) {
			main_window_settings_layout(_owner, _core);
		}
		else {
			auto _gameCtx = _core->game_context();
			if (!_gameCtx)
				return;

			const auto& _style = ImGui::GetStyle();

			const ImVec2 _displaySize = { _owner->width(), _owner->height() };
			const ImVec2 _itemsSize = { _owner->item_width(), _owner->item_height() };

			if (ImGui::Button("Settings", _itemsSize))
				_uiSettingsPage = true;
			ImGui::Separator();

			if (_gameCtx->in_theater()) {
				if (*gc::game::_theaterMode == gc::theater_mode::basic) {
					ImGui::DragScalar("Field of View", ImGuiDataType_Double, gc::game::_fov, 0.01f);
				}

				auto _directorCamera = _gameCtx->director_camera();
				auto _cameraMgr = _gameCtx->director_camera()->camera_manager();
				if (*gc::game::_theaterMode == gc::theater_mode::director) {
					ImGui::BeginCard("CAMERA", nullptr, true);
					{			
						const ImVec2 _cardItemsSize = { _itemsSize.x - _style.WindowPadding.x * 2.0f, _itemsSize.y };

						if (ImGui::DragFloat("Input scale", _cameraMgr->input_scale(), 0.1f))
							*_cameraMgr->input_scale() = clamp(*_cameraMgr->input_scale(), 0.1f, 2.0f);

						if (auto _camera = gc::game::_camera) {
							ImGui::BeginDisabled(*gc::game::_theaterMode == 2);
							ImGui::DragScalar("Field of View", ImGuiDataType_Double, gc::game::_fov, 0.01f);

							if (ImGui::DragFloat("Temperature", &_camera->temperature, 0.1f))
								_camera->temperature = clamp(_camera->temperature, 1000.0f, 10000.0f);

							ImGui::Text("DoF type");
							static int _dofType = _camera->dof_type;
							ImGui::RadioButton("Off", &_dofType, 0);
							ImGui::SameLine();
							ImGui::RadioButton("On", &_dofType, 1);
							ImGui::SameLine();
							ImGui::RadioButton("On (far only)", &_dofType, 2);
							if (_camera->dof_type != (short)_dofType)
								_camera->dof_type = (short)_dofType;

							ImGui::DragFloat("Focal length", &_camera->focal_length, 0.1f, 0.1f, 0.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);

							ImGui::BeginDisabled(*_directorCamera->auto_focus());
							ImGui::DragFloat("Focus distance", &_camera->focus_distance, 0.1f, 0.1f, 0.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
							ImGui::EndDisabled();
							ImGui::DragFloat("F-Stop", &_camera->fStop, 0.1f, 0.1f, 0.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
							ImGui::EndDisabled();

							ImGui::TextAligned(std::format("Markers: {} | custom: {}", *gc::game::_dollyMarkersCount, _directorCamera->custom_dolly()->size()));
						}
						else ImGui::TextAligned("Camera currently is unavailable...");
					}
					ImGui::EndCard();

					if (gc::game::_camera != nullptr && !gc::game::_playerNames.empty()) {
						ImGui::BeginCard("AUTO-FOCUS", nullptr, true);
						{
							const ImVec2 _cardItemsSize = { _itemsSize.x - _style.WindowPadding.x * 2.0f, _itemsSize.y };
							ImGui::Checkbox("Enabled", _directorCamera->auto_focus());
							ImGui::BeginDisabled(!(*_directorCamera->auto_focus()));
							ImGui::DragFloat3("Focus offset", (float*)_directorCamera->focus_offset(), 0.01f);

							static std::string _currName = gc::game::_playerNames[*_directorCamera->focus_target()];
							if (ImGui::InputInt(_currName.c_str(), _directorCamera->focus_target())) {
								*_directorCamera->focus_target() = iclamp(*_directorCamera->focus_target(), NULL, gc::game::_playerNames.size() - 1);
								_currName = gc::game::_playerNames[*_directorCamera->focus_target()];
							}

							ImGui::EndDisabled();
							if (!gc::game::_joints.empty()) {
								if (ImGui::InputInt(std::format("{}##auto_focus_joint", gc::game::_joints[*_directorCamera->focus_joint()].name.c_str()).c_str(), _directorCamera->focus_joint()))
									*_directorCamera->focus_joint() = iclamp(*_directorCamera->focus_joint(), NULL, gc::game::_joints.size() - 1);
							}
						}
						ImGui::EndCard();
					}
				}

				if (*gc::game::_theaterMode == gc::theater_mode::object_link) {
					ImGui::BeginCard("OBJECT LINK", nullptr, true);
					{
						const ImVec2 _cardItemsSize = { _itemsSize.x - _style.WindowPadding.x * 2.0f, _itemsSize.y };
						ImGui::TextAligned("Coming soon...");
					}
					ImGui::EndCard();
				}

				auto _lightman = _core->game_context()->lightman();
				if (*gc::game::_theaterMode == gc::theater_mode::lighter) {
					ImGui::BeginCard("LIGHTING", nullptr, true);
					{							
						const ImVec2 _cardItemsSize = { _itemsSize.x - _style.WindowPadding.x * 2.0f, _itemsSize.y };
						if (_lightman->lights_buffer().size() > NULL) {

							if (ImGui::InputInt("Light index", _lightman->curr_light()))
								*_lightman->curr_light() = iclamp(*_lightman->curr_light(), NULL, *gc::game::_lightsCount - 1);

							if (ImGui::Button(_lightman->dolly_mode() ? "Stop edit dolly" : "Start edit dolly", _cardItemsSize))
								_lightman->push_dolly_mode(!_lightman->dolly_mode());

							ImGui::TextAligned("LIGHT SOURCE");
							gc::light& _light = gc::game::_lights[*_lightman->curr_light()];

							ImGui::DragFloat3("Position##light", (float*)&_light.position, 0.1f);
							ImGui::SameLine();
							if (ImGui::Button(std::format("{}##position_from_camera", (const char*)ICON_FA_CAMERA).c_str()))
								_lightman->relocate(*_lightman->curr_light());
							ImGui::DragFloat3("Rotation##light", (float*)&_light.lookAt, 0.1f);
							ImGui::SameLine();
							if (ImGui::Button(std::format("{}##lookAt_from_camera", (const char*)ICON_FA_CAMERA).c_str()))
								_lightman->redirect(*_lightman->curr_light());

							ImGui::Text("Source type");
							ImGui::RadioButton("Omni", &_light.type, 0);
							ImGui::SameLine();
							ImGui::RadioButton("Spot", &_light.type, 1);

							ImGui::ColorEdit3("Color", (float*)&_light.color);
							ImGui::DragFloat("Intensity", &_light.intensity, 0.1f, 0.1f);
							float _lightRange = _light.range / 100.0f;
							if (ImGui::DragFloat("Range", &_lightRange, 0.1f, 0.1f))
								_light.range = _lightRange * 100.0f;

							if (auto _lDolly = _lightman->light_dolly(*_lightman->curr_light()); _lDolly) {
								ImGui::TextAligned("LIGHT DOLLY");

								if (_lightman->dolly_mode()) {
									ImGui::Text("Remove method");
									ImGui::RadioButton("Last", _lightman->remove_method(), 0);
									ImGui::SameLine();
									ImGui::RadioButton("By position", _lightman->remove_method(), 1);

									int _removeMethod = *_lightman->remove_method();
									if (ImGui::Button("Remove marker", _cardItemsSize)) {
										if (_removeMethod == 0)
											_lDolly->remove_marker(_lDolly->dolly->size() - 1);
										if (_removeMethod == 1)
											_lDolly->remove_marker(_lDolly->nearest_of(gc::game::_theaterCamera->pos));
									}
								}

								ImGui::TextAligned(std::format("Markers: {}", _lDolly->dolly->size()));
							}
						}
						else ImGui::TextAligned("No light sources...");
					}
					ImGui::EndCard();
				}
			}
			else ImGui::TextAligned("Waiting for theater...");
		}
	}
}