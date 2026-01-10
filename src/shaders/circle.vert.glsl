#version 460

struct VertexOut {
    vec4 color;
    vec2 position;
    float outline;
};

layout (location = 0) out VertexOut frag;

const uint TRAIL_LENGTH = 512;
layout (std430, set = 0, binding = 0) readonly buffer PositionStorage { vec2 positions[][TRAIL_LENGTH]; };
layout (std430, set = 0, binding = 1) readonly buffer ColorStorage { vec4 colors[]; };
layout (std430, set = 0, binding = 2) readonly buffer MassStorage { float masses[]; };
layout (std430, set = 0, binding = 3) readonly buffer MovableStorage { float movable[]; };

layout (std140, set = 1, binding = 0) uniform TransformUniform {
    mat4 orthographic;
    mat4 view;
};

layout (std140, set = 1, binding = 1) uniform ConstantsUniform {
    float density;
    float movable_outline;
    float static_outline;
    uint current;
    vec2 _padding;
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
    vec2 position = positions[gl_InstanceIndex][current];
    gl_Position = orthographic * view * vec4(radius * frag.position + position, 0.0, 1.0);
}
