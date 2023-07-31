#version 330

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal0;
layout(location = 2) in vec2 texcoord0;

uniform mat4 model_matrix;
uniform mat4 view_matrix;
uniform mat4 proj_matrix;

out vec3 normal;

void main() {
  gl_Position = proj_matrix * view_matrix * model_matrix * position;
  mat3 norm_matrix = transpose(inverse(mat3(model_matrix)));
  vec3 N = normalize(norm_matrix * normal0);
  normal = N;
}
