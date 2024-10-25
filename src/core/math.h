#ifndef ROTTEN_MATH
#define ROTTEN_MATH

#include <math.h>
#define PI 3.14159265f
#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define MAX(A, B) ((A) > (B) ? (A) : (B))

#define LERP(a, b, t) ((a) + ((b) - (a)) * (t))

#define CLAMP(A, MIN_VAL, MAX_VAL) MIN(MAX(A, MIN_VAL), MAX_VAL)
#define DEG2RAD(D) (D * (PI / 180.f)) 
#define SIGNF(a) (a < 0.0f ? -1.0f : 1.0f)

// Vector2
#define V2_ZERO {0.0f,0.0f}
typedef union v2 {
  f32 arr[2];
  struct {
    f32 x,y;
  };
  struct {
    f32 u, v;
  };
#ifdef __cplusplus
 inline f32& operator[](i32 i) {
  f32& result = arr[i];
  return result;
  };
#endif
} v2;

#ifdef __cplusplus
inline v2 operator-(v2 a, v2 b) {
  v2 result = {a.x - b.x, a.y - b.y};
  return result;
};

inline v2 operator+(v2 a, v2 b) {
  v2 result = {a.x + b.x, a.y + b.y};
  return result;
};

inline v2 operator*(v2 a, v2 b) {
  return {a.x * b.x, a.y * b.y};
};

inline v2 operator*(f32 s, v2 a) {
  v2 result = {a.x * s, a.y * s};
  return result;
};

inline v2 operator*(v2 a, f32 s) {
  return s * a;
};

inline v2 operator/(f32 s, v2 a) {
  v2 result = {a.x / s, a.y / s};
  return result;
};

inline v2 operator/(v2 a, f32 s) {
  return s / a;
};

#endif

inline f32 v2_length2(v2 a) {
  f32 result = a.x * a.x + a.y * a.y;
  return result;
};

inline f32 v2_length(v2 a) {
  f32 result = v2_length2(a);
  if (result > 0.f) {
    result = sqrtf(result);
  }
  return result;
};

typedef union v2i {
  i32 arr[2];
  struct {
    i32 x,y;
  };
  struct {
    i32 u, v;
  };
} v2i;

#ifdef __cplusplus
inline v2i operator-(v2i a, v2i b) {
  v2i result = {a.x - b.x, a.y - b.y};
  return result;
};

inline v2i operator+(v2i a, v2i b) {
  v2i result = {a.x + b.x, a.y + b.y};
  return result;
};

inline v2i operator/(v2i a, i32 s) {
  v2i result = {a.x / s, a.y / s};
  return result;
};

#endif

#define V3_ZERO {0.0f, 0.0f, 0.0f}
#define V3_X_UP {0.0f, 0.0f, 1.0f}
// Vector3
typedef union v3 {
  f32 arr[3];
  struct {
    f32 x,y,z;
  };
  struct {
    f32 r, g, b;
  };
  struct {
    v2 xy;
    f32 _unused0;
  };
#ifdef __cplusplus
  inline f32& operator[](i32 i){
    f32& result = arr[i];
    return result;
    };
#endif
} v3;

inline v3 v3_sub(v3 a, v3 b) {
  v3 result = {a.x - b.x, a.y - b.y, a.z - b.z};
  return result;
};

#ifdef __cplusplus
inline v3 operator-(v3 a, v3 b) {
  v3 result = v3_sub(a, b);
  return result;
};

inline v3 operator-(v3 a) {
  v3 result = {-a.x, -a.y, -a.z};
  return result;
};

inline v3 operator+(v3 a, v3 b) {
  v3 result = {a.x + b.x, a.y + b.y, a.z + b.z};
  return result;
};

inline v3& operator+=(v3& a, v3 b) {
  a.x += b.x;
  a.y += b.y;
  a.z += b.z;
  return a;
};
inline v3& operator-=(v3& a, v3 b) {
  a.x -= b.x;
  a.y -= b.y;
  a.z -= b.z;
  return a;
};


inline v3 operator*(f32 s, v3 a) {
  v3 result = {a.x * s, a.y * s, a.z * s};
  return result;
};

inline v3 operator*(v3 a, f32 s) {
  return s * a;
};

inline v3 operator*(v3 a, v3 b) {
  return {a.x * b.x, a.y * b.y, a.z * b.z};
};

