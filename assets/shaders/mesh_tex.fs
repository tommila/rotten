#version 330

in vec2 uv;
in float height;

out vec4 frag_color;

uniform sampler2D tex;

void main() {
  frag_color = texture(tex, uv * 10.f);
  frag_color.g += pow(height,3.f);
}
