#pragma once
#include "backend.hpp"
#include <filesystem>
namespace theater::preferences {
options sanitize(options value);
options load(const std::filesystem::path& path="MVM/Theater/settings.json");
bool save(options value,const std::filesystem::path& path="MVM/Theater/settings.json");
}
