#version 330
uniform vec4 u_color;

in vec3 normal;

out vec4 frag_color;

const vec3 light_dir = vec3(0.0, 0.5f, 0.5f);
const vec3 light_color = vec3(227.f/255.f, 188.f/255.f, 99.f/255.f);

void main() {
  vec3 l = normalize(light_dir);
  float d = max(dot(normal, l), 0.4);

  frag_color = vec4(mix(u_color.rgb, light_color, 0.2f), u_color.a);
  frag_color.rgb = frag_color.rgb * d;
}
