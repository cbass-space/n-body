#version 460

struct VertexOut {
    vec4 color;
    vec2 position;
    float outline;
};

layout (location = 0) out VertexOut frag;

layout (std140, set = 1, binding = 0) uniform Camera {
    mat4 orthographic;
    mat4 view;
};

layout (std140, set = 1, binding = 1) uniform Constants {
    float density;
    float movable_outline;
    float static_outline;
};

layout (std140, set = 1, binding = 2) uniform Ghost {
    vec4 color;
    vec2 position;
    float mass;
    float movable;
};

float compute_radius(float mass) { return pow(mass / density, 1.0 / 3.0); }
void main() {
    frag.color = color;
    frag.position.x = 2.0 * floor(gl_VertexIndex / 2.0) - 1.0;
    frag.position.y = 2.0 * mod(gl_VertexIndex, 2.0) - 1.0;
    frag.outline = movable == 1.0 ? movable_outline : static_outline;

    float radius = compute_radius(mass);
    gl_Position = orthographic * view * vec4(radius * frag.position + position, 0.0, 1.0);
}

