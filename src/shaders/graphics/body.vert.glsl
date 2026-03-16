#version 460

struct VertexOut {
    vec4 color;
    vec2 position;
    float outline;
};

layout (location = 0) out VertexOut frag;

layout (std430, set = 0, binding = 0) readonly buffer Positions { vec2 positions[]; };
layout (std430, set = 0, binding = 1) readonly buffer Colors { vec4 colors[]; };
layout (std430, set = 0, binding = 2) readonly buffer Masses { float masses[]; };
layout (std430, set = 0, binding = 3) readonly buffer Movables { float movable[]; };

layout (std140, set = 1, binding = 0) uniform Camera {
    mat4 orthographic;
    mat4 view;
};

layout (std140, set = 1, binding = 1) uniform Constants {
    float density;
    float movable_outline;
    float static_outline;
};

float compute_radius(float mass) {
    return pow(mass / density, 1.0 / 3.0);
}

void main() {
    frag.color = colors[gl_InstanceIndex];
    frag.position.x = 2.0 * floor(gl_VertexIndex / 2.0) - 1.0;
    frag.position.y = 2.0 * mod(gl_VertexIndex, 2.0) - 1.0;
    frag.outline = movable[gl_InstanceIndex] == 1.0 ? movable_outline : static_outline;

    float radius = compute_radius(masses[gl_InstanceIndex]);
    vec2 position = positions[gl_InstanceIndex];
    gl_Position = orthographic * view * vec4(radius * frag.position + position, 0.0, 1.0);
}
