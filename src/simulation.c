#include "stb_ds.h"

#include "main.h"

void simulation_init(Simulation *sim) {
    sim->options = (SimulationOptions) {
        .gravity = GRAVITY_DEFAULT,
        .softening = SOFTENING_DEFAULT,
        .density = DENSITY_DEFAULT,
        .integrator = INTEGRATOR_DEFAULT,
        .collisions = COLLISIONS_DEFAULT,
        .barnes_hut = BARNES_HUT_DEFAULT
    };
}

usize simulation_add_body(Simulation *sim, const SimulationAddBodyInfo *body) {
    const usize index = arrlenu(sim->r);
    arrput(sim->r, body->position);
    arrput(sim->v, body->velocity);
    arrput(sim->m, body->mass);
    arrput(sim->movable, body->movable);
    return index;
}

static void integrate_euler(const Simulation *sim, usize i, f32 dt);
static void integrate_verlet(const Simulation *sim, usize i, f32 dt);
static void integrate_rk4(Simulation *sim, usize i, f32 dt);
static void collide_bodies(const Simulation *sim);
void simulation_update(Simulation *sim, const f64 dt) {
    for (usize i = 0; i < arrlenu(sim->r); i++) {
        if (!sim->movable[i]) {
            sim->v[i] = (HMM_Vec2) { .X = 0.0f, .Y = 0.0f };
            continue;
        }

        switch (sim->options.integrator) {
            case INTEGRATOR_EULER:
                integrate_euler(sim, i, (f32) dt);
                break;
            case INTEGRATOR_VERLET:
                integrate_verlet(sim, i, (f32) dt);
                break;
            case INTEGRATOR_RK4:
                integrate_rk4(sim, i, (f32) dt);
                break;
        }
    }

    collide_bodies(sim);
}

static HMM_Vec2 gravitational_acceleration(const Simulation *sim, const HMM_Vec2 position, const usize skip_index) {
    HMM_Vec2 net_acceleration = (HMM_Vec2) { 0 };

    for (usize i = 0; i < arrlenu(sim->r); i++) {
        if (i == skip_index) continue;

        const HMM_Vec2 separation = HMM_SubV2(sim->r[i], position);
        const f32 length_squared = HMM_LenSqrV2(separation) + powf(sim->options.softening, 2.0f);
        if (length_squared < EPSILON) { return (HMM_Vec2) { 0 }; }

        const HMM_Vec2 acceleration = HMM_MulV2F(
            HMM_NormV2(separation),
            (sim->options.gravity * sim->m[i]) / length_squared
        );

        net_acceleration = HMM_AddV2(net_acceleration, acceleration);
    }

    return net_acceleration;
}

// https://gafferongames.com/post/integration_basics/#semi-implicit-euler
static void integrate_euler(const Simulation *sim, const usize i, const f32 dt) {
    const HMM_Vec2 acceleration = gravitational_acceleration(sim, sim->r[i], i);
    sim->v[i] = HMM_AddV2(sim->v[i], HMM_MulV2F(acceleration, dt));
    sim->r[i] = HMM_AddV2(sim->r[i], HMM_MulV2F(sim->v[i], dt));
}

// https://en.wikipedia.org/wiki/Verlet_integration#Velocity_Verlet
static void integrate_verlet(const Simulation *sim, const usize i, const f32 dt) {
    const HMM_Vec2 acceleration = gravitational_acceleration(sim, sim->r[i], i);
    sim->r[i] = HMM_AddV2(sim->r[i], HMM_AddV2(
        HMM_MulV2F(sim->v[i], dt),
        HMM_MulV2F(acceleration, powf(dt, 2.0f) / 2.0f)
    ));

    const HMM_Vec2 new_acceleration = gravitational_acceleration(sim, sim->r[i], i);
    sim->v[i] = HMM_AddV2(sim->v[i],
        HMM_MulV2F(HMM_AddV2(acceleration, new_acceleration),
        dt / 2.0f)
    );
}

typedef struct {
    HMM_Vec2 r;
    HMM_Vec2 v;
} KinematicState;
static KinematicState rk4_add(KinematicState a, KinematicState b);
static KinematicState rk4_scale(KinematicState state, f32 a);
static KinematicState rk4_delta(KinematicState state, const Simulation *sim, usize i);

static void integrate_rk4(Simulation *sim, usize i, f32 dt) {
    KinematicState state = { sim->r[i], sim->v[i] };

    KinematicState k_1 = rk4_delta(state, sim, i);
    KinematicState k_2 = rk4_delta(rk4_add(state, rk4_scale(k_1, dt / 2.0f)), sim, i);
    KinematicState k_3 = rk4_delta(rk4_add(state, rk4_scale(k_2, dt / 2.0f)), sim, i);
    KinematicState k_4 = rk4_delta(rk4_add(state, rk4_scale(k_3, dt)), sim, i);

    KinematicState k_sum = rk4_add(
        k_1, rk4_add(
            rk4_scale(k_2, 2),
            rk4_add(rk4_scale(k_3, 2), k_4)
        )
    );

    KinematicState new_state = rk4_add(state, rk4_scale(k_sum, dt / 6.0f));
    sim->r[i] = new_state.r;
    sim->v[i] = new_state.v;
}

