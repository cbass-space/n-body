#include "../lib/conditionals.lib.glsl"

vec2 gravity(uint self, vec2 r_self) {
    vec2 net_a = vec2(0.0);
    for (uint i = 0; i < count; i++) {
        vec2 R = r[i] - r_self;
        float R2 = dot(R, R) + ee * ee;
        net_a += (G * m[i] / R2) * normalize(R) * when_neq(i, self);
    }

    return net_a;
}
