#pragma once

#include "cglm/types.h"
#include "cglm/vec2.h"

CGLM_INLINE void glm_vec2_dir_to(vec2 a, vec2 b, vec2 dest) {
  glm_vec2_sub(b, a, dest);
  glm_vec2_normalize(dest);
}

CGLM_INLINE void glm_vec2_clamp(vec2 a, vec2 min, vec2 max, vec2 dest) {
  glm_vec2_maxv(a, min, a);
  glm_vec2_minv(a, max, dest);
}

// CGLM_INLINE void glm_vec2_bounce(vec2 a, vec2 normal, vec2 dest) { return
// -reflect(normal); }

CGLM_INLINE void glm_vec2_reflect(vec2 a, vec2 normal, vec2 dest) {
  vec2 n = {normal[0], normal[1]};
  float dot = glm_vec2_dot(a, n);
  glm_vec2_scale(n, 2, n);
  glm_vec2_scale(n, dot, n);
  glm_vec2_sub(n, a, dest);
  // return 2.0f * p_normal * this->dot(p_normal) - *this;
}

CGLM_INLINE void glm_vec2_slide(vec2 a, vec2 normal, vec2 dest) {
  float dot = glm_vec2_dot(a, normal);
  glm_vec2_scale(normal, dot, normal);
  glm_vec2_sub(a, normal, dest);
  // return 2.0f * p_normal * this->dot(p_normal) - *this;
}
