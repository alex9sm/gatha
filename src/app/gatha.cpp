#include "gatha.hpp"
#include "camera.hpp"
#include "../core/types.hpp"
#include "../core/math.hpp"
#include "../core/memory.hpp"
#include "../core/string.hpp"
#include "../core/log.hpp"
#include "../renderer/renderer.hpp"
#include "../renderer/opengl/shader.hpp"
#include "../renderer/opengl/mesh.hpp"
#include "../renderer/opengl/texture.hpp"
#include "../platform/platform.hpp"
#include "../asset/asset.hpp"
#include "../ecs/world.hpp"
#include "../core/file.hpp"
#include "../scene/scene.hpp"

namespace {
	opengl::GLuint shader_program;
	opengl::GLint  vp_loc;
	opengl::GLint  offset_loc;
	opengl::GLint  albedo_loc;
	opengl::GLuint fallback_texture;
	Camera         cam;

	ecs::World     world;
	scene::Scene   current_scene;

	// SSBO for per-instance model matrices
	opengl::GLuint transform_ssbo;
	constexpr u32  MAX_INSTANCES = 16384;

	// Per-frame batch data
	struct DrawBatch {
		u32 asset_id;
		u32 offset;
		u32 count;
	};

	arr::Array<file::FileEntry> asset_file_entries = {};
}

static void on_menu(int action) {
	if (action == platform::MENU_FILE_SAVE) {
		if (current_scene.path[0]) {
			scene::save(&current_scene, &world);
		}
	} else if (action == platform::MENU_FILE_SAVE_AS) {
		char path[256];
		if (platform::editor_save_file_dialog(path, sizeof(path))) {
			str::copy(current_scene.path, path, sizeof(current_scene.path));
			scene::save(&current_scene, &world);
		}
	} else if (action == platform::MENU_FILE_LOAD) {
		char path[256];
		if (platform::editor_open_file_dialog(path, sizeof(path))) {
			scene::unload(&current_scene, &world);
			asset::shutdown();
			scene::load(&current_scene, path, &world);
		}
	}
}

static mat4 transform_to_mat4(const ecs::Transform& t) {
	mat4 m = mat4_translate(t.position);
	m = m * mat4_rotate(t.rotation.x, { 0, 1, 0 });
	m = m * mat4_rotate(t.rotation.y, { 1, 0, 0 });
	m = m * mat4_rotate(t.rotation.z, { 0, 0, 1 });
	m = m * mat4_scale(t.scale);
	return m;
}

bool init() {
	platform::editor_init();
	platform::editor_set_menu_callback(on_menu);

	u32 w, h;
	platform::get_paint_field_size(&w, &h);
	renderer::init(platform::get_native_window_handle(), w, h);
	opengl::shader_load("shaders");
	shader_program = opengl::shader_get("shader");
	vp_loc = opengl::glGetUniformLocation(shader_program, "u_vp");
	offset_loc = opengl::glGetUniformLocation(shader_program, "u_instance_offset");
	albedo_loc = opengl::glGetUniformLocation(shader_program, "u_albedo");
	fallback_texture = opengl::texture_create_solid(255, 0, 255, 255);

	// Create SSBO with GL_DYNAMIC_STORAGE_BIT so we can update it each frame
	opengl::glCreateBuffers(1, &transform_ssbo);
	opengl::glNamedBufferStorage(transform_ssbo,
		MAX_INSTANCES * sizeof(mat4), nullptr,
		opengl::GL_DYNAMIC_STORAGE_BIT);

	ecs::world_init(&world);

	camera_init(&cam, { 0.0f, 1.0f, 5.0f }, 5.0f, 0.002f);

	file::scan_directory("assets", &asset_file_entries);
	platform::editor_set_asset_entries(&asset_file_entries);

	return true;
}

void update() {
	f32 dt = platform::get_delta_time();

	static f32 f5_cooldown = 0.0f;
	f5_cooldown -= dt;
	if (platform::is_key_down(platform::KEY_F5) && f5_cooldown <= 0.0f) {
		platform::editor_toggle();
		if (!platform::is_editor_mode()) {
			platform::set_mouse_captured(true);
		} else {
			platform::set_mouse_captured(false);
		}
		f5_cooldown = 0.3f;
	}

	static f32 esc_cooldown = 0.0f;
	esc_cooldown -= dt;
	if (platform::is_key_down(platform::KEY_ESCAPE) && esc_cooldown <= 0.0f) {
		if (platform::is_mouse_captured()) {
			platform::set_mouse_captured(false);
		}
		esc_cooldown = 0.3f;
	}

	if (platform::is_editor_mode()) {
		static f32 fps_accum = 0.0f;
		static u32 fps_frames = 0;
		fps_accum += dt;
		fps_frames++;
		if (fps_accum >= 0.25f) {
			f32 avg_dt = fps_accum / (f32)fps_frames;
			platform::editor_set_fps(1.0f / avg_dt, avg_dt * 1000.0f);
			fps_accum = 0.0f;
			fps_frames = 0;
		}
	}

	if (platform::is_mouse_captured()) {
		camera_update(&cam, dt);
	}

	for (usize i = 0; i < world.transforms.data.count; i++) {
		world.transforms.data.data[i].local_to_world = transform_to_mat4(world.transforms.data.data[i]);
	}
}

