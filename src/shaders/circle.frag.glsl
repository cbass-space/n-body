#version 460

struct VertexOut {
    vec3 color;
    vec2 position;
    float outline;
};

layout (location = 0) in VertexOut frag;
layout (location = 0) out vec4 out_color;

void main() {
    float distance_squared = pow(frag.position.x, 2.0) + pow(frag.position.y, 2.0);
    float draw = float(distance_squared < 1.0 && distance_squared > (1.0 - frag.outline));
    out_color = vec4(frag.color, draw);
}
