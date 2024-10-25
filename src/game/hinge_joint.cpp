#include "all.h"
// Constraint derivations from
// Constraints Derivation for Rigid Body Simulation in 3D by Daniel Chappuis

// A hinge joint only allows relative rotation between two bodies around a single axis. It has one
// degree of freed - Chapter 2.4

static void applyAxialVelocityStep(rigid_body* bodyA, rigid_body* bodyB, hinge_joint* joint,
                              v3 impulse) {
  m3x3 iA = bodyA->invWorlInertiaTensor;
  m3x3 iB = bodyB->invWorlInertiaTensor;

  v3 wA = iA * impulse;
  v3 wB = iB * impulse;

  bodyA->angularVelocity = bodyA->angularVelocity - wA;
  bodyB->angularVelocity = bodyB->angularVelocity + wB;
}

inline void setupHingeJoint(joint* j, rigid_body* rbA, rigid_body* rbB,
			    v3 pivotA, v3 pivotB, v3 axisIn,
			    f32 herz, f32 damping) {
  j->type = joint_type_hinge;
  j->distance.base.herz = herz;
  j->distance.base.damping = damping;
  j->hinge.base.bodyA = rbA;
  j->hinge.base.bodyB = rbB;

  j->hinge.base.localOriginAnchorA = pivotA;
  j->hinge.base.localOriginAnchorB = pivotB;

  j->hinge.hingeAxis = axisIn;
}

inline v3 fixA2(v3 a1, v3 a2) {
  float d = v3_dot(a1, a2);
  if (d <= 1.0e-3f) {
    // World space axes are more than 90 degrees apart, get a perpendicular
    // vector in the plane formed by a1 and a2 as hinge axis until the rotation
    // is less than 90 degrees
    v3 perp = a2 - d * a1;
    if (v3_length2(perp) < 1.0e-6f) {
      // a1 ~ -a2, take random perpendicular
      perp = perpendicular(a1);
    }

    // Blend in a little bit from a1 so we're less than 90 degrees apart
    a2 = v3_normalize(0.99f * v3_normalize(perp) + 0.01f * a1);
  }
  return a2;
}

inline void preSolveHingeJoint(hinge_joint* joint, f32 h) {
  rigid_body* bodyA = joint->base.bodyA;
  rigid_body* bodyB = joint->base.bodyB;

  joint->base.localAnchorA = joint->base.localOriginAnchorA;
  joint->base.localAnchorB = joint->base.localOriginAnchorB;

  joint->base.invMassB = bodyB->invMass;
  joint->base.invMassA = bodyA->invMass;

  joint->base.invIA = bodyA->invWorlInertiaTensor; 
  joint->base.invIB = bodyB->invWorlInertiaTensor; 
  
  v3 hingeAxis = {joint->hingeAxis.x, joint->hingeAxis.y, joint->hingeAxis.z};
  v4 locAxisARot = { joint->localAxisARotation.x, joint->localAxisARotation.y, joint->localAxisARotation.z, joint->localAxisARotation.w};
  v4 locAxisBRot = { joint->localAxisBRotation.x, joint->localAxisBRotation.y, joint->localAxisBRotation.z, joint->localAxisBRotation.w};
  v3 glmA1 =
      v3_rotate_axis_angle(hingeAxis * bodyA->orientation, locAxisARot.w, locAxisARot.xyz);
  v3 glmA2 =
      v3_rotate_axis_angle(hingeAxis * bodyB->orientation, locAxisBRot.w, locAxisARot.xyz);

  v3 A1 = {glmA1.x, glmA1.y, glmA1.z};
  v3 A2 = fixA2(A1, {glmA2.x, glmA2.y, glmA2.z});
  
  v3 B2 = perpendicular(A2);
  v3 C2 = v3_cross(A2, B2);

  v3 B2xA1 = v3_cross(B2, A1);
  v3 C2xA1 = v3_cross(C2, A1);
  m3x3 sumInvI = joint->base.invIA + joint->base.invIB;

  // Calculate effective mass: K^-1 = (J M^-1 J^T)^-1
  m2x2 effectiveMass = {
    v3_dot(B2xA1, sumInvI * B2xA1),
    v3_dot(B2xA1, sumInvI * C2xA1),
    v3_dot(C2xA1, sumInvI * B2xA1),
    v3_dot(C2xA1, sumInvI * C2xA1)
  };

  joint->invEffectiveMass = m2x2_inverse(effectiveMass);

  calculateSoftConstraintProperties(h, joint->base.herz,
				    joint->base.damping,
				    &joint->base.biasCoefficient,
				    &joint->base.impulseCoefficient,
				    &joint->base.massCoefficient);
}

