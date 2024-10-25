#pragma once


#include <cmath>
#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define MAX(A, B) ((A) > (B) ? (A) : (B))

#define LERP(a, b, t) (a + (b - a) * t)

#define CLAMP(A, MIN_VAL, MAX_VAL) MIN(MAX(A, MIN_VAL), MAX_VAL)

#define PI 3.14159265359f
#define Sqrt3Inv (1.0f / sqrtf(3.0f))

typedef versors quat;

mat3s glms_mat3_add(mat3s a, mat3s b);
mat3s glms_mat3_sub(mat3s a, mat3s b);
mat4s glms_mat4_add(mat4s a, mat4s b);

inline vec2s glms_vec2_round(vec2s a) { return {roundf(a.x),roundf(a.y)}; }
inline ivec2s glms_ivec2_add(ivec2s a, ivec2s b) {return {a.x + b.x, a.y + b.y};}
inline vec3s glms_vec3_rotate(vec3s a, vec4s r) {return glms_vec3_rotate(a, r.w, {r.x, r.y, r.z});}

inline vec2s operator+(vec2s a, vec2s b) { return glms_vec2_add(a, b); }
inline ivec2s operator+(ivec2s a, ivec2s b) { return glms_ivec2_add(a, b); }
inline vec3s operator+(vec3s a, vec3s b) { return glms_vec3_add(a, b); }
inline vec3s operator+(vec3s a, float b) { return glms_vec3_adds(a, b); }

inline vec2s operator-(vec2s a, vec2s b) { return glms_vec2_sub(a, b); }
inline vec3s operator-(vec3s a, vec3s b) { return glms_vec3_sub(a, b); }

inline vec2s operator-(vec2s a) { return {-a.x, -a.y}; }
inline vec3s operator-(vec3s a) { return {-a.x, -a.y, -a.z}; }

inline vec2s operator*(vec2s a, float s) { return glms_vec2_scale(a, s); }
inline vec2s operator*(float s, vec2s a) { return glms_vec2_scale(a, s); }

inline vec3s operator*(vec3s a, float s) { return glms_vec3_scale(a, s); }
inline vec3s operator*(float s, vec3s a) { return glms_vec3_scale(a, s); }
inline vec3s operator*(vec3s a, vec3s s) { return glms_vec3_mul(a, s); }

inline mat3s operator*(mat3s a, mat3s b) { return glms_mat3_mul(a, b); }

inline vec2s operator*(mat2s a, vec2s b) { return glms_mat2_mulv(a, b); }
inline vec2s operator*(vec2s b, mat2s a) { return a * b; }
inline mat2s operator*(mat2s a, float b) { return glms_mat2_scale(a, b); }
inline mat2s operator*(float b, mat2s a) { return a * b; }

inline vec3s operator*(mat3s a, vec3s b) { return glms_mat3_mulv(a, b); }
inline vec3s operator*(vec3s b, mat3s a) { return a * b; }

inline vec3s operator*(mat4s a, vec3s b) {
  return glms_mat4_mulv3(a,b,1.0f);
}
inline mat3s operator*(mat3s a, float b) { return glms_mat3_scale(a, b); }
inline mat3s operator*(float b, mat3s a) { return glms_mat3_scale(a, b); }
inline mat3s operator+(mat3s a, mat3s b) { return glms_mat3_add(a, b); }
inline mat3s operator-(mat3s a, mat3s b) { return glms_mat3_sub(a, b); }

inline mat4s operator*(mat4s a, mat4s b) { return glms_mat4_mul(a, b); }
inline mat4s operator+(mat4s a, mat4s b) { return glms_mat4_add(a, b); }
inline mat4s operator+(mat4s a, vec3s b) { return glms_translate(a, b); }

inline vec2s operator/(vec2s a, float s) { return glms_vec2_divs(a, s); }
inline vec2s operator/(float s, vec2s a) { return glms_vec2_divs(a, s); }
inline vec3s operator/(vec3s a, float s) { return glms_vec3_divs(a, s); }
inline vec3s operator/(float s, vec3s a) { return glms_vec3_divs(a, s); }
inline vec2s operator/(vec2s a, vec2s b) { return glms_vec2_div(a, b); }

inline quat operator+(quat a, quat b) { return glms_quat_add(a, b); }
inline quat operator-(quat a, quat b) { return glms_quat_sub(a, b); }
inline quat operator*(quat a, quat b) { return glms_quat_mul(a, b); }
inline quat operator*(float a, quat b) {
  b.w *= a;
  b.x *= a;
  b.y *= a;
  b.z *= a;
  return b;
}
inline quat operator*(quat a, float b) {
  return b * a;
}

inline vec3s glms_vec3_clampv(vec3s v, vec3s min, vec3s max) {
  return {CLAMP(v.x, min.x, max.x),CLAMP(v.y, min.y, max.y),CLAMP(v.z, min.z, max.z)};
}

