#version 450 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_uv;
layout(location = 3) in vec4 a_tangent;

layout(std430, binding = 0) buffer TransformBuffer {
	mat4 models[];
};

uniform mat4 u_vp;
uniform uint u_instance_offset;

out vec2 v_uv;

void main() {
	mat4 model = models[u_instance_offset + gl_InstanceID];
	gl_Position = u_vp * model * vec4(a_position, 1.0);
	v_uv = a_uv;
}
