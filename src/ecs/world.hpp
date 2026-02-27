#pragma once

#include "ecs.hpp"
#include "components.hpp"

namespace ecs {

	struct World {
		EntityPool              pool;
		Store<Transform>        transforms;
		Store<MeshInstance>     mesh_instances;
		Store<HierarchyNode>    hierarchy;
	};

	inline void world_init(World* world) {
		pool_init(&world->pool);
		store_init(&world->transforms);
		store_init(&world->mesh_instances);
		store_init(&world->hierarchy);
	}

	inline void world_destroy(World* world) {
		store_destroy(&world->hierarchy);
		store_destroy(&world->mesh_instances);
		store_destroy(&world->transforms);
		pool_destroy(&world->pool);
	}

}
