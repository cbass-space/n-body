#version 460

layout (std430, set = 0, binding = 0) buffer Positions { vec2 r[]; };
layout (std430, set = 0, binding = 1) buffer Velocities { vec2 v[]; };
layout (std430, set = 0, binding = 2) readonly buffer Masses { float m[]; };
layout (std430, set = 0, binding = 3) readonly buffer Movable { float mov[]; };

layout (std140, set = 2, binding = 0) uniform Constants {
    uint count;
    float G;
    float ee;
    float dt;
};

#include "gravity.lib.glsl"

// https://en.wikipedia.org/wiki/Semi-implicit_Euler_method#The_method
layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
    uint i = gl_GlobalInvocationID.x;
    v[i] += gravity(i, r[i]) * dt * mov[i];
    r[i] += v[i] * dt * mov[i];
}
