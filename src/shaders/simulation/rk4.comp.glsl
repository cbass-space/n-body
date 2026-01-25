#version 460
#include "gravity.lib.glsl"

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
