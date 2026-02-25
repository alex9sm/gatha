#define _CRT_SECURE_NO_WARNINGS
#define CGLTF_IMPLEMENTATION
#include "../../dependencies/cgltf.h"

#include "asset.hpp"
#include "../core/log.hpp"
#include "../core/math.hpp"
#include "../core/memory.hpp"
#include "../core/string.hpp"
#include "../core/array.hpp"
#include "../renderer/opengl/vertex.hpp"
#include "../renderer/opengl/mesh.hpp"
#include "../renderer/opengl/texture.hpp"

namespace asset {

	namespace {
		arr::Array<Asset> registry;
	}

	static void extract_name(const char* filepath, char* out, usize out_size) {
		const char* last_sep = nullptr;
		for (const char* p = filepath; *p; p++) {
			if (*p == '/' || *p == '\\') last_sep = p;
		}

		const char* filename = last_sep ? last_sep + 1 : filepath;

		const char* dot = nullptr;
		for (const char* p = filename; *p; p++) {
			if (*p == '.') dot = p;
		}

		usize name_len = dot ? (usize)(dot - filename) : str::length(filename);
		if (name_len >= out_size) name_len = out_size - 1;
		memory::copy(out, filename, name_len);
		out[name_len] = '\0';
	}

	static u32 read_accessor_floats(const cgltf_accessor* accessor, f32* out, u32 component_count) {
		if (!accessor) return 0;
		for (cgltf_size i = 0; i < accessor->count; i++) {
			cgltf_accessor_read_float(accessor, i, out + i * component_count, component_count);
		}
		return (u32)accessor->count;
	}

	static u32 read_accessor_indices(const cgltf_accessor* accessor, u32* out) {
		if (!accessor) return 0;
		for (cgltf_size i = 0; i < accessor->count; i++) {
			cgltf_uint val = 0;
			cgltf_accessor_read_uint(accessor, i, &val, 1);
			out[i] = (u32)val;
		}
		return (u32)accessor->count;
	}

	// Both cgltf and our mat4 are column-major, so direct copy works
	static mat4 float16_to_mat4(const cgltf_float* m) {
		mat4 result;
		memory::copy(&result.col[0][0], m + 0,  4 * sizeof(f32));
		memory::copy(&result.col[1][0], m + 4,  4 * sizeof(f32));
		memory::copy(&result.col[2][0], m + 8,  4 * sizeof(f32));
		memory::copy(&result.col[3][0], m + 12, 4 * sizeof(f32));
		return result;
	}

	static void extract_directory(const char* filepath, char* out, usize out_size) {
		const char* last_sep = nullptr;
		for (const char* p = filepath; *p; p++) {
			if (*p == '/' || *p == '\\') last_sep = p;
		}
		if (last_sep) {
			usize len = (usize)(last_sep - filepath);
			if (len >= out_size) len = out_size - 1;
			memory::copy(out, filepath, len);
			out[len] = '\0';
		} else {
			out[0] = '.';
			out[1] = '\0';
		}
	}

	static const cgltf_accessor* find_attribute(const cgltf_primitive* prim, cgltf_attribute_type type) {
		for (cgltf_size i = 0; i < prim->attributes_count; i++) {
			if (prim->attributes[i].type == type) return prim->attributes[i].data;
		}
		return nullptr;
	}

