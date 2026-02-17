#pragma once

#include "types.hpp"
#include <math.h>

struct vec2 { f32 x, y; };
struct vec3 { f32 x, y, z; };
struct vec4 { f32 x, y, z, w; };

struct mat4 { f32 col[4][4]; };

inline vec2 operator+(vec2 a, vec2 b) { return { a.x + b.x, a.y + b.y }; }
inline vec2 operator-(vec2 a, vec2 b) { return { a.x - b.x, a.y - b.y }; }
inline vec2 operator*(vec2 v, f32 s) { return { v.x * s,   v.y * s }; }
inline vec2 operator*(f32 s, vec2 v) { return v * s; }
inline vec2& operator+=(vec2& a, vec2 b) { a.x += b.x; a.y += b.y; return a; }

inline f32  dot(vec2 a, vec2 b) { return a.x * b.x + a.y * b.y; }
inline f32  length_sq(vec2 v) { return dot(v, v); }
inline f32  length(vec2 v) { return sqrtf(length_sq(v)); }
inline vec2 normalize(vec2 v) { f32 l = length(v); return { v.x / l, v.y / l }; }

inline vec3 operator+(vec3 a, vec3 b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
inline vec3 operator-(vec3 a, vec3 b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
inline vec3 operator-(vec3 v) { return { -v.x, -v.y, -v.z }; }
inline vec3 operator*(vec3 v, f32 s) { return { v.x * s, v.y * s, v.z * s }; }
inline vec3 operator*(f32 s, vec3 v) { return v * s; }
inline vec3& operator+=(vec3& a, vec3 b) { a.x += b.x; a.y += b.y; a.z += b.z; return a; }

inline f32  dot(vec3 a, vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline f32  length_sq(vec3 v) { return dot(v, v); }
inline f32  length(vec3 v) { return sqrtf(length_sq(v)); }
inline vec3 normalize(vec3 v) { f32 l = length(v); return { v.x / l, v.y / l, v.z / l }; }

inline vec3 cross(vec3 a, vec3 b) {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

inline vec4 operator+(vec4 a, vec4 b) { return { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w }; }
inline vec4 operator*(vec4 v, f32 s) { return { v.x * s, v.y * s, v.z * s, v.w * s }; }
inline f32  dot(vec4 a, vec4 b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }

inline mat4 mat4_identity() {
    mat4 m = {};
    m.col[0][0] = 1.0f;
    m.col[1][1] = 1.0f;
    m.col[2][2] = 1.0f;
    m.col[3][3] = 1.0f;
    return m;
}

inline mat4 mat4_mul(mat4 a, mat4 b) {
    mat4 r = {};
    for (int c = 0; c < 4; c++) {
        for (int row = 0; row < 4; row++) {
            r.col[c][row] =
                a.col[0][row] * b.col[c][0] +
                a.col[1][row] * b.col[c][1] +
                a.col[2][row] * b.col[c][2] +
                a.col[3][row] * b.col[c][3];
        }
    }
    return r;
}

inline mat4 operator*(mat4 a, mat4 b) { return mat4_mul(a, b); }

inline mat4 mat4_translate(vec3 t) {
    mat4 m = mat4_identity();
    m.col[3][0] = t.x;
    m.col[3][1] = t.y;
    m.col[3][2] = t.z;
    return m;
}

inline mat4 mat4_scale(vec3 s) {
    mat4 m = mat4_identity();
    m.col[0][0] = s.x;
    m.col[1][1] = s.y;
    m.col[2][2] = s.z;
    return m;
}

inline mat4 mat4_scale(f32 s) {
    return mat4_scale({ s, s, s });
}

inline mat4 mat4_rotate(f32 angle, vec3 axis) {
    f32 c = cosf(angle);
    f32 s = sinf(angle);
    f32 t = 1.0f - c;

    mat4 m = {};
    m.col[0][0] = t * axis.x * axis.x + c;
    m.col[0][1] = t * axis.x * axis.y + s * axis.z;
    m.col[0][2] = t * axis.x * axis.z - s * axis.y;

    m.col[1][0] = t * axis.x * axis.y - s * axis.z;
    m.col[1][1] = t * axis.y * axis.y + c;
    m.col[1][2] = t * axis.y * axis.z + s * axis.x;

    m.col[2][0] = t * axis.x * axis.z + s * axis.y;
    m.col[2][1] = t * axis.y * axis.z - s * axis.x;
    m.col[2][2] = t * axis.z * axis.z + c;

    m.col[3][3] = 1.0f;
    return m;
}

inline mat4 mat4_look_at(vec3 eye, vec3 center, vec3 up) {
    vec3 f = normalize(center - eye);
    vec3 r = normalize(cross(f, up));
    vec3 u = cross(r, f);

    mat4 m = {};
    m.col[0][0] = r.x;  m.col[1][0] = r.y;  m.col[2][0] = r.z;  m.col[3][0] = -dot(r, eye);
    m.col[0][1] = u.x;  m.col[1][1] = u.y;  m.col[2][1] = u.z;  m.col[3][1] = -dot(u, eye);
    m.col[0][2] = -f.x;  m.col[1][2] = -f.y;  m.col[2][2] = -f.z;  m.col[3][2] = dot(f, eye);
    m.col[3][3] = 1.0f;
    return m;
}

inline mat4 mat4_perspective(f32 fov_y, f32 aspect, f32 z_near, f32 z_far) {
    f32 f = 1.0f / tanf(fov_y * 0.5f);
    mat4 m = {};
    m.col[0][0] = f / aspect;
    m.col[1][1] = f;
    m.col[2][2] = (z_far + z_near) / (z_near - z_far);
    m.col[2][3] = -1.0f;
    m.col[3][2] = (2.0f * z_far * z_near) / (z_near - z_far);
    return m;
}

constexpr f32 PI = 3.14159265358979323846f;
constexpr f32 TAU = 6.28318530717958647692f;

inline f32 to_radians(f32 degrees) { return degrees * (PI / 180.0f); }
inline f32 to_degrees(f32 radians) { return radians * (180.0f / PI); }