inline v3 operator/(v3 a, f32 s) {
  v3 result = {a.x / s, a.y / s, a.z / s};
  return result;
};
#endif

inline f32 v3_length2(v3 a) {
  f32 result = a.x * a.x + a.y * a.y + a.z * a.z;
  return result;
};

inline f32 v3_length(v3 a) {
  f32 result = v3_length2(a);
  if (result > 0.f) {
    result = sqrtf(result);
  }
  return result;
};

inline v3 v3_normalize(v3 a) {
  f32 l = v3_length(a);
  v3 result = l ? (v3){a.x / l, a.y / l, a.z / l} : a;
  return result;
};

inline f32 v3_dot(v3 a, v3 b) {
  f32 result = a.x * b.x + a.y * b.y + a.z * b.z;
  return result;
};

inline v3 v3_cross(v3 a, v3 b) {
  v3 result = {a.y * b.z - a.z * b.y,
               a.z * b.x - a.x * b.z,
               a.x * b.y - a.y * b.x};
  return result;
};

inline v3 v3_direction_to(v3 a, v3 b) {
  v3 result = v3_normalize(v3_sub(b, a));
  return result;
};

inline v3 v3_rotate_axis_angle(v3 v, f32 angle, v3 k) {
  // Axis angle rotation using Rodrigues' rotation formula
  f32 cosA = cosf(angle);
  f32 sinA = sinf(angle);
  v3 crossKV = v3_cross(k, v);
  f32 d = v3_dot(k,v) * (1.f - cosA); 
  v3 kDot = {k.x * d, k.y * d, k.z * d};

  v3 result = (v3){v.x * cosA + crossKV.x * sinA + kDot.x,
                   v.y * cosA + crossKV.y * sinA + kDot.y,
                   v.z * cosA + crossKV.z * sinA + kDot.z};
  return result;                                                        
}

// Vector4
typedef union v4 {
  f32 arr[4];
  struct {
    f32 x,y,z,w;
  };
  struct {
    f32 r,g,b,a;
  };
  struct {
    v3 xyz;
    f32 _unused0;
  };
  struct {
    v2 xy;
    f32 _unused1;
    f32 _unused2;
  };

#ifdef __cplusplus
  inline f32& operator[](i32 i) {
    f32& result = arr[i];
    return result;
  };

  inline v4& operator=(v3 v){
    *this = {v.x, v.y, v.z, 0.f};
    return *this;
  };

#endif
} v4;

#ifdef __cplusplus

inline v4 operator-(v4 a, v4 b) {
  v4 result = {a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w};
  return result;
};

inline v4 operator-(v4 a) {
  v4 result = {-a.x, -a.y, -a.z};
  return result;
};

inline v4 operator+(v4 a, v4 b) {
  v4 result = {a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w};
  return result;
};

inline v4& operator+=(v4& a, v4 b) {
  a.x += b.x;
  a.y += b.y;
  a.z += b.z;
  a.w += b.w;
  return a;
};

inline v4 operator*(v4 a, f32 s) {
  v4 result = {a.x * s, a.y * s, a.z * s, a.w * s};
  return result;
};
inline v4 operator/(v4 a, f32 s) {
  v4 result = {a.x / s, a.y / s, a.z / s, a.w / s};
  return result;
};

#endif

inline v4 v4_from_v3(v3 v, f32 w) {
  v4 result = {v.x, v.y, v.z, w};
  return result;
}

typedef union v4i {
  u32 arr[4];
  struct {
    u32 x,y,z,w;
  };
 struct {
    u32 r,g,b,a;
  };
  struct {
    v3 xyz;
    u32 _unused0;
  };
  struct {
    v2 xy;
    u32 _unused1;
    u32 _unused2;
  };
#ifdef __cplusplus
  inline u32& operator[](i32 i) {
    u32& result = arr[i];
    return result;
  };

  #endif
} v4i;

typedef union rgb8 {
  u8 arr[3];
  struct {
    u8 r,g,b;
  };
  #ifdef __cplusplus
  inline u8& operator[](i32 i) {
    u8& result = arr[i];
    return result;
  };

  #endif
} rgb8;

typedef union rgba8 {
  u8 arr[4];
  struct {
    u8 r,g,b,a;
  };
  struct {
    rgb8 rgb;
    u8 _unusedAlpha;
  };
  #ifdef __cplusplus
  inline u8& operator[](i32 i) {
    u8& result = arr[i];
    return result;
  };

  #endif
} rgba8;
#ifdef __cplusplus

