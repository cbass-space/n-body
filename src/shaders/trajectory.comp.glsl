#version 460
#include "lib/conditionals.lib.glsl"

const uint PREDICTION_LENGTH = 2048;
layout (std430, set = 0, binding = 0) buffer TrajectoryPositions { vec2 r[][PREDICTION_LENGTH]; };
layout (std430, set = 0, binding = 1) buffer TrajectoryVelocities { vec2 v[]; };
layout (std430, set = 0, binding = 2) readonly buffer Positions { vec2 r_0[]; };
layout (std430, set = 0, binding = 3) readonly buffer Velocities { vec2 v_0[]; };
layout (std430, set = 0, binding = 4) readonly buffer Masses { float m[]; };
layout (std430, set = 0, binding = 5) readonly buffer Movable { float mov[]; };

const uint EULER = 0;
const uint VERLET = 1;
const uint RK4 = 2;

layout (std140, set = 2, binding = 0) uniform Constants {
    uint count;
    uint integrator;
    float G;
    float ee;
    float dt;
};

layout (std140, set = 2, binding = 1) uniform Frame { uint frame; };

vec2 gravity(uint self, vec2 r_self, uint frame) {
    vec2 net_a = vec2(0.0);
    for (uint i = 0; i < count; i++) {
        vec2 R = r[i][frame] - r_self;
        float R2 = dot(R, R) + ee * ee;
        net_a += (G * m[i] / R2) * normalize(R) * when_neq(i, self);
    }

    return net_a;
}

struct State {
    vec2 r;
    vec2 v;
};

State add(State a, State b) { return State(a.r + b.r, a.v + b.v); }
State scale(State y, float a) { return State(a * y.r, a * y.v); }
State f(State y, uint i) { return State(y.v, gravity(i, y.r, frame - 1)); }

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
    uint i = gl_GlobalInvocationID.x;

    // we're branching on uniform constants, so we should be ok?
    if (frame == 0) {
        v[i] = v_0[i];
        r[i][frame] = r_0[i];
    } else {
        switch (integrator) {
            case EULER:
                v[i] += gravity(i, r[i][frame - 1], frame - 1) * dt;
                r[i][frame] = r[i][frame - 1] + v[i] * dt;
                break;
            case VERLET:
                vec2 a = gravity(i, r[i][frame - 1], frame - 1);
                r[i][frame] = r[i][frame - 1] + v[i] * dt + a * (dt * dt) / 2;
                vec2 a_next = gravity(i, r[i][frame], frame);
                v[i] += (a + a_next) * (dt / 2);
                break;
            case RK4:
                State y = State(r[i][frame - 1], v[i]);
                State k_1 = f(y, i);
                State k_2 = f(add(y, scale(k_1, dt / 2)), i);
                State k_3 = f(add(y, scale(k_2, dt / 2)), i);
                State k_4 = f(add(y, scale(k_3, dt)), i);

                State k_sum = add(
                    k_1, add(
                        scale(k_2, 2),
                        add(scale(k_3, 2), k_4)
                    )
                );

                State y_next = add(y, scale(k_sum, dt / 6));
                r[i][frame] = y_next.r;
                v[i] = y_next.v;
                break;
        }
    }
}
