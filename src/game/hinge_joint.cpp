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

static void applyAxialVelocityStep(rigid_body* bodyA, rigid_body* bodyB, hinge_joint* joint,
                              vec3s impulse) {
  mat3s iA = joint->invIA, iB = joint->invIB;

  bodyA->angularVelocity = bodyA->angularVelocity - iA * impulse;
  bodyB->angularVelocity = bodyB->angularVelocity + iB * impulse;
}

inline void setupHingeJoint(hinge_joint* joint, rigid_body* rbA, rigid_body* rbB,
                       vec3s pivotA, vec3s pivotB, vec3s axisInA, vec3s axisInB) {
  joint->motorEnabled = false;
  joint->bodyA = rbA;
  joint->bodyB = rbB;

  joint->localOriginAnchorA = pivotA;
  joint->localOriginAnchorB = pivotB;

  joint->hingeAxisA = axisInA;
  joint->hingeAxisB = axisInB;
}

inline void preSolveHingeJoint(hinge_joint* joint, f32 h, b32 warmStart) {
  rigid_body* bodyA = joint->bodyA;
  rigid_body* bodyB = joint->bodyB;

  joint->localAnchorA = joint->localOriginAnchorA;
  joint->localAnchorB = joint->localOriginAnchorB;

  joint->invMassB = bodyB->invMass;
  joint->invMassA = bodyA->invMass;

  joint->invIB = bodyB->invWorlInertiaTensor;
  joint->invIA = bodyA->invWorlInertiaTensor;

  mat3s iA = joint->invIA, iB = joint->invIB;

  mat3s qA = bodyA->orientation;
  mat3s qB = bodyB->orientation;

  // Hinge constraint setup
  {
    // Calculate hinge axis in world space
    joint->mA1 = glms_vec3_rotate(joint->hingeAxisA * qA, joint->localAxisARotation);
    vec3s a2 = glms_vec3_rotate(joint->hingeAxisB * qB, joint->localAxisBRotation);
    float dot = glms_vec3_dot(joint->mA1, a2);
    if (dot <= 1.0e-3f) {
      // World space axes are more than 90 degrees apart, get a perpendicular
      // vector in the plane formed by mA1 and a2 as hinge axis until the rotation
      // is less than 90 degrees
      vec3s perp = a2 - dot * joint->mA1;
      if (glms_vec3_norm2(perp) < 1.0e-6f) {
        // mA1 ~ -a2, take random perpendicular
        perp = glms_vec3_norm_perpendicular(joint->mA1);
      }

      // Blend in a little bit from mA1 so we're less than 90 degrees apart
      a2 = glms_vec3_normalize(0.99f * glms_vec3_normalize(perp) + 0.01f * joint->mA1);
    }
    joint->mB2 = glms_vec3_norm_perpendicular(a2);
    joint->mC2 = glms_vec3_cross(a2, joint->mB2);

    joint->mB2xA1 = glms_vec3_cross(joint->mB2, joint->mA1);
    joint->mC2xA1 = glms_vec3_cross(joint->mC2, joint->mA1);
    mat3s sumInvI = iA + iB;

    // Calculate effective mass: K^-1 = (J M^-1 J^T)^-1
    mat2s effectiveMass = GLMS_MAT2_ZERO_INIT;
    effectiveMass.m00 = glms_vec3_dot(joint->mB2xA1, sumInvI * joint->mB2xA1);
    effectiveMass.m01 = glms_vec3_dot(joint->mB2xA1, sumInvI * joint->mC2xA1);
    effectiveMass.m10 = glms_vec3_dot(joint->mC2xA1, sumInvI * joint->mB2xA1);
    effectiveMass.m11 = glms_vec3_dot(joint->mC2xA1, sumInvI * joint->mC2xA1);

    joint->invEffectiveMass = glms_mat2_inv(effectiveMass);

    const f32 zeta = 1.0f;        // damping
    f32 omega = 2.0f * PI * 45.f; // frequency
    joint->biasCoefficient = omega / (2.0f * zeta + h * omega);
    f32 c = h * omega * (2.0f * zeta + h * omega);
    joint->impulseCoefficient = 1.0f / (1.0f + c);
    joint->massCoefficient = c * joint->impulseCoefficient;
  }
}

