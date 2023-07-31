#version 330

layout(location = 0) in vec4 position;
layout(location = 1) in vec4 color0;

uniform mat4 mvp;

out vec4 color;

void main() {
  gl_Position = mvp * position;
  color = color0;
}
