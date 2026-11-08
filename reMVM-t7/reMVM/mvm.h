#pragma once
#include <filesystem>
#include <memory>

#include <core.h>
#include "mvm_build.h"

namespace mvm
{
	inline std::filesystem::path _path;
	inline std::shared_ptr<mvm::core> _core;

	void initialize();
	void shutdown();

	void update_render();

	void create_input_binding(bool _override = false);
	void handle_keys();

	void register_windows();
	void update_windows_state();
	void update_overlays_state();

	void main_window();
}