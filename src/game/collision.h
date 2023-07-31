// static bool check_collision_aabb(vec4* aabb1, vec4* aabb2) {
//   bool d0 = (*aabb2)[2] < (*aabb1)[0];
//   bool d1 = (*aabb1)[2] < (*aabb2)[0];
//   bool d2 = (*aabb2)[3] < (*aabb1)[1];
//   bool d3 = (*aabb1)[3] < (*aabb2)[1];
//   return !(d0 | d1 | d2 | d3);
// }

// static bool check_collision_aabb(ivec4* aabb1, ivec4* aabb2) {
//   bool d0 = (*aabb2)[2] < (*aabb1)[0];
//   bool d1 = (*aabb1)[2] < (*aabb2)[0];
//   bool d2 = (*aabb2)[3] < (*aabb1)[1];
//   bool d3 = (*aabb1)[3] < (*aabb2)[1];
//   return !(d0 | d1 | d2 | d3);
// }

// static float collision_pointToPlane(vec3s point, shape_plane plane) {
//   // return plane.normal.x * point.x + plane.normal.y * point.y + plane.normal.z * point.z + plane.d;
//   // dotProduct(myPoint - plane.point, plane.normal)
//   return (glms_vec3_dot(plane.normal, point) - plane.d);
//   // return t;
// }
