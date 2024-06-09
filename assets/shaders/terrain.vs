#version 330

layout(location = 0) in vec4 position;

uniform mat4 mvp;
uniform vec2 offset;

uniform sampler2D height_map;

out vec2 uv;
out float height;
out vec3 diffuse;

const vec2 cell_num = vec2(100.f,100.f);

const vec2 plane_size = vec2(512.f, 512.f);
const vec2 world_size = vec2(1.f, 1.f);
const vec2 cell_size = plane_size / cell_num;

const vec3 light_dir = vec3(-1.f,1.0f,0.5f);

vec3 texNormalMap(in vec2 tex_uv) {


  vec2 s = 1.0 / vec2(100.f, 100.f);

  vec3 height;
  vec3 normal;

  height.x = texture(height_map, tex_uv).x;
  height.y = texture(height_map, tex_uv + vec2(s.x, 0.0)).x;
  height.z = texture(height_map, tex_uv + vec2(0.0, s.y)).x;

  normal.xy = (height.x - height.yz);

  // float x = (height.x - height.y);
  // normal.y = (height.x - height.z);
  // normal.x = x;
  normal.xy /= s;

  normal.z = 10.f;

  normal = normalize(normal);
  normal = normal * 0.5 + 0.5;

  return normal;
}

void main() {
  vec4 pos = position;
  vec2 int_part;

  // Offsetting vertex positions with uv will prevent terrain malformations.
  pos.xy += offset + plane_size.xy * 0.5f;
  pos.xy -= mod(offset, cell_size);
  uv = (position.xy / plane_size / world_size) + floor(offset / cell_size) * cell_size / plane_size / world_size;
  vec4 tex = texture(height_map, uv);
  vec3 normal = tex.gba;
  height = tex.x;

  pos.z = height * 20.f;

  vec3 l = normalize(light_dir);
  float diff = max(dot(normal, l), 0.0);
  diffuse = vec3(1.0,1.0,1.0) * diff;

  gl_Position = mvp * pos;
}
