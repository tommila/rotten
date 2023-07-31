#version 330

layout(location = 0) in vec3 position;

uniform mat4 mvp;
uniform vec2 offset;

uniform vec2 gridSize;
uniform vec2 gridCells;

uniform vec3 heightMapScale;
uniform vec3 scaleAndFallOf;

uniform sampler2D height_map;

out vec3 color;
out vec2 uv;
out float fog_distance;

const vec3 light_dir = vec3(0.0, 0.5f, 0.5f);

void main() {
  float scale = scaleAndFallOf.x;

  vec4 pos = vec4(position.xy * scale, position.z, 1.0);

  vec2 cellSize = gridSize / gridCells;
  vec2 pos_offset = mod(offset.xy, cellSize * scale);
  fog_distance = length(pos.xy - pos_offset);
  // Offsetting vertex positions with uv will prevent terrain malformations.
  float distanceFromCenter = length(pos.xy);
  pos.xy += offset.xy;
  pos.xy -= pos_offset;

  vec2 heightMapSize = gridSize * heightMapScale.xy;
  vec2 uvOffset = cellSize * 0.5f / heightMapSize + vec2(0.5f,0.5f);
  uv = (pos.xy / heightMapSize) + uvOffset;
  vec4 tex = texture(height_map, uv);
  vec3 normal = normalize(tex.yzw * 2.f - vec3(1.f));
  float height = tex.x;
  vec3 l = normalize(light_dir);
  float d = max(dot(normal, l), 0.0);
  color = vec3(1.0, 1.0, 1.0) * d;
  // color = (normal + vec3(1.f));
  pos.z = height * heightMapScale.z - heightMapScale.z;

  pos.z -= mix(0.f, 1.0f * heightMapScale.z, max(0.0f, distanceFromCenter / (heightMapSize.x * scale) - scaleAndFallOf.y));
  pos.z -= mix(0.f, 1.0f * heightMapScale.z, max(0.0f, 1.0f - distanceFromCenter / (heightMapSize.x * scale) - scaleAndFallOf.z));

  gl_Position = mvp * pos;
}
