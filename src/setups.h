// #include "simulation.h"

// void setup_collisions(SimulationState *simulation, Arena *arena) {
//     simulation->gui.collisions = ELASTIC;
//
//     *list_push(&simulation->planets, arena) = (Planet) {
//         .position = (Vector2) { -200.0, 0.0 },
//         .velocity = (Vector2) { 50.0, 0.0 } ,
//         .mass = simulation->gui.mass,
//         .movable = simulation->gui.movable,
//         .color = simulation->gui.color,
//     };
//
//     *list_push(&simulation->planets, arena) = (Planet) {
//         .position = (Vector2) { 0.0, 200.0 },
//         .velocity = (Vector2) { 0.0, -20.0 } ,
//         .mass = simulation->gui.mass,
//         .movable = simulation->gui.movable,
//         .color = simulation->gui.color,
//     };
// }
