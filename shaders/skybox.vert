#version 450

layout(location = 0) out vec3 f_pos;

layout(location = 0) in vec3 in_pos;

layout(set = 0, binding = 0) uniform VP {
    mat4 projection;
    mat4 view;
} u_vp;

void main() {
    f_pos = in_pos;
    f_pos.xy *= -1.0;

    gl_Position = u_vp.projection * vec4(mat3(u_vp.view) * in_pos, 1.0);
}
