#include "camera.hpp"
#include "../core/math.hpp"
#include "../platform/platform.hpp"

void camera_init(Camera* cam, vec3 pos, f32 speed, f32 sensitivity) {
    cam->position = pos;
    cam->yaw = 0.0f;
    cam->pitch = 0.0f;
    cam->speed = speed;
    cam->sensitivity = sensitivity;
}

void camera_update(Camera* cam, f32 dt) {
    f32 dx, dy;
    platform::get_mouse_delta(&dx, &dy);

    cam->yaw -= dx * cam->sensitivity;
    cam->pitch -= dy * cam->sensitivity;
    if (cam->yaw > TAU) cam->yaw -= TAU;
    if (cam->yaw < 0.0f) cam->yaw += TAU;
    f32 max_pitch = to_radians(89.0f);
    if (cam->pitch > max_pitch) cam->pitch = max_pitch;
    if (cam->pitch < -max_pitch) cam->pitch = -max_pitch;
    vec3 forward = {
        cosf(cam->pitch) * sinf(cam->yaw),
        sinf(cam->pitch),
        cosf(cam->pitch) * cosf(cam->yaw)
    };

    vec3 world_up = { 0.0f, 1.0f, 0.0f };
    vec3 right = normalize(cross(forward, world_up));

    f32 speed = cam->speed * dt;
    if (platform::is_key_down(platform::KEY_W)) cam->position += forward *  speed;
    if (platform::is_key_down(platform::KEY_S)) cam->position += forward * -speed;
    if (platform::is_key_down(platform::KEY_D)) cam->position += right   *  speed;
    if (platform::is_key_down(platform::KEY_A)) cam->position += right   * -speed;
    if (platform::is_key_down(platform::KEY_SPACE)) cam->position.y += speed;
    if (platform::is_key_down(platform::KEY_CTRL))  cam->position.y -= speed;
}

mat4 camera_get_view(const Camera* cam) {
    vec3 forward = {
        cosf(cam->pitch) * sinf(cam->yaw),
        sinf(cam->pitch),
        cosf(cam->pitch) * cosf(cam->yaw)
    };

    vec3 target = cam->position + forward;
    vec3 world_up = { 0.0f, 1.0f, 0.0f };

    return mat4_look_at(cam->position, target, world_up);
}