#version 460
#include "gravity.lib.glsl"

// https://en.wikipedia.org/wiki/Verlet_integration#Velocity_Verlet
layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
    uint i = gl_GlobalInvocationID.x;
    vec2 a = gravity(i, r[i]);
    r[i] += v[i] * dt + a * (dt * dt) / 2;

    vec2 a_next = gravity(i, r[i]);
    v[i] += (a + a_next) * (dt / 2);
}
