#pragma once

#include "../core/types.h"
#include "shapes.h"

#define baumgarte 0.02f

#define MAX_CONTACTS 32

static f32 Kdl = 0.425f;        // linear damping factor
static f32 Kda = 0.001f;        // angular damping factor

static vec3s Gravity = {0.0f, 0.0f, -15.0f};

static u16 subStepAmount = 8;

typedef struct rigid_body {
  u32 id;
  f32 mass;
  f32 invMass;
  f32 friction;

  vec3s velocity;
  vec3s angularVelocity;

  // Position (center of mass)
  vec3s position;

  // Body origin (not center of mass)
  vec3s origin;

  // Location of center of mass relative to the body origin
  vec3s localCenter;

  mat3s invBodyInertiaTensor;
  quat orientationQuat;
  i32 shapeNum;

  // auxiliary quantities
  mat3s orientation;
  mat3s invWorlInertiaTensor;
  vec3s deltaPosition;
} rigid_body;

typedef struct contact_manifold_point {
  vec3s localPointA;
  vec3s localPointB;
  f32 separation;
  f32 normalImpulse;
  f32 tangentImpulse[2];

} contact_manifold_point;

typedef struct contact_manifold {
  contact_manifold_point point;
  rigid_body *bodyA;
  rigid_body *bodyB;
  i32 contactIdx;
  vec3s normal;
  vec3s tangents[2];
} contact_manifold;

typedef struct hinge_joint {
  rigid_body *bodyA;
  rigid_body *bodyB;

  u32 id;
  vec3s localOriginAnchorA;
  vec3s localOriginAnchorB;
  vec4s localAxisARotation;
  vec4s localAxisBRotation;

  // constraint axii.
  vec3s hingeAxisA;
  vec3s hingeAxisB;

  // Solver shared
  f32 motorImpulse;

  b32 motorEnabled;
  b32 breaking;
  f32 motorMaxTorque;
  f32 motorSpeed;
  f32 motorAngle;

  f32 slipRatio;

  // Local anchors relative to center of mass
  vec3s localAnchorA;
  vec3s localAnchorB;

  f32 invMassA;
  f32 invMassB;
  mat3s invIA;
  mat3s invIB;

  // Hinge constraint
  vec2s totalLambda;

  vec3s mA1;  ///< World space hinge axis for body 1
  vec3s mB2;  ///< World space perpendiculars of hinge axis for body 2
  vec3s mC2;
  vec3s mB2xA1;
  vec3s mC2xA1;
  mat2s invEffectiveMass;
  // Soft constraint
  f32 massCoefficient;
  f32 biasCoefficient;
  f32 impulseCoefficient;

} hinge_joint;

typedef struct rear_axle_motor {
  rigid_body *bodyA;
  rigid_body *bodyB;

  u32 id;
  // constraint axii.
  vec3s hingeAxisA;
  vec3s hingeAxisB;
  vec3s mA1;
  vec4s localAxisARotation;
  // Solver shared
  b32 enabled;
  f32 impulseA;
  f32 impulseB;
  f32 maxTorque;
  f32 speed;
} rear_axle_motor;

typedef struct fixed_joint {
  rigid_body *bodyA;
  rigid_body *bodyB;

  u32 id;
  vec3s localOriginAnchorA;
  vec3s localOriginAnchorB;
  vec4s localAxisARotation;
  vec4s localAxisBRotation;

  // constraint axii.
  vec3s hingeAxisA;
  vec3s hingeAxisB;

  // Local anchors relative to center of mass
  vec3s localAnchorA;
  vec3s localAnchorB;

  f32 invMassA;
  f32 invMassB;
  mat3s invIA;
  mat3s invIB;

  vec3s totalImpulseTrans;
  vec3s totalImpulseRot;

  vec3s mA1;  ///< World space hinge axis for body 1
  vec3s mB2;  ///< World space perpendiculars of hinge axis for body 2
  vec3s mC2;
  vec3s mB2xA1;
  vec3s mC2xA1;

  vec3s centerDiff0;
  mat3s invEffectiveMassTrans;
  mat3s invEffectiveMassRot;
  // Soft constraint
  f32 massCoefficient;
  f32 biasCoefficient;
  f32 impulseCoefficient;

} fixed_joint;

typedef struct distance_joint {
  rigid_body *bodyA;
  rigid_body *bodyB;

  u32 id;
  vec3s localOriginAnchorA;
  vec3s localOriginAnchorB;
  vec4s localAxisARotation;
  vec4s localAxisBRotation;

  // Local anchors relative to center of mass
  vec3s localAnchorA;
  vec3s localAnchorB;

  f32 invMassA;
  f32 invMassB;
  mat3s invIA;
  mat3s invIB;

  // Distance constraint
  vec3s totalImpulse;

  vec3s centerDiff0;
  mat3s invEffectiveMass;

  // Soft constraint
  f32 massCoefficient;
  f32 biasCoefficient;
  f32 impulseCoefficient;
} distance_joint;