inline void solveHingeJoint(hinge_joint* joint, f32 h, f32 invH, b32 useBias) {
  rigid_body* bodyA = joint->base.bodyA;
  rigid_body* bodyB = joint->base.bodyB;

  // Hinge constraint
  // This constraint removes two rotational degrees of freedom from the system.
  v3 wA = bodyA->angularVelocity;
  v3 wB = bodyB->angularVelocity;

  m3x3 qA = bodyA->orientation;
  m3x3 qB = bodyB->orientation;

  v3 wD = wA - wB;

  // At the beginning of the
  // simulation, when the user specifies the world-space hinge axis vector a, we
  // convert this vector into two local-space vectors a1l and a2l in each
  // local-space of the two bodies of the joint. Then, at each frame, we convert
  // those two vectors back in world-space. So we get the two unit length
  // vectors a1 and a2 in world-space coordinates. Then, we compute two unit
  // orthogonal vectors b2 and c2 that are orthogonal to the vector a2.
  v3 glmA1 =
      v3_rotate_axis_angle(joint->hingeAxis * qA, joint->localAxisARotation.w, joint->localAxisARotation.xyz);
  v3 glmA2 =
      v3_rotate_axis_angle(joint->hingeAxis * qB, joint->localAxisBRotation.w, joint->localAxisBRotation.xyz);

  v3 A1 = {glmA1.x, glmA1.y, glmA1.z};
  v3 A2 = fixA2(A1, {glmA2.x, glmA2.y, glmA2.z});
  v3 B2 = perpendicular(A2);
  v3 C2 = v3_cross(A2, B2);

  v3 B2xA1 = v3_cross(B2, A1);
  v3 C2xA1 = v3_cross(C2, A1);
  v2 jRot = V2_ZERO;
  // CRot(s) = (a1 · b2
  //            a1 · c2)
  // Time derivate
  // =>
  // CDot = (−(b2 × a1) · (w2 × w1),
  //         −(c2 × a1) · (w2 × w1))
  // Jv = CDot
  jRot.arr[1] = v3_dot(C2xA1, wD);
  jRot.arr[0] = v3_dot(B2xA1, wD);

  v2 bias = V2_ZERO;
  f32 massScale = 1.0f;
  f32 impulseScale = 0.0f;
  if (useBias) {
    // Calculate rotation bias velocity
    // Crot(s) = (a1 · b2, a1 · c2)
    v2 CRot;
    CRot.arr[0] = v3_dot(A1, B2);
    CRot.arr[1] = v3_dot(A1, C2);
    bias = -joint->base.biasCoefficient * CRot;

    massScale = joint->base.massCoefficient;
    impulseScale = joint->base.impulseCoefficient;
    // bias = -baumgarte * invH * CPos;
  }

  // Calculate lagrange multiplier:
  //
  // lambda = -K^-1 (Jv + b)
  m2x2 invEffectiveMass = joint->invEffectiveMass; 
  v2 lambda = massScale * (invEffectiveMass * (jRot + bias)) -
                 impulseScale * joint->totalLambda;
  // v2 lambda = massScale * (joint->invEffectiveMass * (bias)) -
  //   impulseScale * joint->totalLambda;

  v2 newLambda = joint->totalLambda + lambda;

  // Store total impulse
  lambda = newLambda - joint->totalLambda;
  joint->totalLambda = newLambda;
  v3 impulse = B2xA1 * lambda.arr[0] + C2xA1 * lambda.arr[1];

  applyAxialVelocityStep(bodyA, bodyB, joint, impulse);

  // Motor
  m3x3 invWorldInertiaTensorA = joint->base.bodyA->invWorlInertiaTensor;
  m3x3 invWorldInertiaTensorB = joint->base.bodyB->invWorlInertiaTensor;

  if (fabsf(joint->motorTorque) > 0.1f) {
    f32 K =
      v3_dot(A1, A1 * invWorldInertiaTensorA) +
      v3_dot(A1, A1 * invWorldInertiaTensorB);

    f32 cdot = v3_dot(A1, wB - wA);
    f32 impulse = -K * ((joint->motorTorque < 0.f && cdot > 0.f)
                            ? cdot
                            : -joint->motorTorque * h);

    f32 oldImpulse = joint->motorImpulse;
    f32 maxImpulse = fabsf(h * joint->motorTorque);
    joint->motorImpulse =
        CLAMP(joint->motorImpulse + impulse, -maxImpulse, maxImpulse);
    impulse = joint->motorImpulse - oldImpulse;

    applyAxialVelocityStep(bodyA, bodyB, joint, A1 * impulse);
  }
}

inline void warmStartHingeJoint(hinge_joint* joint) {
  rigid_body* bodyA = joint->base.bodyA;
  rigid_body* bodyB = joint->base.bodyB;

  m3x3 qA = bodyA->orientation;
  m3x3 qB = bodyB->orientation;

  v3 glmA1 =
      v3_rotate_axis_angle(joint->hingeAxis * qA, joint->localAxisARotation.w, joint->localAxisARotation.xyz);
  v3 glmA2 =
      v3_rotate_axis_angle(joint->hingeAxis * qB, joint->localAxisBRotation.w, joint->localAxisBRotation.xyz);

  v3 A1 = {glmA1.x, glmA1.y, glmA1.z};
  v3 A2 = fixA2(A1, {glmA2.x, glmA2.y, glmA2.z});
  v3 B2 = perpendicular(A2);
  v3 C2 = v3_cross(A2, B2);

  v3 B2xA1 = v3_cross(B2, A1);
  v3 C2xA1 = v3_cross(C2, A1);

  v3 axialImpulse =
      B2xA1 * joint->totalLambda.arr[0] + C2xA1 * joint->totalLambda.arr[1];

  if (fabsf(joint->motorTorque) > 0.1f) {
    axialImpulse = axialImpulse + A1 * joint->motorImpulse;
  }

  applyAxialVelocityStep(bodyA, bodyB, joint, axialImpulse);
}