inline rgba8 operator-(rgba8 a, rgba8 b) {
  rgba8 result = (rgba8){(u8)(a.r - b.r), (u8)(a.g - b.g), (u8)(a.b - b.b),(u8)(a.a - b.a)};
  return result;
};

inline rgba8 operator-(rgba8 a) {
  rgba8 result = {(u8)-a.r, (u8)-a.g, (u8)-a.b};
  return result;
};

inline rgba8 operator+(rgba8 a, rgba8 b) {
  rgba8 result = {(u8)(a.r + b.r), (u8)(a.g + b.g), (u8)(a.b + b.b), (u8)(a.a + b.a)};
  return result;
};

inline rgba8& operator+=(rgba8& a, rgba8 b) {
  a.r += b.r;
  a.g += b.g;
  a.b += b.b;
  a.a += b.a;
  return a;
};

inline rgba8 operator+(rgba8 a, rgb8 b) {
  rgba8 result = {(u8)(a.r + b.r), (u8)(a.g + b.g), (u8)(a.b + b.b), a.a};
  return result;
};

inline rgba8& operator+=(rgba8& a, rgb8 b) {
  a.r += b.r;
  a.g += b.g;
  a.b += b.b;
  return a;
};


inline rgba8 operator*(rgba8 a, f32 s) {
  rgba8 result = {(u8)(a.r * s), (u8)(a.g * s), (u8)(a.b * s), (u8)(a.a * s)};
  return result;
};
inline rgba8 operator/(rgba8 a, f32 s) {
  rgba8 result = {(u8)(a.r / s), (u8)(a.g / s), (u8)(a.b / s), (u8)(a.a / s)};
  return result;
};

#endif

#define QUAT_IDENTITY  {1.0f, 0.0f, 0.0f, 0.0f}

typedef union quat {
  f32 versors[4]; 
  struct {
    f32 s,x,y,z;
  };
} quat;

#ifdef __cplusplus
inline quat operator+(quat a, quat b) {
  quat result = a;
  result.s += b.s;
  result.x += b.x;
  result.y += b.y;
  result.z += b.z;
  return result;
};

inline quat operator-(quat a, quat b) {
  quat result = a;
  result.s -= b.s;
  result.x -= b.x;
  result.y -= b.y;
  result.z -= b.z;
  return result;
};

inline quat operator*(quat a, quat b) {
  quat result = {
    a.s * b.s - a.x * b.x - a.y * b.y - a.z * b.z,
    a.s * b.x + b.s * a.x + a.y * b.z - b.y * a.z,
    a.s * b.y + b.s * a.y + a.z * b.x - b.z * a.x,
    a.s * b.z + b.s * a.z + a.x * b.y - b.x * a.y
  };
  return result;
};

inline quat operator/(quat a, f32 s) {
  quat result = {a.s / s, a.x / s, a.y / s, a.z / s};
  return result;
};
#endif

inline f32 quat_length2(quat q) {
  f32 result = q.s * q.s + q.x * q.x + q.y * q.y + q.z * q.z;
  return result; 
};

inline f32 quat_length(quat q) {
  f32 lenSqrt = quat_length2(q); 
  f32 result = lenSqrt ? sqrtf(lenSqrt) : 0.f;
  return result;
};

inline quat quat_normalize(quat q) {
  f32 l = quat_length(q);
  quat result;
  result = l ? (quat){q.s / l, q.x / l, q.y / l, q.z / l} : q;
  return result;
};

// Matrix 3x3 (column major)
typedef union m2x2 {
  f32 arr[4];
  v2 col[2];
  struct {
    f32 m00, m01;
    f32 m10, m11;
  };
} m2x2;

inline m2x2 m2x2_scale(m2x2 m, f32 s) {
  m2x2 result = {
    m.m00 * s, m.m01 * s,
    m.m10 * s, m.m11 * s,
  };
  return result;
};

#ifdef __cplusplus
inline m2x2 operator*(m2x2 m, f32 s) {
  m2x2 result = m2x2_scale(m, s);
  return result;
};

inline m2x2 operator*(f32 s, m2x2 m) {
  return m * s;
};

