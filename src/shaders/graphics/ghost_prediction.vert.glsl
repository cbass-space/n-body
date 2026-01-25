#version 460

layout (location = 0) out vec4 out_color;

const uint PREDICTION_LENGTH = 2048;
layout (std430, set = 0, binding = 0) readonly buffer PositionStorage { vec2 positions[][PREDICTION_LENGTH]; };
layout (std430, set = 0, binding = 1) readonly buffer GhostStorage { vec2 ghost_positions[PREDICTION_LENGTH]; };

layout (std140, set = 1, binding = 0) uniform TransformUniform {
    mat4 orthographic;
    mat4 view;
};

layout (std140, set = 1, binding = 1) uniform ConstantsUniform {
    vec4 _padding1;
    float brightness;
    uint target;
};

layout(std140, set = 1, binding = 2) uniform GhostUniform {
    vec4 color;
    vec4 _padding2;
};

void main() {
    vec2 position = ghost_positions[gl_VertexIndex];
    if (target != uint(-1)) {
        position += positions[target][0]
            - positions[target][gl_VertexIndex];
    }

    gl_Position = orthographic * view * vec4(position, 0.0, 1.0);

    float alpha = (brightness / 3.0) * (1.0 - float(gl_VertexIndex) / float(PREDICTION_LENGTH));
    out_color = vec4(color.rgb, alpha);
}

