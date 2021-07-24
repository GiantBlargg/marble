#pragma once

#include "model.hpp"
#include "render.hpp"
#include <string>

namespace Render {

Model importModel(std::string filename);

TextureHandle importSkybox(std::string filename);

} // namespace Render