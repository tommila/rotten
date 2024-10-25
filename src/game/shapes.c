inline shape_box mergeBoxShape(shape_box oldShape, f32 *points,
                                 usize pointNum) {

  vec3s max = oldShape.max, min = oldShape.min;
  for (usize i = 0; i < pointNum; i+=3) {
    min.x = MIN(points[i],   min.x);
    min.y = MIN(points[i+1], min.y);
    min.z = MIN(points[i+2], min.z);

    max.x = MAX(points[i],   max.x);
    max.y = MAX(points[i+1], max.y);
    max.z = MAX(points[i+2], max.z);
  }

  return {min, max, max - min, (min + max) * 0.5f};
}

inline shape_box createBoxShape(vec3s min, vec3s max) {
  shape_box result = {min, max, max - min, (min + max) * 0.5f};
  return result;
}

inline shape_box createBoxShape(f32 *points) {
  shape_box def = BOX_DEFAULT;
  return mergeBoxShape(def, points, 8 * 3);
}

inline void decomposeBoxShape(shape_box shape, float* out) {
  u16 idx = 0;
  out[idx++] = shape.min.x;  out[idx++] = shape.min.y;  out[idx++] = shape.min.z;
  out[idx++] = shape.max.x;  out[idx++] = shape.min.y;  out[idx++] = shape.min.z;
  out[idx++] = shape.min.x;  out[idx++] = shape.max.y;  out[idx++] = shape.min.z;
  out[idx++] = shape.max.x;  out[idx++] = shape.max.y;  out[idx++] = shape.min.z;

  out[idx++] = shape.min.x;  out[idx++] = shape.min.y;  out[idx++] = shape.max.z;
  out[idx++] = shape.max.x;  out[idx++] = shape.min.y;  out[idx++] = shape.max.z;
  out[idx++] = shape.min.x;  out[idx++] = shape.max.y;  out[idx++] = shape.max.z;
  out[idx++] = shape.max.x;  out[idx++] = shape.max.y;  out[idx++] = shape.max.z;
}

inline shape_plane shape_planeCreateFromTerrainData(vec4s v) {
  shape_plane shape = {(vec3s){v.y,v.z,v.w},v.x};
  return shape;
}