// “skew-symmetric” angular velocity matrix
inline mat3s glms_angular_vel_mat3(vec3s a) {

  mat3s ret;
  ret.m00 =  0.f;   ret.m01 = -a.z;  ret.m02 =  a.y;
  ret.m10 =  a.z;   ret.m11 =  0.0f; ret.m12 = -a.x;
  ret.m20 = -a.y;   ret.m21 =  a.x;  ret.m22 =  0.f;
  return ret;
}

inline mat3s glms_mat3_add(mat3s a, mat3s b) {
  mat3s ret;
  ret.m00 = a.m00 + b.m00;  ret.m01 = a.m01 + b.m01;  ret.m02 = a.m02 + b.m02;
  ret.m10 = a.m10 + b.m10;  ret.m11 = a.m11 + b.m11;  ret.m12 = a.m12 + b.m12;
  ret.m20 = a.m20 + b.m20;  ret.m21 = a.m21 + b.m21;  ret.m22 = a.m22 + b.m22;
  return ret;
}

inline mat3s glms_mat3_sub(mat3s a, mat3s b) {
  mat3s ret;
  ret.m00 = a.m00 - b.m00;  ret.m01 = a.m01 - b.m01;  ret.m02 = a.m02 - b.m02;
  ret.m10 = a.m10 - b.m10;  ret.m11 = a.m11 - b.m11;  ret.m12 = a.m12 - b.m12;
  ret.m20 = a.m20 - b.m20;  ret.m21 = a.m21 - b.m21;  ret.m22 = a.m22 - b.m22;
  return ret;
}

inline mat4s glms_mat4_add(mat4s a, mat4s b) {
  mat4s ret;
  ret.m00 = a.m00 + b.m00;  ret.m01 = a.m01 + b.m01;  ret.m02 = a.m02 + b.m02;  ret.m03 = a.m03 + b.m03;
  ret.m10 = a.m10 + b.m10;  ret.m11 = a.m11 + b.m11;  ret.m12 = a.m12 + b.m12;  ret.m12 = a.m12 + b.m13;
  ret.m20 = a.m20 + b.m20;  ret.m21 = a.m21 + b.m21;  ret.m22 = a.m22 + b.m23;  ret.m23 = a.m23 + b.m23;
  ret.m30 = a.m30 + b.m30;  ret.m31 = a.m31 + b.m31;  ret.m32 = a.m32 + b.m33;  ret.m33 = a.m33 + b.m33;
  return ret;
}

inline mat3s glms_mat3_orthonormalize(mat3s orientation){

  vec3s X = (vec3s){{orientation.m00, orientation.m10, orientation.m20}};
  vec3s Y = (vec3s){{orientation.m01, orientation.m11, orientation.m21}};
  vec3s Z = (vec3s){{0.0f,0.0f,0.0f}};

  X = glms_vec3_normalize(X);
  Z = glms_vec3_normalize(glms_cross(X, Y));
  Y = glms_vec3_normalize(glms_cross(Z, X));

  mat3s ret;
  ret.m00 = X.x; ret.m01 = Y.x; ret.m02 = Z.x;
  ret.m10 = X.y; ret.m11 = Y.y; ret.m12 = Z.y;
  ret.m20 = X.z; ret.m21 = Y.z; ret.m22 = Z.z;
  return ret;
}

mat3s glms_mat3_skew(vec3s v) {
  // mat3s m;
  // m.m00 = 0.0f, m.m10 = v.z,  m.m20 = v.y;
  // m.m01 = v.z,  m.m11 = 0.0f, m.m21 = -v.x;
  // m.m02 = -v.y, m.m12 = v.x,  m.m22 = 0.0f;


  return {0.0f, -v.z,  v.y,
          v.z,   0.0f, -v.x,
          -v.y,  v.x,   0.0f};
  // return m;
}

inline vec3s rightPerp(vec3s normal) {
  vec3s t1 = glms_cross(normal, GLMS_YUP);
  vec3s t2 = glms_cross(normal, GLMS_ZUP);
  return (glms_vec3_norm2(t1) > glms_vec3_norm2(t2) ? t1 : t2);
  // return glms_vec3_cross(normal, GLMS_ZUP);
}

// float getHeightf(float x, float y) {
//   float h;
//   float s = x - int(x);
//   float t = y - int(y);
//   // now, assuming that the first triangle is made with
//   // vertices (x,y)-(x+1,y)-(x,y+1)
//   float h0 = float(getHeighti(int(x), int(y));
//   float h1 = float(getHeighti(int(x)+1, int(y));
//   float h2 = float(getHeighti(int(x), int(y)+1);
//   float h3 = float(getHeighti(int(x)+1, int(y)+1);
//   if (s > t) { // upper triangle
//     h = h0 + s * (h1 - h0) + t * (h2 - h0);
//   } else { // lower triangle
//     h = h1 + (1 - s) * (h2 - h3) + t * (h3 - h1);
//   }  return h;
// }

