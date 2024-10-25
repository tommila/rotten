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

static void applyLinearVelocityStep(rigid_body* bodyA, rigid_body* bodyB,
                                    fixed_joint* joint, vec3s impulse) {
  mat3s qA = bodyA->orientation;
  mat3s qB = bodyB->orientation;

  vec3s rA = qA * joint->localAnchorA;
  vec3s rB = qB * joint->localAnchorB;

  f32 mA = joint->invMassA, mB = joint->invMassB;
  mat3s iA = joint->invIA, iB = joint->invIB;

  bodyA->velocity = bodyA->velocity - mA * impulse;
  bodyA->angularVelocity =
      bodyA->angularVelocity - iA * glms_vec3_cross(rA, impulse);

  bodyB->velocity = bodyB->velocity + mB * impulse;
  bodyB->angularVelocity =
      bodyB->angularVelocity + iB * glms_vec3_cross(rB, impulse);
}

static void applyAxialVelocityStep(rigid_body* bodyA, rigid_body* bodyB,
                                   fixed_joint* joint, vec3s impulse) {
  mat3s iA = joint->invIA, iB = joint->invIB;

  bodyA->angularVelocity = bodyA->angularVelocity - iA * impulse;
  bodyB->angularVelocity = bodyB->angularVelocity + iB * impulse;
}

inline void setupFixedJoint(fixed_joint* joint, rigid_body* rbA,
                            rigid_body* rbB, vec3s pivotA, vec3s pivotB,
                            vec3s axisInA, vec3s axisInB) {
  joint->bodyA = rbA;
  joint->bodyB = rbB;

  joint->localOriginAnchorA = pivotA;
  joint->localOriginAnchorB = pivotB;

  joint->hingeAxisA = axisInA;
  joint->hingeAxisB = axisInB;
}

inline void preSolveFixedJoint(fixed_joint* joint, f32 h, b32 warmStart) {
  rigid_body* bodyA = joint->bodyA;
  rigid_body* bodyB = joint->bodyB;

  joint->localAnchorA = joint->localOriginAnchorA;
  joint->invMassA = bodyA->invMass;
  joint->invIA = bodyA->invWorlInertiaTensor;

  joint->localAnchorB = joint->localOriginAnchorB;
  joint->invMassB = bodyB->invMass;
  joint->invIB = bodyB->invWorlInertiaTensor;

  f32 mA = joint->invMassA, mB = joint->invMassB;
  mat3s iA = joint->invIA, iB = joint->invIB;

  mat3s qA = bodyA->orientation;
  mat3s qB = bodyB->orientation;

  // Fixed constraint setup
  {
    vec3s rA = qA * joint->localAnchorA;
    vec3s rB = qB * joint->localAnchorB;

    // J = [-I -r1_skew]
    //     [I   r2_skew]
    // r_skew = [-ry; rx]
    mat3s r1 = glms_mat3_skew(rA);
    mat3s r2 = glms_mat3_skew(rB);

    mat3s K = mA * (mat3s)GLMS_MAT3_IDENTITY_INIT +
              r1 * iA * glms_mat3_transpose(r1) +
              mB * (mat3s)GLMS_MAT3_IDENTITY_INIT +
              r2 * iB * glms_mat3_transpose(r2);

    joint->invEffectiveMassTrans = glms_mat3_inv(K);
    joint->invEffectiveMassRot = glms_mat3_inv(bodyA->invWorlInertiaTensor +
                                               bodyB->invWorlInertiaTensor);

    joint->centerDiff0 = bodyB->position - bodyA->position;

    f32 hz = 20.f;

    const f32 zeta = 1.0f;       // damping
    f32 omega = 2.0f * PI * hz;  // frequency
    joint->biasCoefficient = omega / (2.0f * zeta + h * omega);
    f32 c = h * omega * (2.0f * zeta + h * omega);
    joint->impulseCoefficient = 1.0f / (1.0f + c);
    joint->massCoefficient = c * joint->impulseCoefficient;
  }
}

inline void solveFixedJoint(fixed_joint* joint, f32 h, f32 invH, b32 useBias) {
  rigid_body* bodyA = joint->bodyA;
  rigid_body* bodyB = joint->bodyB;

  // Fixed constraint, it has zero degrees of freedom
  if (1) {
    vec3s vA = bodyA->velocity;
    vec3s wA = bodyA->angularVelocity;
    vec3s vB = bodyB->velocity;
    vec3s wB = bodyB->angularVelocity;

    mat3s qA = bodyA->orientation;
    mat3s qB = bodyB->orientation;
    vec3s rA = qA * joint->localAnchorA;
    vec3s rB = qB * joint->localAnchorB;

    vec3s biasTrans = GLMS_VEC3_ZERO_INIT;
    vec3s biasRot = GLMS_VEC3_ZERO_INIT;
    f32 massScale = 1.0f;
    f32 impulseScale = 0.0f;

    // Position constraint
    // CPos(s) = x2 + r2 + r2 − x1 − r1
    // Time derivate
    // =>
    // CDot = (v2 + ω2 × r2 − v1 − ω1 × r1)
    // Jv = CDot
    vec3s jvPos =
        ((vA - glms_vec3_cross(rA, wA)) - vB + glms_vec3_cross(rB, wB));
    vec3s jvRot = wA - wB;
    if (useBias) {
      vec3s dcA = bodyA->deltaPosition;
      vec3s dcB = bodyB->deltaPosition;

      // Calculate translation bias velocity
      // bias = beta / h * CPos
      vec3s CPos = (dcB - dcA) + (joint->centerDiff0) + rB - rA;
      vec3s CRot =
          glms_quat_axis(bodyB->orientationQuat * bodyA->orientationQuat);

      biasTrans = -joint->biasCoefficient * CPos;
      biasRot = -joint->biasCoefficient * CRot;
      massScale = joint->massCoefficient;
      impulseScale = joint->impulseCoefficient;
      // bias = -baumgarte * invH * CPos;
    }

    // Calculate lagrange multiplier:
    //
    // lambda = -K^-1 (Jv + b)
    {
      vec3s impulse =
          massScale * (joint->invEffectiveMassTrans * (jvPos + biasTrans)) -
          impulseScale * joint->totalImpulseTrans;
      vec3s newImpulse = joint->totalImpulseTrans + impulse;
      // Store total impulse
      impulse = newImpulse - joint->totalImpulseTrans;
      joint->totalImpulseTrans = newImpulse;

      applyLinearVelocityStep(bodyA, bodyB, joint, impulse);
    }
    {
      vec3s impulse =
          massScale * (joint->invEffectiveMassTrans * (jvRot + biasRot)) -
          impulseScale * joint->totalImpulseTrans;
      vec3s newImpulse = joint->totalImpulseRot + impulse;
      // Store total impulse
      impulse = newImpulse - joint->totalImpulseRot;
      joint->totalImpulseRot = newImpulse;

      applyAxialVelocityStep(bodyA, bodyB, joint, impulse);
    }
  }
}

inline void warmStartFixedJoint(fixed_joint* joint) {
  rigid_body* bodyA = joint->bodyA;
  rigid_body* bodyB = joint->bodyB;

  applyLinearVelocityStep(bodyA, bodyB, joint, joint->totalImpulseTrans);
  applyAxialVelocityStep(bodyA, bodyB, joint, joint->totalImpulseRot);
}
