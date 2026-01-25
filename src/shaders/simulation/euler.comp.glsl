#version 460
#include "gravity.lib.glsl"

// https://en.wikipedia.org/wiki/Semi-implicit_Euler_method#The_method
layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
    uint i = gl_GlobalInvocationID.x;
    v[i] += gravity(i, r[i]) * dt * mov[i];
    r[i] += v[i] * dt * mov[i];
}
