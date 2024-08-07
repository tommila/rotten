#version 330

in vec2 uv;
in vec3 normal;
in float fog_distance;

out vec4 frag_color;
uniform sampler2D tex;

const vec4 fog_colour = vec4(90.f/255.f, 128.f/255.f, 143.f/255.f, 1.0);

void main() {
  frag_color = texture(tex, uv);
  frag_color.rgb = mix(
                 frag_color.rgb, fog_colour.rgb,
                 clamp(pow(fog_distance, 4.f), 0.f, 1.f));
}
