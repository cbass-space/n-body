#version 460

layout (std430, set = 0, binding = 0) buffer Positions { vec2 r[]; };
layout (std430, set = 0, binding = 1) buffer Velocities { vec2 v[]; };
layout (std430, set = 0, binding = 2) readonly buffer Masses { float m[]; };
layout (std430, set = 0, binding = 3) readonly buffer Movable { float mov[]; };

layout (std140, set = 2, binding = 0) uniform Constants {
    uint body_count;
    float G;
    float ee;
    float dt;
};

uint when_neq(uint a, uint b) { return uint(a != b); }
vec2 gravity(uint self, vec2 r_self) {
    vec2 net_a = vec2(0.0);
    for (uint i = 0; i < body_count; i++) {
        vec2 R = r[i] - r_self;
        float R2 = dot(R, R) + ee * ee;
        net_a += (G * m[i] / R2) * normalize(R) * when_neq(i, self);
    }

    return net_a;
}

// https://en.wikipedia.org/wiki/Semi-implicit_Euler_method#The_method
layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
    uint i = gl_GlobalInvocationID.x;
    v[i] += gravity(i, r[i]) * dt * mov[i];
    r[i] += v[i] * dt * mov[i];
}
