#pragma once

#include "model.hpp"

#include <filesystem>

namespace Render {

Model load_gltf(std::filesystem::path path, Render& render);

} // namespace Render