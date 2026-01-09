#version 460

layout (location = 0) out vec4 out_color;

const uint TRAIL_LENGTH = 256;
layout (std430, set = 0, binding = 0) readonly buffer PositionStorage { vec2 positions[][TRAIL_LENGTH]; };
layout (std430, set = 0, binding = 1) readonly buffer ColorStorage { vec4 colors[]; };

layout (std140, set = 1, binding = 0) uniform TransformUniform {
    mat4 orthographic;
    mat4 view;
};

layout (std140, set = 1, binding = 1) uniform ConstantsUniform {
    vec3 _padding;
    uint counter;
    float brightness;
    uint target;
};

void main() {
    vec2 position = positions[gl_InstanceIndex][(counter - gl_VertexIndex) % TRAIL_LENGTH];
    if (target != uint(-1)) {
        position += positions[target][counter]
            - positions[target][(counter - gl_VertexIndex) % TRAIL_LENGTH];
    }

    gl_Position = orthographic * view * vec4(position, 0.0, 1.0);

    float alpha = brightness * (1.0 - float(gl_VertexIndex) / float(TRAIL_LENGTH));
    out_color = vec4(colors[gl_InstanceIndex].rgb, alpha);
}
