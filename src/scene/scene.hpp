#pragma once

#include "../core/types.hpp"
#include "../core/array.hpp"
#include "../ecs/ecs.hpp"

namespace ecs { struct World; }

namespace scene {

	struct Scene {
		char                    name[64];
		char                    path[256];
		arr::Array<ecs::Entity> entities;
		bool                    dirty;
	};

	bool load(Scene* scene, const char* filepath, ecs::World* world);
	bool save(const Scene* scene, const ecs::World* world);
	void unload(Scene* scene, ecs::World* world);
	void make_entity_name(const char* base_name, const ecs::World* world, const Scene* scene, char* out, usize out_size);

}
