#version 330

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal0;

uniform mat4 model_matrix;
uniform mat4 view_matrix;
uniform mat4 proj_matrix;

out vec3 normal;

void main() {
  gl_Position = proj_matrix * view_matrix * model_matrix * vec4(position.xyz, 1.0);
  mat3 norm_matrix = transpose(inverse(mat3(model_matrix)));
  vec3 N = normalize(norm_matrix * normal0);
  normal = N;
}
