#include "model.hpp"

#include <algorithm>

namespace Render {

ModelInstance::ModelInstance(Model group, mat4 transform) : render(group.render) {
	instances.resize(group.surfaces.size());
	std::transform(group.surfaces.begin(), group.surfaces.end(), instances.begin(), [this](Model::Surface surface) {
		return Instance{render.surface_create(surface.mesh, surface.material), surface.transform};
	});
	setTransform(transform);
}

ModelInstance::~ModelInstance() {
	for (auto& i : instances) {
		render.surface_delete(std::move(i.handle));
	}
}

void ModelInstance::setTransform(mat4 transform) {
	for (auto& i : instances) {
		render.surface_set_transform(i.handle, transform * i.baseTrans);
	}
}

} // namespace Render