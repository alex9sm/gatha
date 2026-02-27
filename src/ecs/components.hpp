#pragma once

#include "../core/types.hpp"
#include "../core/math.hpp"
#include "ecs.hpp"

namespace ecs {

	struct Transform {
		vec3 position;
		vec3 rotation; // euler angles (yaw, pitch, roll)
		vec3 scale;
		mat4 local_to_world;
	};

	struct MeshInstance {
		u32 asset_id;
	};

	struct HierarchyNode {
		Entity parent;
		char   name[64];
	};

}