inline vec3s findBarycentricCoordinates(vec2s v1,vec2s v2, vec2s v3, vec2s p) {
  f32 lambdaA = ((v2.y - v3.y) * (p.x - v3.x) + (v3.x - v2.x) * (p.y - v3.y)) /
    ((v2.y - v3.y) * (v1.x - v3.x) + (v3.x - v2.x) * (v1.y - v3.y));
  f32 lambdaB = ((v3.y - v1.y) * (p.x - v3.x) + (v1.x - v3.x) * (p.y - v3.y)) /
    ((v2.y - v3.y) * (v1.x - v3.x) + (v3.x - v2.x) * (v1.y - v3.y));
  f32 lambdaC = 1.0f - lambdaA - lambdaB;
  return {lambdaA, lambdaB, lambdaC};
}

inline quat glms_shortest_arc(vec3s v0, vec3s v1) {
  vec3s c = glms_vec3_cross(v0, v1);
  f32 d = glms_vec3_dot(v0, v1);

  if (d < -1.0f + (f32)0.00001) {
    return  {
      0, // x
      1, // y
      0, // z
      0, // w
    };
  } else {
    f32 s = sqrtf((1.0f + d) * 2.0f);
    f32 rs = 1.0f / s;
    return {
      c.x * rs, // x
      c.y * rs, // y
      c.z * rs, // z
      s * 0.5f, // w
    };
  }
}

inline vec3s glms_quat_xform(quat q, vec3s v) {
  vec3s u = {q.x, q.y, q.z};
  vec3s uv = glms_vec3_cross(u, v);
  return v + ((uv * q.w) + glms_vec3_cross(u,uv)) * (2.f);
}

inline vec3s glms_vec3_norm_perpendicular(vec3s v) {
  if (abs(v.raw[0]) > abs(v.raw[1])) {
    float len = sqrtf(v.raw[0] * v.raw[0] + v.raw[2] * v.raw[2]);
    return (vec3s){v.raw[2], 0.0f, -v.raw[0]} / len;
  } else {
    float len = sqrtf(v.raw[1] * v.raw[1] + v.raw[2] * v.raw[2]);
    return (vec3s){0.0f, v.raw[2], -v.raw[1]} / len;
  }
}


// https://box2d.org/2014/02/computing-a-basis/
// https://web.archive.org/web/20191218210035/http://box2d.org:80/2014/02/computing-a-basis/
inline void compute_basis(const vec3s a, vec3s* b, vec3s* c)
{
  // Suppose vector a has all equal components and is a unit vector: a = (s, s, s)
  // Then 3*s*s = 1, s = sqrt(1/3) = 0.57735. This means that at least one component of a
  // unit vector must be greater or equal to 0.57735.

  if (fabsf(a.x) >= 0.57735f)
    *b = (vec3s){a.y, -a.x, 0.0f};
  else
    *b = (vec3s){0.0f, a.z, -a.y};

  *b = glms_vec3_normalize(*b);
  *c = glms_vec3_cross(a, *b);
}

inline ivec3s normalToRGB(vec3s n) {
  return (ivec3s){
    (i32)LERP(0.0, 255.f, (n.x + 1.f) * 0.5f),
    (i32)LERP(0.f, 255.f, (n.y + 1.f) * 0.5f),
    (i32)LERP(0.f, 255.f, (n.z + 1.f) * 0.5f)
  };
};

inline vec4s normalToF32RGBA(vec3s n) {
  vec4s result = {
    (n.x + 1.f) * 0.5f,
    (n.y + 1.f) * 0.5f,
    (n.z + 1.f) * 0.5f,
    1.f
  };
  return result;
};

inline vec4s createPlane(vec3s A, vec3s B, vec3s C){
  vec3s AB = B - A;
  vec3s AC = C - A;
  vec3s N = glms_vec3_normalize(glms_vec3_cross(AB, AC));
  f32 d = glms_vec3_dot(N, A);

  return {N.x, N.y, N.z, d};
}

inline mat4s glms_mat3_make_mat4(mat3s m3) {
  mat4s m4 = GLMS_MAT4_IDENTITY_INIT;
  m4 = glms_mat4_ins3(m3, m4);
  return m4;
}

inline vec3s calcNormal(vec3s A, vec3s B, vec3s C) {
  vec3s AB = B - A;
  vec3s AC = C - A;
  return glms_vec3_normalize(glms_vec3_cross(AB, AC));
};

inline f32 bezier5n(f32 p0, f32 p1, f32 p2, f32 p3, f32 p4, f32 p5, f32 t) {
  return powf((1.f - t), 5.f) * p0 +
    5.f * t * powf((1.f - t), 4.f) * p1 +
    10.f * powf(t, 2.f) * powf((1.f - t), 3.f) * p2 +
    10.f * powf(t, 3.f) * powf((1.f - t), 2.f) * p3 +
    5.f * powf(t, 4.f) * (1.f - t) * p4 +
    powf(t,  5.f) * p5;
}
