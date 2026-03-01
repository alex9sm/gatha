#pragma once

#include "../core/types.hpp"
#include "../core/math.hpp"

namespace asset {

	struct Asset {
		char  name[64];
		char  path[256];
		u32   vao;
		u32   vbo;
		u32   ibo;
		u32   texture;
		AABB  bounds;
		u32   vertex_count;
		u32   index_count;
	};

	i32   load(const char* filepath);
	Asset* get(u32 id);
	Asset* find(const char* name);
	i32   find_id(const char* name);
	void  shutdown();

}
