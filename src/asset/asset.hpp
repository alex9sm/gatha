#pragma once

#include "../core/types.hpp"
#include "../core/math.hpp"
#include "../renderer/opengl/mesh.hpp"

namespace asset {

	struct Asset {
		char          name[64];
		char          path[256];
		opengl::Mesh  mesh;
		AABB          bounds;
		u32           vertex_count;
		u32           index_count;
	};

	i32   load(const char* filepath);
	Asset* get(u32 id);
	Asset* find(const char* name);
	i32   find_id(const char* name);
	void  shutdown();

}
