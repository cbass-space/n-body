#version 460

layout (location = 0) out vec4 out_color;

const uint PREDICTION_LENGTH = 2048;
layout (std430, set = 0, binding = 0) readonly buffer Positions { vec2 _padding1; vec2 positions[PREDICTION_LENGTH]; };

layout (std140, set = 1, binding = 0) uniform Camera {
    mat4 orthographic;
    mat4 view;
};

layout (std140, set = 1, binding = 1) uniform Constants {
    vec3 _padding2;
    uint target;
    float brightness;
};

layout (std140, set = 1, binding = 2) uniform Ghost { vec4 color; };

void main() {
    vec2 position = positions[gl_VertexIndex];
    if (target != uint(-1)) {
        position += positions[0]
            - positions[gl_VertexIndex];
    }

    gl_Position = orthographic * view * vec4(position, 0.0, 1.0);

    float alpha = (brightness / 2.0) * (1.0 - float(gl_VertexIndex) / float(PREDICTION_LENGTH));
    out_color = vec4(color.rgb, alpha);
}

