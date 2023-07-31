// Adapted to Rotten engine from Erin Catto Solver2d library

// SPDX-FileCopyrightText: 2024 Erin Catto
// SPDX-License-Identifier: MIT


#include <math.h>
#include "cglm/struct/euler.h"
#include "cglm/struct/mat3.h"
#include "cglm/struct/mat4.h"
#include "cglm/struct/quat.h"
#include "cglm/struct/vec3.h"
#include "cglm/types-struct.h"
#include "physics.h"

static void applyLinearVelocityStep(rigid_body* bodyA, rigid_body* bodyB, distance_joint* joint,
                              vec3s impulse) {
  mat3s qA = bodyA->orientation;
  mat3s qB = bodyB->orientation;

  vec3s rA = qA * joint->localAnchorA;
  vec3s rB = qB * joint->localAnchorB;

  f32 mA = joint->invMassA, mB = joint->invMassB;
  mat3s iA = joint->invIA, iB = joint->invIB;

  bodyA->velocity = bodyA->velocity - mA * impulse;
  bodyA->angularVelocity = bodyA->angularVelocity - iA * glms_vec3_cross(rA, impulse);

  bodyB->velocity = bodyB->velocity + mB * impulse;
  bodyB->angularVelocity = bodyB->angularVelocity + iB * glms_vec3_cross(rB, impulse);
}

inline void setupDistanceJoint(distance_joint* joint, rigid_body* rbA, rigid_body* rbB,
                       vec3s pivotA, vec3s pivotB, vec3s axisInA, vec3s axisInB) {
  joint->bodyA = rbA;
  joint->bodyB = rbB;

  joint->localOriginAnchorA = pivotA;
  joint->localOriginAnchorB = pivotB;
}

inline void preSolveDistanceJoint(distance_joint* joint, f32 h, b32 warmStart) {
  rigid_body* bodyA = joint->bodyA;
  rigid_body* bodyB = joint->bodyB;

  joint->localAnchorA = joint->localOriginAnchorA;
  joint->localAnchorB = joint->localOriginAnchorB;

  joint->invMassB = bodyB->invMass;
  joint->invMassA = bodyA->invMass;

  joint->invIB = bodyB->invWorlInertiaTensor;
  joint->invIA = bodyA->invWorlInertiaTensor;

  f32 mA = joint->invMassA, mB = joint->invMassB;
  mat3s iA = joint->invIA, iB = joint->invIB;

  mat3s qA = bodyA->orientation;
  mat3s qB = bodyB->orientation;

  // point-to-point constraint setup
  {
    vec3s rA = qA * joint->localAnchorA;
    vec3s rB = qB * joint->localAnchorB;

    // J = [-I -r1_skew]
    //     [I   r2_skew]
    // r_skew = [-ry; rx]
    mat3s S1 = glms_mat3_skew(rA);
    mat3s S2 = glms_mat3_skew(rB);

    mat3s K = mA * (mat3s)GLMS_MAT3_IDENTITY_INIT +
              S1 * iA * glms_mat3_transpose(S1) +
              mB * (mat3s)GLMS_MAT3_IDENTITY_INIT +
              S2 * iB * glms_mat3_transpose(S2);

    joint->invEffectiveMass = glms_mat3_inv(K);
    joint->centerDiff0 = bodyB->position - bodyA->position;

    f32 hz = 15.f;

    const f32 zeta = 1.0f;        // damping
    f32 omega = 2.0f * PI * hz;   // frequency
    joint->biasCoefficient = omega / (2.0f * zeta + h * omega);
    f32 c = h * omega * (2.0f * zeta + h * omega);
    joint->impulseCoefficient = 1.0f / (1.0f + c);
    joint->massCoefficient = c * joint->impulseCoefficient;
  }
}

inline void solveDistanceJoint(distance_joint* joint, f32 h, f32 invH,
                              b32 useBias) {
  rigid_body* bodyA = joint->bodyA;
  rigid_body* bodyB = joint->bodyB;

  // point-to-point constraint
  // This constraint removes three translation degrees of freedom from the system
  if (1) {
    vec3s vA = bodyA->velocity;
    vec3s wA = bodyA->angularVelocity;
    vec3s vB = bodyB->velocity;
    vec3s wB = bodyB->angularVelocity;

    mat3s qA = bodyA->orientation;
    mat3s qB = bodyB->orientation;
    vec3s rA = qA * joint->localAnchorA;
    vec3s rB = qB * joint->localAnchorB;

    vec3s bias = GLMS_VEC3_ZERO_INIT;
    f32 massScale = 1.0f;
    f32 impulseScale = 0.0f;

    // CPos(s) = x2 + r2 + r2 − x1 − r1
    // Time derivate
    // =>
    // CDot = (v2 + ω2 × r2 − v1 − ω1 × r1)
    // Jv = CDot
    vec3s jv = ((vA - glms_vec3_cross(rA, wA)) - vB + glms_vec3_cross(rB, wB));

    if (useBias) {
      vec3s dcA = bodyA->deltaPosition;
      vec3s dcB = bodyB->deltaPosition;

      // Calculate translation bias velocity
      // bias = beta / h * CPos
      vec3s CPos = (dcB - dcA) + (joint->centerDiff0) + rB - rA;

      bias = -joint->biasCoefficient * CPos;
      massScale = joint->massCoefficient;
      impulseScale = joint->impulseCoefficient;
      // bias = -baumgarte * invH * CPos;
    }

    // Calculate lagrange multiplier:
    //
    // lambda = -K^-1 (Jv + b)
    vec3s impulse = massScale * (joint->invEffectiveMass * (jv + bias)) -
                    impulseScale * joint->totalImpulse;
    vec3s newImpulse = joint->totalImpulse + impulse;
    // Store total impulse
    impulse = newImpulse - joint->totalImpulse;
    joint->totalImpulse = newImpulse;

    applyLinearVelocityStep(bodyA, bodyB, joint, impulse);
  }
}

inline void warmStartDistanceJoint(distance_joint* joint) {
  rigid_body* bodyA = joint->bodyA;
  rigid_body* bodyB = joint->bodyB;

  applyLinearVelocityStep(bodyA, bodyB, joint, joint->totalImpulse);
}
