#pragma once

#include "../core/math.hpp"

struct Camera {
	vec3 position;
	f32 yaw;
	f32 pitch;
	f32 speed;
	f32 sensitivity;
};

void camera_init(Camera* cam, vec3 pos, f32 speed, f32 sensitivity);
void camera_update(Camera* cam, f32 dt);
mat4 camera_get_view(const Camera* cam);