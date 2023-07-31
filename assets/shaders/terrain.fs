#version 330

in vec2 uv;
in vec3 color;
in float fog_distance;

out vec4 frag_color;

uniform sampler2D terrain_tex;

const float fog_maxdist = 3000.0;
const float fog_mindist = 500.0;
const vec4 fog_colour = vec4(90.f/255.f, 128.f/255.f, 143.f/255.f, 1.0);;

void main() {
  float uvRepeat = 160.0;
  vec4 c = texture(terrain_tex, uv * uvRepeat);
  // c.rgb = mix(c.rgb, color, 1.0);
  c.rgb *= color;

  // Calculate fog
  float fog_factor = (fog_maxdist - fog_distance) /
    (fog_maxdist - fog_mindist);
  fog_factor = clamp(fog_factor, 0.0f, 1.0f);
  frag_color = mix(fog_colour, c, pow(fog_factor,2.0f));
}
