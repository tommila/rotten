#version 330

layout(location = 0) in vec4 position;
layout(location = 1) in vec2 texcoord0;

uniform mat4 mvp;

out vec2 uv;

void main() {
  gl_Position = mvp * position;
  uv = texcoord0;
}