inline v2 operator*(m2x2 m, v2 v) {
  v2 result = {
      m.m00 * v[0] + m.m10 * v[1],
      m.m01 * v[0] + m.m11 * v[1],
  };
  return result;
};

inline v2 operator*(v2 v, m2x2 m) {
  return v * m;
};
#endif

inline m2x2 m2x2_inverse(m2x2 m) {
  // det(m) = ad−bc
  f32 a = m.m00, b = m.m01,
      c = m.m10, d = m.m11;    
  f32 invDet = 1.f / (a * d - b * c);
  m2x2 result = m2x2_scale((m2x2){d, -b, -c, a}, invDet);
  return result;
}

// Matrix 3x3 (column major)
#define M3X3_IDENTITY        \
        {1.f, 0.f, 0.f, \
         0.f, 1.f, 0.f, \
         0.f, 0.f, 1.f} \


typedef union m3x3 {
  f32 arr[9];
  v3 col[3];
  struct {
    f32 m00, m01, m02;
    f32 m10, m11, m12;
    f32 m20, m21, m22;
  };
} m3x3;

inline v3 m3x3_mul_v3(m3x3 m, v3 v) {
  v3 result = {
      m.m00 * v.x + m.m10 * v.y + m.m20 * v.z,
      m.m01 * v.x + m.m11 * v.y + m.m21 * v.z,
      m.m02 * v.x + m.m12 * v.y + m.m22 * v.z,
  };
  return result;
};

inline m3x3 m3x3_scale(m3x3 m, f32 s) {
  m3x3 result = {
    m.m00 * s, m.m01 * s, m.m02 * s,
    m.m10 * s, m.m11 * s, m.m12 * s,
    m.m20 * s, m.m21 * s, m.m22 * s,
  };
  return result;
};

#ifdef __cplusplus
inline m3x3 operator+(m3x3 m1, m3x3 m2) {
  m3x3 result;
  result.col[0] = m1.col[0] + m2.col[0];
  result.col[1] = m1.col[1] + m2.col[1];
  result.col[2] = m1.col[2] + m2.col[2];
  return result;
};

inline m3x3 operator*(m3x3 m, f32 s) {
  m3x3 result = m3x3_scale(m, s);
  return result;
};

inline m3x3 operator*(f32 s, m3x3 m) {
  return m * s;
};

inline v3 operator*(m3x3 m, v3 v) {
  v3 result = m3x3_mul_v3(m, v);
  return result;
};

inline v3 operator*(v3 v, m3x3 m) {
  return m * v;
};

inline m3x3 operator*(m3x3 m1, m3x3 m2) {

  v3 c1 = {
    m1.m00 * m2.m00 + m1.m10 * m2.m01 + m1.m20 * m2.m02,
    m1.m01 * m2.m00 + m1.m11 * m2.m01 + m1.m21 * m2.m02,
    m1.m02 * m2.m00 + m1.m12 * m2.m01 + m1.m22 * m2.m02};

  v3 c2 = {
    m1.m00 * m2.m10 + m1.m10 * m2.m11 + m1.m20 * m2.m12,
    m1.m01 * m2.m10 + m1.m11 * m2.m11 + m1.m21 * m2.m12,
    m1.m02 * m2.m10 + m1.m12 * m2.m11 + m1.m22 * m2.m12};

  v3 c3 = {
    m1.m00 * m2.m20 + m1.m10 * m2.m21 + m1.m20 * m2.m22,
    m1.m01 * m2.m20 + m1.m11 * m2.m21 + m1.m21 * m2.m22,
    m1.m02 * m2.m20 + m1.m12 * m2.m21 + m1.m22 * m2.m22};

  m3x3 result;
  result.col[0] = c1;
  result.col[1] = c2;
  result.col[2] = c3;
  return result;
};
#endif

inline m3x3 m3x3_transpose(m3x3 m) {
  m3x3 result = {
    m.m00,m.m10,m.m20,
    m.m01,m.m11,m.m21,
    m.m02,m.m12,m.m22};
  return result;
}

