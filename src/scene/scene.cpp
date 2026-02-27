#include "scene.hpp"
#include "json.hpp"
#include "../core/log.hpp"
#include "../core/file.hpp"
#include "../core/memory.hpp"
#include "../core/string.hpp"
#include "../core/math.hpp"
#include "../asset/asset.hpp"
#include "../ecs/components.hpp"

namespace scene {

	static i32 entity_to_scene_index(const Scene* scene, ecs::Entity e) {
		for (usize i = 0; i < scene->entities.count; i++) {
			if (scene->entities.data[i] == e) return (i32)i;
		}
		return -1;
	}

	void make_entity_name(const char* base_name, const ecs::World* world, const Scene* scene, char* out, usize out_size) {
		u32 count = 0;
		for (usize i = 0; i < scene->entities.count; i++) {
			ecs::Entity e = scene->entities.data[i];
			if (!ecs::store_has(&world->hierarchy, e)) continue;
			const ecs::HierarchyNode* hn = ecs::store_get(
				const_cast<ecs::Store<ecs::HierarchyNode>*>(&world->hierarchy), e);
			if (str::equal(hn->name, base_name)) { count++; continue; }
			char prefix[68];
			str::copy(prefix, base_name, sizeof(prefix));
			str::concat(prefix, ".", sizeof(prefix));
			if (str::starts_with(hn->name, prefix)) count++;
		}
		if (count == 0) {
			str::copy(out, base_name, out_size);
		} else {
			str::format(out, out_size, "%s.%03u", base_name, count);
		}
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

	bool load(Scene* scene, const char* filepath, ecs::World* world) {
		*scene = {};
		extract_name(filepath, scene->name, sizeof(scene->name));
		str::copy(scene->path, filepath, sizeof(scene->path));

		u64 file_size = 0;
		if (!file::get_size(filepath, &file_size)) {
			logger::error("scene: could not find '%s'", filepath);
			return false;
		}

		char* text = (char*)memory::malloc(file_size + 1);
		u64 bytes_read = file::read_file(filepath, text, file_size);
		text[bytes_read] = '\0';

		json::Value* root = json::parse(text, bytes_read);
		memory::free(text);

		if (!root) {
			logger::error("scene: failed to parse JSON '%s'", filepath);
			return false;
		}

		json::Value* assets_arr = json::get(root, "assets");
		u32 asset_count = json::length(assets_arr);
		for (u32 i = 0; i < asset_count; i++) {
			const char* asset_path = json::as_string(json::at(assets_arr, i));
			if (asset_path[0]) asset::load(asset_path);
		}

		json::Value* entities_arr = json::get(root, "entities");
		u32 entity_count = json::length(entities_arr);
		arr::Array<ecs::Entity> index_to_entity = {};

		for (u32 i = 0; i < entity_count; i++) {
			json::Value* ent_json = json::at(entities_arr, i);
			if (!ent_json) continue;

			ecs::Entity e = ecs::pool_create(&world->pool);
			if (e == ecs::INVALID_ENTITY) break;
			arr::array_push(&scene->entities, e);
			arr::array_push(&index_to_entity, e);

			json::Value* t = json::get(ent_json, "transform");
			if (t) {
				ecs::Transform transform = {};
				transform.position = json::to_vec3(json::get(t, "position"));
				transform.rotation = json::to_vec3(json::get(t, "rotation"));
				transform.scale    = json::to_vec3(json::get(t, "scale"), {1, 1, 1});
				transform.local_to_world = mat4_identity();
				ecs::store_add(&world->transforms, e, transform);
			}

			json::Value* mi = json::get(ent_json, "mesh_instance");
			if (mi) {
				const char* asset_name = json::as_string(json::get(mi, "asset"));
				i32 id = asset::find_id(asset_name);
				if (id >= 0) {
					ecs::store_add(&world->mesh_instances, e, { (u32)id });
				} else {
					logger::error("scene: entity references unknown asset '%s'", asset_name);
				}
			}

			ecs::HierarchyNode hn = {};
			hn.parent = ecs::INVALID_ENTITY;

			json::Value* parent_val = json::get(ent_json, "parent");
			if (parent_val) {
				i32 parent_idx = (i32)json::as_number(parent_val, -1.0);
				if (parent_idx >= 0 && parent_idx < (i32)index_to_entity.count) {
					hn.parent = index_to_entity.data[parent_idx];
				}
			}

			const char* base = "Entity";
			if (ecs::store_has(&world->mesh_instances, e)) {
				ecs::MeshInstance* mesh_inst = ecs::store_get(&world->mesh_instances, e);
				asset::Asset* a = asset::get(mesh_inst->asset_id);
				if (a) base = a->name;
			}
			make_entity_name(base, world, scene, hn.name, sizeof(hn.name));
			ecs::store_add(&world->hierarchy, e, hn);
		}

		arr::array_destroy(&index_to_entity);

		json::destroy(root);
		logger::info("scene: loaded '%s' (%u entities)", filepath, (u32)scene->entities.count);
		return true;
	}

	struct Writer {
		arr::Array<char> buf;
		u32 indent;
	};

	static void write_raw(Writer* w, const char* s) {
		while (*s) arr::array_push(&w->buf, *s++);
	}

	static void write_indent(Writer* w) {
		for (u32 i = 0; i < w->indent; i++) { write_raw(w, "  "); }
	}

	static void write_number(Writer* w, f64 val) {
		char tmp[64];
		if (val == (f64)(i64)val && val >= -1e15 && val <= 1e15) {
			str::format(tmp, sizeof(tmp), "%lld", (i64)val);
		} else {
			str::format(tmp, sizeof(tmp), "%.6f", val);
			char* dot = nullptr;
			for (char* p = tmp; *p; p++) { if (*p == '.') dot = p; }
			if (dot) {
				char* end = tmp + str::length(tmp) - 1;
				while (end > dot + 1 && *end == '0') *end-- = '\0';
			}
		}
		write_raw(w, tmp);
	}

	static void write_vec3(Writer* w, vec3 v) {
		write_raw(w, "[");
		write_number(w, v.x); write_raw(w, ", ");
		write_number(w, v.y); write_raw(w, ", ");
		write_number(w, v.z);
		write_raw(w, "]");
	}

	bool save(const Scene* scene, const ecs::World* world) {
		Writer w = {};
		w.indent = 0;

		write_raw(&w, "{\n");

		w.indent = 1;
		write_indent(&w); write_raw(&w, "\"assets\": [\n");
		w.indent = 2;

		arr::Array<u32> seen_assets = {};
		for (usize i = 0; i < scene->entities.count; i++) {
			ecs::Entity e = scene->entities.data[i];
			if (!ecs::store_has(&world->mesh_instances, e)) continue;
			const ecs::MeshInstance* mi = ecs::store_get(
				const_cast<ecs::Store<ecs::MeshInstance>*>(&world->mesh_instances), e);
			bool found = false;
			for (usize j = 0; j < seen_assets.count; j++) {
				if (seen_assets.data[j] == mi->asset_id) { found = true; break; }
			}
			if (!found) arr::array_push(&seen_assets, mi->asset_id);
		}

		for (usize i = 0; i < seen_assets.count; i++) {
			asset::Asset* a = asset::get(seen_assets.data[i]);
			if (!a) continue;
			write_indent(&w); write_raw(&w, "\"");
			write_raw(&w, a->path); write_raw(&w, "\"");
			if (i + 1 < seen_assets.count) write_raw(&w, ",");
			write_raw(&w, "\n");
		}
		arr::array_destroy(&seen_assets);

		w.indent = 1;
		write_indent(&w); write_raw(&w, "],\n");

		write_indent(&w); write_raw(&w, "\"entities\": [\n");
		w.indent = 2;

		for (usize i = 0; i < scene->entities.count; i++) {
			ecs::Entity e = scene->entities.data[i];

			write_indent(&w); write_raw(&w, "{\n");
			w.indent = 3;

			bool has_prev = false;

			if (ecs::store_has(&world->transforms, e)) {
				const ecs::Transform* t = ecs::store_get(
					const_cast<ecs::Store<ecs::Transform>*>(&world->transforms), e);
				write_indent(&w); write_raw(&w, "\"transform\": {\n");
				w.indent = 4;
				write_indent(&w); write_raw(&w, "\"position\": "); write_vec3(&w, t->position); write_raw(&w, ",\n");
				write_indent(&w); write_raw(&w, "\"rotation\": "); write_vec3(&w, t->rotation); write_raw(&w, ",\n");
				write_indent(&w); write_raw(&w, "\"scale\": ");    write_vec3(&w, t->scale);    write_raw(&w, "\n");
				w.indent = 3;
				write_indent(&w); write_raw(&w, "}");
				has_prev = true;
			}

			if (ecs::store_has(&world->mesh_instances, e)) {
				const ecs::MeshInstance* mi = ecs::store_get(
					const_cast<ecs::Store<ecs::MeshInstance>*>(&world->mesh_instances), e);
				asset::Asset* a = asset::get(mi->asset_id);
				if (a) {
					if (has_prev) write_raw(&w, ",");
					write_raw(&w, "\n");
					write_indent(&w); write_raw(&w, "\"mesh_instance\": { \"asset\": \"");
					write_raw(&w, a->name);
					write_raw(&w, "\" }");
					has_prev = true;
				}
			}

			if (ecs::store_has(&world->hierarchy, e)) {
				const ecs::HierarchyNode* hn = ecs::store_get(
					const_cast<ecs::Store<ecs::HierarchyNode>*>(&world->hierarchy), e);
				if (hn->parent != ecs::INVALID_ENTITY) {
					i32 parent_idx = entity_to_scene_index(scene, hn->parent);
					if (parent_idx >= 0) {
						if (has_prev) write_raw(&w, ",");
						write_raw(&w, "\n");
						write_indent(&w); write_raw(&w, "\"parent\": ");
						char tmp[16];
						str::format(tmp, sizeof(tmp), "%d", parent_idx);
						write_raw(&w, tmp);
						has_prev = true;
					}
				}
			}

			write_raw(&w, "\n");
			w.indent = 2;
			write_indent(&w); write_raw(&w, "}");
			if (i + 1 < scene->entities.count) write_raw(&w, ",");
			write_raw(&w, "\n");
		}

		w.indent = 1;
		write_indent(&w); write_raw(&w, "]\n");

		write_raw(&w, "}\n");
		arr::array_push(&w.buf, '\0');

		bool ok = file::write_file(scene->path, w.buf.data, w.buf.count - 1);
		arr::array_destroy(&w.buf);

		if (ok) logger::info("scene: saved '%s'", scene->path);
		return ok;
	}

	void unload(Scene* scene, ecs::World* world) {
		for (usize i = 0; i < scene->entities.count; i++) {
			ecs::Entity e = scene->entities.data[i];
			ecs::store_remove(&world->hierarchy, e);
			ecs::store_remove(&world->transforms, e);
			ecs::store_remove(&world->mesh_instances, e);
			ecs::pool_release(&world->pool, e);
		}
		arr::array_destroy(&scene->entities);
		logger::info("scene: unloaded '%s'", scene->name);
	}

}