inline void solveHingeJoint(hinge_joint* joint, f32 h, f32 invH,
			    b32 useBias) {
  rigid_body* bodyA = joint->bodyA;
  rigid_body* bodyB = joint->bodyB;

  // Hinge constraint
  // This constraint removes three translation
  // and also two rotational degrees of freedom from the system.
  if (1) {
    vec3s wA = bodyA->angularVelocity;
    vec3s wB = bodyB->angularVelocity;

    vec3s wD = wA - wB;
    vec2s jv = GLMS_VEC2_ZERO_INIT;
    // CPos(s) = x2 + r2 + r2 − x1 − r1
    // Time derivate
    // =>
    // CDot = (−(b2 × a1) · (w2 × w1), −(c2 × a1) · (w2 × w1))
    // Jv = CDot
    jv.raw[1] = glms_vec3_dot(joint->mC2xA1, wD);
    jv.raw[0] = glms_vec3_dot(joint->mB2xA1, wD);

    vec2s bias = GLMS_VEC2_ZERO_INIT;
    f32 massScale = 1.0f;
    f32 impulseScale = 0.0f;
    if (useBias) {
      // Calculate rotation bias velocity
      // Crot(s) = (a1 · b2, a1 · c2)
      vec2s CRot;
      CRot.raw[0] = glms_vec3_dot(joint->mA1, joint->mB2);
      CRot.raw[1] = glms_vec3_dot(joint->mA1, joint->mC2);
      bias = -joint->biasCoefficient * CRot;

      massScale = joint->massCoefficient;
      impulseScale = joint->impulseCoefficient;
      // bias = -baumgarte * invH * CPos;
    }

    // Calculate lagrange multiplier:
    //
    // lambda = -K^-1 (Jv + b)
    vec2s lambda = massScale * (joint->invEffectiveMass * (jv + bias)) -
      impulseScale * joint->totalLambda;

    vec2s newLambda = joint->totalLambda + lambda;

    // Store total impulse
    lambda = newLambda - joint->totalLambda;
    joint->totalLambda = newLambda;
    vec3s impulse =
      joint->mB2xA1 * lambda.raw[0] +
      joint->mC2xA1 * lambda.raw[1];

    applyAxialVelocityStep(bodyA, bodyB, joint, impulse);
  }

  // Motor
  if (joint->motorEnabled) {
    vec3s wA = bodyA->angularVelocity;
    vec3s wB = bodyB->angularVelocity;

    f32 Cdot = glms_vec3_dot(wB - wA, joint->mA1);
    if (joint->breaking) {
      f32 longitudinalSpeed = glms_vec3_dot(GLMS_XUP * bodyA->orientation, bodyB->velocity);
      if (SIGNF(longitudinalSpeed) < 0.f) {
	Cdot = joint->motorSpeed - Cdot;
      }
    }
    else {
      Cdot -= joint->motorSpeed;
    }
    // Kmotor = Kmin = a^T * I1⁻¹a + a^T * I2⁻¹
    f32 K = glms_vec3_dot(joint->mA1, joint->mA1 * joint->bodyA->invWorlInertiaTensor) +
      glms_vec3_dot(joint->mA1, joint->mA1 * joint->bodyB->invWorlInertiaTensor);
    f32 lambda = -K * Cdot;

    f32 maxImpulse = fabsf(h * joint->motorMaxTorque);
    joint->motorImpulse =
        CLAMP(joint->motorImpulse + lambda, -maxImpulse, maxImpulse);

    vec3s impulse = joint->mA1 * joint->motorImpulse;

    applyAxialVelocityStep(bodyA, bodyB, joint, impulse);
  }
}

inline void warmStartHingeJoint(hinge_joint* joint) {
  rigid_body* bodyA = joint->bodyA;
  rigid_body* bodyB = joint->bodyB;

  vec3s axialImpulse =
    joint->mB2xA1 * joint->totalLambda.raw[0] +
    joint->mC2xA1 * joint->totalLambda.raw[1];

  if (joint->motorEnabled) {
    axialImpulse = axialImpulse + joint->mA1 * joint->motorImpulse;
  }

  applyAxialVelocityStep(bodyA, bodyB, joint, axialImpulse);
}