inline m3x3 m3x3_inverse(m3x3 m) {
  // det(m) = a(ek−fh)−b(dk−fg)+c(dh−eg) 
  f32 a = m.m00, b = m.m01, c = m.m02,
      d = m.m10, e = m.m11, f = m.m12,
      g = m.m20, h = m.m21, k = m.m22;

  f32 invDet = 1.f / (a*(e*k - f*h) - b*(d*k - f*g) + c*(d*h - e*g));
  m3x3 result = m3x3_scale((m3x3){
      (e * k - f * h), -(b * k - c * h),  (b * f - c * e),
     -(d * k - f * g),  (a * k - c * g), -(a * f - c * d),
      (d * h - e * g), -(a * h - b * g),  (a * e - b * d)
  }, invDet);
  return result;
}
// Requires the quatarnion to be normalized
inline m3x3 m3x3_from_quat(quat q) {
  m3x3 result;
  f32 x2 = q.x * q.x;
  f32 y2 = q.y * q.y;
  f32 z2 = q.z * q.z;
     
  result.m00 = 1.f - 2.f * ( y2 + z2 ); 
  result.m11 = 1.f - 2.f * ( x2 + z2 ); 
  result.m22 = 1.f - 2.f * ( x2 + y2 );

  f32 xy = q.x * q.y;
  f32 zs = q.z * q.s;
  result.m01 = 2.0 * (xy + zs);
  result.m10 = 2.0 * (xy - zs);
  
  f32 xz = q.x * q.z;
  f32 ys = q.y * q.s;
  result.m02 = 2.0 * (xz - ys);
  result.m20 = 2.0 * (xz + ys);
  f32 yz = q.y * q.z;
  f32 xs = q.x * q.s;
  result.m12 = 2.0 * (yz + xs);
  result.m21 = 2.0 * (yz - xs);
  return result;
}

inline m3x3 m3x3_rotate_make(v3 angles) {

  f32 sX = sinf(angles.x), sY = sinf(angles.y), sZ = sinf(angles.z);
  f32 cX = cosf(angles.x), cY = cosf(angles.y), cZ = cosf(angles.z);

  m3x3 result = M3X3_IDENTITY;
  result.m00 = cY*cZ;
  result.m01 = -cX*sZ + sX*sY*cZ;
  result.m02 = sX*sZ + cX*sY*cZ;
  
  result.m10 = cY*sZ;
  result.m11 = cX*cZ + sX*sY*sZ;
  result.m12 = -sX*cZ + cX*sY*sZ;

  result.m20 = -sY;
  result.m21 = sX*cY;
  result.m22 = cX*cY;

  return result;
}
#define M4X4_IDENTITY        \
        {1.f, 0.f, 0.f, 0.f, \
         0.f, 1.f, 0.f, 0.f, \
         0.f, 0.f, 1.f, 0.f, \
         0.f, 0.f, 0.f, 1.f}

// Matrix 4x4 (column major)
typedef union m4x4 {
  f32 arr[16];
  v4 col[4];
  struct {
    f32 m00, m01, m02, m03;
    f32 m10, m11, m12, m13;
    f32 m20, m21, m22, m23;
    f32 m30, m31, m32, m33;
  };
#ifdef __cplusplus
  inline m4x4& operator=(const m3x3& m);
#endif
} m4x4;

#ifdef __cplusplus
inline m4x4& m4x4::operator=(const m3x3& m) {
  (*this).col[0] = m.col[0];
  (*this).col[1] = m.col[1];
  (*this).col[2] = m.col[2];
  return *this;
}

inline m4x4 operator*(m4x4 m, f32 s) {
  m4x4 result = {
    m.m00 * s, m.m01 * s, m.m02 * s,m.m03 * s,
    m.m10 * s, m.m11 * s, m.m12 * s,m.m13 * s,
    m.m20 * s, m.m21 * s, m.m22 * s,m.m23 * s,
    m.m30 * s, m.m31 * s, m.m32 * s,m.m33 * s,
  };
  return result;
};

inline v4 operator*(m4x4 m, v4 v) {
  v4 result = {
      m.m00 * v[0] + m.m10 * v[1] + m.m20 * v[2] + m.m30 * v[3],
      m.m01 * v[0] + m.m11 * v[1] + m.m21 * v[2] + m.m31 * v[3],
      m.m02 * v[0] + m.m12 * v[1] + m.m22 * v[2] + m.m32 * v[3],
      m.m03 * v[0] + m.m13 * v[1] + m.m23 * v[2] + m.m33 * v[3]
  };
  return result;
};

