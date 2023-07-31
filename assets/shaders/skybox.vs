#version 330

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal0;
layout(location = 2) in vec2 texcoord0;

uniform mat4 mvp;

out vec2 uv;
out vec3 normal;
out float fog_distance;

void main() {
  vec4 pos = mvp * position;
  gl_Position = vec4(pos.xy, pos.w, pos.w);
  fog_distance = 1.0 - max(position.z, 0.f);
  uv = texcoord0;
  normal = normal0;
}