static KinematicState rk4_add(const KinematicState a, const KinematicState b) {
    return (KinematicState) {
        .r = HMM_AddV2(a.r, b.r),
        .v = HMM_AddV2(a.v, b.v),
    };
}

static KinematicState rk4_scale(const KinematicState state, const f32 a) {
    return (KinematicState) {
        .r = HMM_MulV2F(state.r, a),
        .v = HMM_MulV2F(state.v, a),
    };
}

static KinematicState rk4_delta(const KinematicState state, const Simulation *sim, const usize i) {
    return (KinematicState) {
        .r = state.v,
        .v = gravitational_acceleration(sim, state.r, i),
    };
}

static void collide_elastic(const Simulation *sim, usize i, usize j);
static void collide_merge(const Simulation *sim, usize i, usize j);
static void collide_bodies(const Simulation *sim) {
    if (sim->options.collisions == COLLISIONS_NONE) return;

    // TODO: bucket or quadtree optimization?
    for (usize i = 0; i < arrlenu(sim->r); i++) {
        for (usize j = i + 1; j < arrlenu(sim->r); j++) {
            const f32 R_i = body_radius(sim, sim->m[i]);
            const f32 R_j = body_radius(sim, sim->m[j]);

            const HMM_Vec2 delta = HMM_SubV2(sim->r[i], sim->r[j]);
            const f32 distance = HMM_LenV2(delta);
            if (distance > R_i + R_j) continue;
            const HMM_Vec2 relative_velocity = HMM_SubV2(sim->v[i], sim->v[j]);
            if (HMM_DotV2(delta, relative_velocity) > 0.0) continue;

            if (sim->options.collisions == COLLISIONS_BOUNCE) collide_elastic(sim, i, j);
            if (sim->options.collisions == COLLISIONS_MERGE) collide_merge(sim, i, j);
        }
    }
}

// https://www.youtube.com/watch?v=bSVfItpvG5Q & https://en.wikipedia.org/wiki/Elastic_collision
static void collide_elastic(const Simulation *sim, const usize i, const usize j) {
    const f32 R_i = body_radius(sim, sim->m[i]);
    const f32 R_j = body_radius(sim, sim->m[j]);
    const HMM_Vec2 delta = HMM_SubV2(sim->r[i], sim->r[j]);
    const f32 distance = HMM_LenV2(delta);

    // TODO: coefficient of restitution?
    const f32 angle = atan2f(delta.Y, delta.X);
    HMM_Vec2 i_norm = HMM_RotateV2(sim->v[i], -angle);
    HMM_Vec2 j_norm = HMM_RotateV2(sim->v[j], -angle);
    const f32 i_final = ((sim->m[i] - sim->m[j]) / (sim->m[i] + sim->m[j])) * i_norm.X + ((2 * sim->m[j]) / (sim->m[i] + sim->m[j])) * j_norm.X;
    const f32 j_final = ((2 * sim->m[i]) / (sim->m[i] + sim->m[j])) * i_norm.X + ((sim->m[j] - sim->m[i]) / (sim->m[i] + sim->m[j])) * j_norm.X;
    i_norm.X = i_final;
    j_norm.X = j_final;

    sim->v[i] = HMM_RotateV2(i_norm, angle);
    sim->v[j] = HMM_RotateV2(j_norm, angle);

    const f32 overlap_distance = R_i + R_j - distance;
    if (overlap_distance > EPSILON) {
        const HMM_Vec2 overlap = HMM_MulV2F(HMM_NormV2(delta), R_i + R_j - distance);
        sim->r[i] = HMM_AddV2(sim->r[i], HMM_MulV2F(overlap, 1.0f/2.0f));
        sim->r[j] = HMM_SubV2(sim->r[j], HMM_MulV2F(overlap, 1.0f/2.0f));
    }
}

static void collide_merge(const Simulation *sim, const usize i, const usize j) {
    sim->m[i] = sim->m[i] + sim->m[j];
    sim->v[i] = HMM_MulV2F(HMM_AddV2(
        HMM_MulV2F(sim->v[i], sim->m[i]),
        HMM_MulV2F(sim->v[j], sim->m[j])
    ), 1.0f / (sim->m[i] + sim->m[j]));

    // TODO: delete body j
}

f32 body_radius(const Simulation *sim, const f32 mass) {
    return powf(mass / sim->options.density, 1.0f/3.0f);
}

void simulation_free(Simulation *sim) {
    arrfree(sim->r);
    arrfree(sim->v);
    arrfree(sim->m);
    arrfree(sim->movable);
}
