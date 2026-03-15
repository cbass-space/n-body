#version 460

layout (std430, set = 0, binding = 0) buffer Matrices {
    mat4 orthographic;
    mat4 view;
};

layout (std430, set = 0, binding = 1) readonly buffer Camera {
    vec2 position;
    float zoom;
    float padding;
};

layout (std140, set = 2, binding = 0) uniform Constants {
    float width;
    float height;
};

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
    float left = zoom * (-width / 2.0);
    float right = zoom * (width / 2.0);
    float bottom = zoom * (-height / 2.0);
    float top = zoom * (height / 2.0);
    float near = 0.0;
    float far = 1.0;

    orthographic[0][0] = 2.0 / (right - left);
    orthographic[1][1] = 2.0 / (top - bottom);
    orthographic[2][2] = 2.0 / (far - near);
    orthographic[3][3] = 1.0;

    orthographic[3][0] = (left + right) / (left - right);
    orthographic[3][1] = (bottom + top) / (bottom - top);
    orthographic[3][2] = near / (near - far);

    vec3 eye = vec3(position, 0.0);
    vec3 center = vec3(position, 1.0);
    vec3 up = vec3(0.0, 1.0, 0.0);

    vec3 F = normalize(eye - center);
    vec3 S = normalize(cross(F, up));
    vec3 U = cross(S, F);

    view[0][0] = S.x;
    view[0][1] = U.x;
    view[0][2] = -F.x;
    view[0][3] = 0.0;

    view[1][0] = S.y;
    view[1][1] = U.y;
    view[1][2] = -F.y;
    view[1][3] = 0.0;

    view[2][0] = S.z;
    view[2][1] = U.z;
    view[2][2] = -F.z;
    view[2][3] = 0.0;

    view[3][0] = -dot(S, eye);
    view[3][1] = -dot(U, eye);
    view[3][2] = dot(F, eye);
    view[3][3] = 1.0;
}

