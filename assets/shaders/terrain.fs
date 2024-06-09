#version 330

in vec2 uv;
in float height;
in vec3 diffuse;

out vec4 frag_color;

uniform sampler2D terrain_tex;

void main() {
  vec4 color = texture(terrain_tex, uv * 10.f);
  color.rgb *= diffuse;
  frag_color = color;
  // frag_color.g += pow(height,1.f);
}
