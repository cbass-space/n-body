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

struct State {
    vec2 r;
    vec2 v;
};

State add(State a, State b) { return State(a.r + b.r, a.v + b.v); }
State scale(State y, float a) { return State(a * y.r, a * y.v); }
State f(State y, uint i) { return State(y.v, gravity(i, y.r)); }

// https://en.wikipedia.org/wiki/Runge–Kutta_methods
layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
    uint i = gl_GlobalInvocationID.x;
    State y = State(r[i], v[i]);
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
    r[i] = y_next.r;
    v[i] = y_next.v;
}

