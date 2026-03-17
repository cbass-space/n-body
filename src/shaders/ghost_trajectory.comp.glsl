#version 460

const uint PREDICTION_LENGTH = 2048;
layout (std430, set = 0, binding = 0) buffer TrajectoryPositions { vec2 r[][PREDICTION_LENGTH]; };
layout (std430, set = 0, binding = 1) buffer TrajectoryVelocities { vec2 v[]; };
layout (std430, set = 0, binding = 2) buffer TrajectoryGhost { vec2 v_g; vec2 r_g[PREDICTION_LENGTH]; };
layout (std430, set = 0, binding = 3) readonly buffer Positions { vec2 r_0[]; };
layout (std430, set = 0, binding = 4) readonly buffer Velocities { vec2 v_0[]; };
layout (std430, set = 0, binding = 5) readonly buffer Masses { float m[]; };
layout (std430, set = 0, binding = 6) readonly buffer Movable { float mov[]; };

const uint EULER = 0;
const uint VERLET = 1;
const uint RK4 = 2;

layout (std140, set = 2, binding = 0) uniform Constants {
    uint body_count;
    uint integrator;
    float G;
    float ee;
    float dt;
};

layout (std140, set = 2, binding = 1) uniform Ghost {
    vec2 r_g0;
    vec2 v_g0;
    float m_g;
    bool ghost;
};

layout (std140, set = 2, binding = 2) uniform Frame { uint frame; };

vec2 gravity(vec2 r_self, uint frame) {
    vec2 net_a = vec2(0.0);
    for (uint i = 0; i < body_count; i++) {
        vec2 R = r[i][frame] - r_self;
        float R2 = dot(R, R) + ee * ee;
        net_a += (G * m[i] / R2) * normalize(R);
    }

    return net_a;
}

struct State {
    vec2 r;
    vec2 v;
};

State add(State a, State b) { return State(a.r + b.r, a.v + b.v); }
State scale(State y, float a) { return State(a * y.r, a * y.v); }
State f(State y) { return State(y.v, gravity(y.r, frame - 1)); }

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
    if (frame == 0) {
        r_g[frame] = r_g0;
        v_g = v_g0;
    } else {
        switch (integrator) {
            case EULER:
                v_g += gravity(r_g[frame - 1], frame - 1) * dt;
                r_g[frame] = r_g[frame - 1] + v_g * dt;
                break;

            case VERLET:
                vec2 a = gravity(r_g[frame - 1], frame - 1);
                r_g[frame] = r_g[frame - 1] + v_g * dt + a * (dt * dt) / 2;
                vec2 a_next = gravity(r_g[frame], frame - 1);
                v_g += (a + a_next) * (dt / 2);
                break;

            case RK4:
                State y = State(r_g[frame - 1], v_g);
                State k_1 = f(y);
                State k_2 = f(add(y, scale(k_1, dt / 2)));
                State k_3 = f(add(y, scale(k_2, dt / 2)));
                State k_4 = f(add(y, scale(k_3, dt)));

                State k_sum = add(
                    k_1, add(
                        scale(k_2, 2),
                        add(scale(k_3, 2), k_4)
                    )
                );

                State y_next = add(y, scale(k_sum, dt / 6));
                r_g[frame] = y_next.r;
                v_g = y_next.v;
                break;
        }
    }
}
