#version 460

layout (std430, set = 0, binding = 0) buffer Positions { vec2 positions[]; };
layout (std430, set = 0, binding = 1) buffer Velocities { vec2 velocities[]; };
layout (std430, set = 0, binding = 2) buffer Masses { vec2 masses[]; };
layout (std430, set = 0, binding = 3) buffer Movable { vec2 movables[]; };

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
    uint index = gl_GlobalInvocationID.x;
    positions[index] += velocities[index];
}
