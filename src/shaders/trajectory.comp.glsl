#version 460
#include "lib/conditionals.lib.glsl"

const uint PREDICTION_LENGTH = 2048;
layout (std430, set = 0, binding = 0) buffer TrajectoryPositions { vec2 r[][PREDICTION_LENGTH]; };
layout (std430, set = 0, binding = 1) buffer TrajectoryVelocities { vec2 v[][PREDICTION_LENGTH]; };
layout (std430, set = 0, binding = 2) readonly buffer Positions { vec2 r_0[]; };
layout (std430, set = 0, binding = 3) readonly buffer Velocities { vec2 v_0[]; };
layout (std430, set = 0, binding = 4) readonly buffer Masses { float m[]; };
layout (std430, set = 0, binding = 5) readonly buffer Movable { float mov[]; };

layout (std140, set = 2, binding = 0) uniform Constants {
    uint count;
    float G;
    float ee;
    float dt;
};

layout (std140, set = 2, binding = 1) uniform Frame { uint frame; };

vec2 gravity(uint self) {
    vec2 net_a = vec2(0.0);
    for (uint i = 0; i < count; i++) {
        vec2 R = r[i][frame - 1] - r[self][frame - 1];
        float R2 = dot(R, R) + ee * ee;
        net_a += (G * m[i] / R2) * normalize(R) * when_neq(i, self);
    }

    return net_a;
}

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
    uint body = gl_GlobalInvocationID.x;

    // TODO: add mov[i] == 0.0 to frame == 0
    v[body][frame] = (frame == 0)
        ? v_0[body]
        : v[body][frame - 1] + gravity(body) * dt;
    r[body][frame] = (frame == 0)
        ? r_0[body]
        : r[body][frame - 1] + v[body][frame] * dt;
}
