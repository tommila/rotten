#pragma once

#include <stdlib.h>

#include "cglm/types.h"

static bool check_collision_aabb(vec4* aabb1, vec4* aabb2) {
  bool d0 = (*aabb2)[2] < (*aabb1)[0];
  bool d1 = (*aabb1)[2] < (*aabb2)[0];
  bool d2 = (*aabb2)[3] < (*aabb1)[1];
  bool d3 = (*aabb1)[3] < (*aabb2)[1];
  return !(d0 | d1 | d2 | d3);
}

static bool check_collision_aabb(ivec4* aabb1, ivec4* aabb2) {
  bool d0 = (*aabb2)[2] < (*aabb1)[0];
  bool d1 = (*aabb1)[2] < (*aabb2)[0];
  bool d2 = (*aabb2)[3] < (*aabb1)[1];
  bool d3 = (*aabb1)[3] < (*aabb2)[1];
  return !(d0 | d1 | d2 | d3);
}