void render() {
	u32 w, h;
	platform::get_paint_field_size(&w, &h);
	renderer::on_resize(w, h);
	renderer::begin_frame();
	f32 aspect = (h > 0) ? (f32)w / (f32)h : 1.0f;
	mat4 view = camera_get_view(&cam);
	mat4 proj = mat4_perspective(to_radians(60.0f), aspect, 0.1f, 1000.0f);
	mat4 vp = proj * view;

	Frustum frustum = frustum_from_vp(vp);

	u32 instance_count = (u32)world.mesh_instances.data.count;
	if (instance_count == 0) { renderer::end_frame(); return; }
	if (instance_count > MAX_INSTANCES) instance_count = MAX_INSTANCES;

	arr::Array<DrawBatch> batches = {};
	mat4* matrices = (mat4*)memory::malloc(instance_count * sizeof(mat4));

	// Pass 1: cull and count visible instances per asset
	for (usize i = 0; i < instance_count; i++) {
		ecs::Entity e = world.mesh_instances.entities.data[i];
		u32 aid = world.mesh_instances.data.data[i].asset_id;

		ecs::Transform* t = ecs::store_get(&world.transforms, e);
		mat4 model = t ? t->local_to_world : mat4_identity();

		asset::Asset* a = asset::get(aid);
		if (!a) continue;

		AABB world_bounds = aabb_transform(a->bounds, model);
		if (!frustum_test_aabb(frustum, world_bounds)) continue;

		DrawBatch* batch = nullptr;
		for (usize b = 0; b < batches.count; b++) {
			if (batches.data[b].asset_id == aid) { batch = &batches.data[b]; break; }
		}
		if (!batch) {
			arr::array_push(&batches, DrawBatch{ aid, 0, 0 });
			batch = &batches.data[batches.count - 1];
		}
		batch->count++;
	}

	// Assign offsets
	u32 running_offset = 0;
	for (usize b = 0; b < batches.count; b++) {
		batches.data[b].offset = running_offset;
		running_offset += batches.data[b].count;
	}

	// Pass 2: fill matrices for visible instances
	u32* fill_counts = (u32*)memory::malloc(batches.count * sizeof(u32));
	memory::set(fill_counts, 0, batches.count * sizeof(u32));

	for (usize i = 0; i < instance_count; i++) {
		ecs::Entity e = world.mesh_instances.entities.data[i];
		u32 aid = world.mesh_instances.data.data[i].asset_id;

		ecs::Transform* t = ecs::store_get(&world.transforms, e);
		mat4 model = t ? t->local_to_world : mat4_identity();

		asset::Asset* a = asset::get(aid);
		if (!a) continue;

		AABB world_bounds = aabb_transform(a->bounds, model);
		if (!frustum_test_aabb(frustum, world_bounds)) continue;

		usize batch_idx = 0;
		for (usize b = 0; b < batches.count; b++) {
			if (batches.data[b].asset_id == aid) { batch_idx = b; break; }
		}

		u32 dst = batches.data[batch_idx].offset + fill_counts[batch_idx];
		matrices[dst] = model;
		fill_counts[batch_idx]++;
	}

	memory::free(fill_counts);

	// Upload all transforms in one call
	opengl::glNamedBufferSubData(transform_ssbo, 0,
		running_offset * sizeof(mat4), matrices);
	memory::free(matrices);

	// Bind SSBO and shader, set VP matrix once
	opengl::glBindBufferBase(opengl::GL_SHADER_STORAGE_BUFFER, 0, transform_ssbo);
	opengl::glUseProgram(shader_program);
	opengl::glUniformMatrix4fv(vp_loc, 1, opengl::GL_FALSE, &vp.col[0][0]);
	opengl::glUniform1i(albedo_loc, 0);

	for (usize b = 0; b < batches.count; b++) {
		asset::Asset* a = asset::get(batches.data[b].asset_id);
		if (!a) continue;

		opengl::GLuint tex = a->texture ? a->texture : fallback_texture;
		opengl::glBindTextureUnit(0, tex);
		opengl::glUniform1ui(offset_loc, batches.data[b].offset);
		opengl::glBindVertexArray(a->mesh.vao);
		opengl::glDrawElementsInstanced(opengl::GL_TRIANGLES,
			a->mesh.index_count, opengl::GL_UNSIGNED_INT,
			nullptr, batches.data[b].count);
	}

	arr::array_destroy(&batches);
	renderer::end_frame();
}

void shutdown() {
	platform::set_mouse_captured(false);
	platform::editor_set_asset_entries(nullptr);
	arr::array_destroy(&asset_file_entries);
	scene::unload(&current_scene, &world);
	ecs::world_destroy(&world);
	opengl::glDeleteBuffers(1, &transform_ssbo);
	opengl::texture_destroy(fallback_texture);
	asset::shutdown();
	opengl::shader_unload();
}