	i32 load(const char* filepath) {
		for (usize i = 0; i < registry.count; i++) {
			if (str::equal(registry.data[i].path, filepath)) return (i32)i;
		}

		cgltf_options options = {};
		cgltf_data* data = nullptr;
		cgltf_result result = cgltf_parse_file(&options, filepath, &data);
		if (result != cgltf_result_success) {
			logger::error("asset: failed to parse '%s' (cgltf error %d)", filepath, result);
			return -1;
		}

		result = cgltf_load_buffers(&options, data, filepath);
		if (result != cgltf_result_success) {
			logger::error("asset: failed to load buffers for '%s' (cgltf error %d)", filepath, result);
			cgltf_free(data);
			return -1;
		}

		u32 total_vertices = 0;
		u32 total_indices = 0;

		for (cgltf_size n = 0; n < data->nodes_count; n++) {
			cgltf_node* node = &data->nodes[n];
			if (!node->mesh) continue;

			cgltf_mesh* mesh = node->mesh;
			for (cgltf_size p = 0; p < mesh->primitives_count; p++) {
				cgltf_primitive* prim = &mesh->primitives[p];
				const cgltf_accessor* pos_acc = find_attribute(prim, cgltf_attribute_type_position);
				if (pos_acc) total_vertices += (u32)pos_acc->count;
				if (prim->indices) total_indices += (u32)prim->indices->count;
			}
		}

		if (total_vertices == 0) {
			logger::error("asset: '%s' has no geometry", filepath);
			cgltf_free(data);
			return -1;
		}

		arr::Array<opengl::Vertex> vertices = {};
		arr::Array<u32> indices = {};
		arr::array_reserve(&vertices, total_vertices);
		arr::array_reserve(&indices, total_indices > 0 ? total_indices : total_vertices);

		f32* temp_floats = (f32*)memory::malloc(total_vertices * 4 * sizeof(f32));
		u32* temp_indices = (u32*)memory::malloc(total_indices * sizeof(u32));

		vec3 bounds_min = {  1e18f,  1e18f,  1e18f };
		vec3 bounds_max = { -1e18f, -1e18f, -1e18f };
		u32 vertex_offset = 0;

		for (cgltf_size n = 0; n < data->nodes_count; n++) {
			cgltf_node* node = &data->nodes[n];
			if (!node->mesh) continue;

			cgltf_float world_xform[16];
			cgltf_node_transform_world(node, world_xform);
			mat4 world = float16_to_mat4(world_xform);

			cgltf_mesh* mesh = node->mesh;

			for (cgltf_size p = 0; p < mesh->primitives_count; p++) {
				cgltf_primitive* prim = &mesh->primitives[p];

				const cgltf_accessor* pos_acc = find_attribute(prim, cgltf_attribute_type_position);
				if (!pos_acc) continue;
				u32 vert_count = (u32)pos_acc->count;

				read_accessor_floats(pos_acc, temp_floats, 3);

				const cgltf_accessor* norm_acc = find_attribute(prim, cgltf_attribute_type_normal);
				f32* norm_data = (f32*)memory::malloc(vert_count * 3 * sizeof(f32));
				if (norm_acc) {
					read_accessor_floats(norm_acc, norm_data, 3);
				} else {
					for (u32 i = 0; i < vert_count; i++) {
						norm_data[i * 3 + 0] = 0.0f;
						norm_data[i * 3 + 1] = 0.0f;
						norm_data[i * 3 + 2] = 1.0f;
					}
				}

				const cgltf_accessor* uv_acc = find_attribute(prim, cgltf_attribute_type_texcoord);
				f32* uv_data = (f32*)memory::malloc(vert_count * 2 * sizeof(f32));
				if (uv_acc) {
					read_accessor_floats(uv_acc, uv_data, 2);
				} else {
					memory::set(uv_data, 0, vert_count * 2 * sizeof(f32));
				}

				const cgltf_accessor* tan_acc = find_attribute(prim, cgltf_attribute_type_tangent);
				f32* tan_data = (f32*)memory::malloc(vert_count * 4 * sizeof(f32));
				if (tan_acc) {
					read_accessor_floats(tan_acc, tan_data, 4);
				} else {
					for (u32 i = 0; i < vert_count; i++) {
						tan_data[i * 4 + 0] = 1.0f;
						tan_data[i * 4 + 1] = 0.0f;
						tan_data[i * 4 + 2] = 0.0f;
						tan_data[i * 4 + 3] = 1.0f;
					}
				}

				for (u32 i = 0; i < vert_count; i++) {
					vec3 pos = { temp_floats[i*3+0], temp_floats[i*3+1], temp_floats[i*3+2] };
					vec3 norm = { norm_data[i*3+0], norm_data[i*3+1], norm_data[i*3+2] };
					vec2 uv = { uv_data[i*2+0], uv_data[i*2+1] };
					vec4 tan = { tan_data[i*4+0], tan_data[i*4+1], tan_data[i*4+2], tan_data[i*4+3] };

					pos = mat4_transform_point(world, pos);

					if (pos.x < bounds_min.x) bounds_min.x = pos.x;
					if (pos.y < bounds_min.y) bounds_min.y = pos.y;
					if (pos.z < bounds_min.z) bounds_min.z = pos.z;
					if (pos.x > bounds_max.x) bounds_max.x = pos.x;
					if (pos.y > bounds_max.y) bounds_max.y = pos.y;
					if (pos.z > bounds_max.z) bounds_max.z = pos.z;

					norm = mat4_transform_dir(world, norm);
					f32 norm_len = length(norm);
					if (norm_len > 0.0001f) norm = norm * (1.0f / norm_len);

					// Transform tangent xyz, preserve w (bitangent handedness)
					vec3 tan_xyz = mat4_transform_dir(world, { tan.x, tan.y, tan.z });
					f32 tan_len = length(tan_xyz);
					if (tan_len > 0.0001f) tan_xyz = tan_xyz * (1.0f / tan_len);
					tan = { tan_xyz.x, tan_xyz.y, tan_xyz.z, tan.w };

					arr::array_push(&vertices, opengl::Vertex{ pos, norm, uv, tan });
				}

				if (prim->indices) {
					u32 idx_count = (u32)prim->indices->count;
					read_accessor_indices(prim->indices, temp_indices);
					for (u32 i = 0; i < idx_count; i++) {
						arr::array_push(&indices, temp_indices[i] + vertex_offset);
					}
				} else {
					for (u32 i = 0; i < vert_count; i++) {
						arr::array_push(&indices, vertex_offset + i);
					}
				}

				memory::free(norm_data);
				memory::free(uv_data);
				memory::free(tan_data);

				vertex_offset += vert_count;
			}
		}

		memory::free(temp_floats);
		memory::free(temp_indices);

		// Extract texture from first material's baseColorTexture
		opengl::GLuint texture = 0;
		if (data->materials_count > 0) {
			cgltf_material* mat = &data->materials[0];
			if (mat->has_pbr_metallic_roughness &&
				mat->pbr_metallic_roughness.base_color_texture.texture &&
				mat->pbr_metallic_roughness.base_color_texture.texture->image) {
				const char* uri = mat->pbr_metallic_roughness.base_color_texture.texture->image->uri;
				if (uri) {
					char dir[256];
					extract_directory(filepath, dir, sizeof(dir));
					char tex_path[512];
					str::format(tex_path, sizeof(tex_path), "%s/%s", dir, uri);
					texture = opengl::texture_load(tex_path);
				}
			}
		}

		cgltf_free(data);

		opengl::Mesh gpu_mesh = opengl::mesh_create(
			vertices.data, (u32)vertices.count,
			indices.data, (u32)indices.count
		);

		Asset asset = {};
		extract_name(filepath, asset.name, sizeof(asset.name));
		str::copy(asset.path, filepath, sizeof(asset.path));
		asset.mesh = gpu_mesh;
		asset.texture = texture;
		asset.bounds = { bounds_min, bounds_max };
		asset.vertex_count = (u32)vertices.count;
		asset.index_count = (u32)indices.count;

		i32 id = (i32)registry.count;
		arr::array_push(&registry, asset);

		logger::info("asset: loaded '%s' (verts=%u, indices=%u)", filepath, asset.vertex_count, asset.index_count);

		arr::array_destroy(&vertices);
		arr::array_destroy(&indices);

		return id;
	}

	Asset* get(u32 id) {
		if (id >= registry.count) return nullptr;
		return &registry.data[id];
	}

	Asset* find(const char* name) {
		for (usize i = 0; i < registry.count; i++) {
			if (str::equal(registry.data[i].name, name)) return &registry.data[i];
		}
		return nullptr;
	}

	i32 find_id(const char* name) {
		for (usize i = 0; i < registry.count; i++) {
			if (str::equal(registry.data[i].name, name)) return (i32)i;
		}
		return -1;
	}

	void shutdown() {
		for (usize i = 0; i < registry.count; i++) {
			opengl::mesh_destroy(&registry.data[i].mesh);
			opengl::texture_destroy(registry.data[i].texture);
		}
		arr::array_destroy(&registry);
		logger::info("asset: shutdown");
	}

}