inline m4x4 operator*(m4x4 m1, m4x4 m2) {

  v4 c1 = {
    m1.m00 * m2.m00 + m1.m10 * m2.m01 + m1.m20 * m2.m02 + m1.m30 * m2.m03,
    m1.m01 * m2.m00 + m1.m11 * m2.m01 + m1.m21 * m2.m02 + m1.m31 * m2.m03,
    m1.m02 * m2.m00 + m1.m12 * m2.m01 + m1.m22 * m2.m02 + m1.m32 * m2.m03,
    m1.m03 * m2.m00 + m1.m13 * m2.m01 + m1.m23 * m2.m02 + m1.m33 * m2.m03};

  v4 c2 = {
    m1.m00 * m2.m10 + m1.m10 * m2.m11 + m1.m20 * m2.m12 + m1.m30 * m2.m13,
    m1.m01 * m2.m10 + m1.m11 * m2.m11 + m1.m21 * m2.m12 + m1.m31 * m2.m13,
    m1.m02 * m2.m10 + m1.m12 * m2.m11 + m1.m22 * m2.m12 + m1.m32 * m2.m13,
    m1.m03 * m2.m10 + m1.m13 * m2.m11 + m1.m23 * m2.m12 + m1.m33 * m2.m13};

  v4 c3 = {
    m1.m00 * m2.m20 + m1.m10 * m2.m21 + m1.m20 * m2.m22 + m1.m30 * m2.m23,
    m1.m01 * m2.m20 + m1.m11 * m2.m21 + m1.m21 * m2.m22 + m1.m31 * m2.m23,
    m1.m02 * m2.m20 + m1.m12 * m2.m21 + m1.m22 * m2.m22 + m1.m32 * m2.m23,
    m1.m03 * m2.m20 + m1.m13 * m2.m21 + m1.m23 * m2.m22 + m1.m33 * m2.m23};

  v4 c4 = {
    m1.m00 * m2.m30 + m1.m10 * m2.m31 + m1.m20 * m2.m32 + m1.m30 * m2.m33,
    m1.m01 * m2.m30 + m1.m11 * m2.m31 + m1.m21 * m2.m32 + m1.m31 * m2.m33,
    m1.m02 * m2.m30 + m1.m12 * m2.m31 + m1.m22 * m2.m32 + m1.m32 * m2.m33,
    m1.m03 * m2.m30 + m1.m13 * m2.m31 + m1.m23 * m2.m32 + m1.m33 * m2.m33};

  m4x4 result;
  result.col[0] = c1;
  result.col[1] = c2;
  result.col[2] = c3;
  result.col[3] = c4;
  return result;
};

inline v3 operator*(m4x4 a, v3 b) {
  v4 v = {b.x,b.y,b.z,1.f};
  v3 result = (a * v).xyz;
  return result;
}

#endif

inline m4x4 m4x4_translate_make(v4 v) {
  m4x4 m = M4X4_IDENTITY;
  m.col[3] = v;
  return m;
}

inline m4x4 m4x4_scale_make(v3 v) {
  m4x4 m = M4X4_IDENTITY;
  m.m00 = v.x;
  m.m11 = v.y;
  m.m22 = v.z;
  return m;
}

inline m4x4 m4x4_rotate_make(v3 angles) {

  f32 sX = sinf(angles.x), sY = sinf(angles.y), sZ = sinf(angles.z);
  f32 cX = cosf(angles.x), cY = cosf(angles.y), cZ = cosf(angles.z);

  m4x4 result = M4X4_IDENTITY;
  result.m00 = cY*cZ;
  result.m01 = -cX*sZ + sX*sY*cZ;
  result.m02 = sX*sZ + cX*sY*cZ;
  
  result.m10 = cY*sZ;
  result.m11 = cX*cZ + sX*sY*sZ;
  result.m12 = -sX*cZ + cX*sY*sZ;

  result.m20 = -sY;
  result.m21 = sX*cY;
  result.m22 = cX*cY;

  return result;
}

inline m4x4 m4x4_from_m3x3(m3x3 m) {
  m4x4 result = M4X4_IDENTITY;
  result.col[0].xyz = m.col[0];
  result.col[1].xyz = m.col[1];
  result.col[2].xyz = m.col[2];
  return result;
}

inline m4x4 perspective(f32 fov, f32 aspectRatio, f32 near, f32 far) {
  f32 f = tanf(fov * 0.5f);
  m4x4 result = {0.f};
  result.m00 = 1.f / (aspectRatio * f);
  result.m11 = 1.f / f;
  result.m22 = -((far + near) / (far - near));
  result.m23 = -1.f;
  result.m32 = -((2.f * far * near) / (far - near));
  return result;
}

