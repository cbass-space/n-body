#version 460

struct VertexOut {
    vec3 color;
    vec2 position;
    float outline;
};

layout (location = 0) out VertexOut frag;

const uint TRAIL_LENGTH = 256;
layout (std430, set = 0, binding = 0) readonly buffer PositionStorage { vec2 positions[][TRAIL_LENGTH]; };
layout (std430, set = 0, binding = 1) readonly buffer OffsetStorage { uint offsets[]; };
layout (std430, set = 0, binding = 2) readonly buffer ColorStorage { vec4 colors[]; };
layout (std430, set = 0, binding = 3) readonly buffer MassStorage { float masses[]; };
layout (std430, set = 0, binding = 4) readonly buffer MovableStorage { float movable[]; };

layout (std140, set = 1, binding = 0) uniform TransformUniform {
    mat4 orthographic;
    mat4 view;
};
layout (std140, set = 1, binding = 1) uniform CounterUniform { uint counter; };
layout (std140, set = 1, binding = 2) uniform ConstantsUniform {
    float density;
    float movable_outline;
    float static_outline;
    float padding;
};

float compute_radius(float mass) {
    return pow(mass / density, 1.0/3.0);
}

void main() {
    frag.color = colors[gl_InstanceIndex].rgb;
    frag.position.x = 2.0 * floor(gl_VertexIndex / 2.0) - 1.0;
    frag.position.y = 2.0 * mod(gl_VertexIndex, 2.0) - 1.0;
    frag.outline = movable[gl_InstanceIndex] == 0.0 ? static_outline : movable_outline;

    float radius = compute_radius(masses[gl_InstanceIndex]);
    uint current = (counter - offsets[gl_InstanceIndex]) % TRAIL_LENGTH;
    vec2 position = positions[gl_InstanceIndex][current];
    gl_Position = orthographic * view * vec4(radius * frag.position + position, 0.0, 1.0);
}
