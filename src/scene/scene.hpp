#pragma once

#include "../core/types.hpp"
#include "../core/array.hpp"
#include "../ecs/ecs.hpp"
#include "../ecs/world.hpp"

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

}