inline m4x4 lookFromTo(v3 eye, v3 target, v3 upDirection) {
  m4x4 result = M4X4_IDENTITY;

  v3 forward, left, up;
  
  forward = v3_normalize(v3_sub(eye, target));
  left = v3_normalize(v3_cross(upDirection, forward));
  up = v3_normalize(v3_cross(forward, left));

  result.m00 = left.x   ; result.m10 = left.y   ; result.m20 = left.z;
  result.m01 = up.x     ; result.m11 = up.y     ; result.m21 = up.z;
  result.m02 = forward.x; result.m12 = forward.y; result.m22 = forward.z;
  
  result.m30 = -left.x * eye.x - left.y * eye.y - left.z * eye.z;
  result.m31 = -up.x * eye.x - up.y * eye.y - up.z * eye.z;
  result.m32 = -forward.x * eye.x - forward.y * eye.y - forward.z * eye.z;

  return result;
}

inline m4x4 lookAt(v3 target, v3 upDirection) {
  m4x4 result = M4X4_IDENTITY;

  v3 forward, left, up;
  
  forward = v3_normalize(target);
  left = v3_normalize(v3_cross(upDirection, forward));
  up = v3_normalize(v3_cross(forward, left));

  result.m00 = left.x   ; result.m10 = left.y   ; result.m20 = left.z;
  result.m01 = up.x     ; result.m11 = up.y     ; result.m21 = up.z;
  result.m02 = forward.x; result.m12 = forward.y; result.m22 = forward.z;
  
  return result;
}

#define Sqrt3Inv (1.0f / sqrtf(3.0f))

inline m3x3 m3x3_skew_symmetric(v3 v) {
  // mat3s m;
  // m.m00 = 0.0f, m.m10 = v.z,  m.m20 = v.y;
  // m.m01 = v.z,  m.m11 = 0.0f, m.m21 = -v.x;
  // m.m02 = -v.y, m.m12 = v.x,  m.m22 = 0.0f;

  return (m3x3){0.0f, -v.z,  v.y,
          v.z,   0.0f, -v.x,
          -v.y,  v.x,   0.0f};
  // return m;
}

inline v3 perpendicular(v3 v) {
  if (fabsf(v.arr[0]) > fabsf(v.arr[1])) {
    f32 len = sqrtf(v.arr[0] * v.arr[0] + v.arr[2] * v.arr[2]);
    return (v3){v.arr[2] / len, 0.0f, -v.arr[0] / len};
  } else {
    f32 len = sqrtf(v.arr[1] * v.arr[1] + v.arr[2] * v.arr[2]);
    return (v3){0.0f, v.arr[2] / len, -v.arr[1] / len};
  }
}

// https://box2d.org/2014/02/computing-a-basis/
// https://web.archive.org/web/20191218210035/http://box2d.org:80/2014/02/computing-a-basis/
inline void compute_basis(const v3 a, v3* b, v3* c)
{
  // Suppose vector a has all equal components and is a unit vector: a = (s, s, s)
  // Then 3*s*s = 1, s = sqrt(1/3) = 0.57735. This means that at least one component of a
  // unit vector must be greater or equal to 0.57735.

  if (fabsf(a.x) >= 0.57735f)
    *b = (v3){a.y, -a.x, 0.0f};
  else
    *b = (v3){0.0f, a.z, -a.y};

  *b = v3_normalize(*b);
  *c = v3_cross(a, *b);
}

inline v3 calcNormal(v3 A, v3 B, v3 C) {
  v3 AB = v3_sub(B, A);
  v3 AC = v3_sub(C, A);
  return v3_normalize(v3_cross(AB, AC));
};

inline f32 bezier6n(f32* p, f32 t) {
  return powf((1.f - t), 5.f) * p[0] +
    5.f * t * powf((1.f - t), 4.f) * p[1] +
    10.f * powf(t, 2.f) * powf((1.f - t), 3.f) * p[2] +
    10.f * powf(t, 3.f) * powf((1.f - t), 2.f) * p[3] +
    5.f * powf(t, 4.f) * (1.f - t) * p[4] +
    powf(t,  5.f) * p[5];
}

#endif // ROTTEN_MATH
