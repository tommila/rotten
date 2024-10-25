#define BOX_DEFAULT                \
  {{ FLT_MAX,  FLT_MAX,  FLT_MAX}, \
   {-FLT_MAX, -FLT_MAX, -FLT_MAX}, \
   {0.f, 0.f, 0.f}, \
   {0.f, 0.f, 0.f}};

typedef struct shape_box {
  v3 min;
  v3 max;
  v3 size;
  v3 center;
} shape_box;

typedef struct shape_plane {
  v3 normal;
  f32 d;
} shape_plane;

typedef struct shape_sphere {
  v3 p;
  f32 radius;
} shape_sphere;

typedef struct shape_point {
  v3 p;
} shape_point;

typedef enum shape_type {
  SHAPE_UNDEFINED,
  SHAPE_BOX,
  SHAPE_PLANE,
  SHAPE_SPHERE,
  SHAPE_POINT
} shape_type;

typedef struct shape {
  shape_type type;
  union {
    shape_box box;
    shape_plane plane;
    shape_sphere sphere;
    shape_point point;
  };
} shape;
