// Constraint derivations from
// Constraints Derivation for Rigid Body Simulation in 3D by Daniel Chappuis

// A hinge joint only allows relative rotation between two bodies around a single axis. It has one
// degree of freed - Chapter 2.4

static void applyAxialVelocityStep(rigid_body* bodyA, rigid_body* bodyB, hinge_joint* joint,
                              vec3s impulse) {
  mat3s iA = joint->base.invIA, iB = joint->base.invIB;

  bodyA->angularVelocity = bodyA->angularVelocity - iA * impulse;
  bodyB->angularVelocity = bodyB->angularVelocity + iB * impulse;
}

inline void setupHingeJoint(joint* j, rigid_body* rbA, rigid_body* rbB,
			    vec3s pivotA, vec3s pivotB, vec3s axisIn,
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

inline vec3s fixA2(vec3s a1, vec3s a2) {
  float dot = glms_vec3_dot(a1, a2);
  if (dot <= 1.0e-3f) {
    // World space axes are more than 90 degrees apart, get a perpendicular
    // vector in the plane formed by a1 and a2 as hinge axis until the rotation
    // is less than 90 degrees
    vec3s perp = a2 - dot * a1;
    if (glms_vec3_norm2(perp) < 1.0e-6f) {
      // a1 ~ -a2, take random perpendicular
      perp = glms_vec3_norm_perpendicular(a1);
    }

    // Blend in a little bit from a1 so we're less than 90 degrees apart
    a2 = glms_vec3_normalize(0.99f * glms_vec3_normalize(perp) + 0.01f * a1);
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

  joint->base.invIB = bodyB->invWorlInertiaTensor;
  joint->base.invIA = bodyA->invWorlInertiaTensor;

  // Hinge constraint setup
  mat3s iA = joint->base.invIA, iB = joint->base.invIB;
  mat3s qA = glms_quat_mat3(bodyA->orientationQuat);
  mat3s qB = glms_quat_mat3(bodyB->orientationQuat);

  vec3s A1 =
      glms_vec3_rotate(joint->hingeAxis * qA, joint->localAxisARotation);
  vec3s A2 =
      glms_vec3_rotate(joint->hingeAxis * qB, joint->localAxisBRotation);

  A2 = fixA2(A1, A2);

  vec3s B2 = glms_vec3_norm_perpendicular(A2);
  vec3s C2 = glms_vec3_cross(A2, B2);

  vec3s B2xA1 = glms_vec3_cross(B2, A1);
  vec3s C2xA1 = glms_vec3_cross(C2, A1);
  mat3s sumInvI = iA + iB;

  // Calculate effective mass: K^-1 = (J M^-1 J^T)^-1
  mat2s effectiveMass = {
    glms_vec3_dot(B2xA1, sumInvI * B2xA1),
    glms_vec3_dot(B2xA1, sumInvI * C2xA1),
    glms_vec3_dot(C2xA1, sumInvI * B2xA1),
    glms_vec3_dot(C2xA1, sumInvI * C2xA1)
  };

  joint->invEffectiveMass = glms_mat2_inv(effectiveMass);

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
  vec3s wA = bodyA->angularVelocity;
  vec3s wB = bodyB->angularVelocity;
  mat3s qA = glms_quat_mat3(bodyA->orientationQuat);
  mat3s qB = glms_quat_mat3(bodyB->orientationQuat);

  vec3s wD = wA - wB;

  // At the beginning of the
  // simulation, when the user specifies the world-space hinge axis vector a, we
  // convert this vector into two local-space vectors a1l and a2l in each
  // local-space of the two bodies of the joint. Then, at each frame, we convert
  // those two vectors back in world-space. So we get the two unit length
  // vectors a1 and a2 in world-space coordinates. Then, we compute two unit
  // orthogonal vectors b2 and c2 that are orthogonal to the vector a2.

  vec3s A1 =
      glms_vec3_rotate(joint->hingeAxis * qA, joint->localAxisARotation);
  vec3s A2 =
      glms_vec3_rotate(joint->hingeAxis * qB, joint->localAxisBRotation);
  A2 = fixA2(A1, A2);

  vec3s B2 = glms_vec3_norm_perpendicular(A2);
  vec3s C2 = glms_vec3_cross(A2, B2);

  vec3s B2xA1 = glms_vec3_cross(B2, A1);
  vec3s C2xA1 = glms_vec3_cross(C2, A1);
  vec2s jRot = GLMS_VEC2_ZERO_INIT;
  // CRot(s) = (a1 · b2
  //            a1 · c2)
  // Time derivate
  // =>
  // CDot = (−(b2 × a1) · (w2 × w1),
  //         −(c2 × a1) · (w2 × w1))
  // Jv = CDot
  jRot.raw[1] = glms_vec3_dot(C2xA1, wD);
  jRot.raw[0] = glms_vec3_dot(B2xA1, wD);

  vec2s bias = GLMS_VEC2_ZERO_INIT;
  f32 massScale = 1.0f;
  f32 impulseScale = 0.0f;
  if (useBias) {
    // Calculate rotation bias velocity
    // Crot(s) = (a1 · b2, a1 · c2)
    vec2s CRot;
    CRot.raw[0] = glms_vec3_dot(A1, B2);
    CRot.raw[1] = glms_vec3_dot(A1, C2);
    bias = -joint->base.biasCoefficient * CRot;

    massScale = joint->base.massCoefficient;
    impulseScale = joint->base.impulseCoefficient;
    // bias = -baumgarte * invH * CPos;
  }

  // Calculate lagrange multiplier:
  //
  // lambda = -K^-1 (Jv + b)
  vec2s lambda = massScale * (joint->invEffectiveMass * (jRot + bias)) -
                 impulseScale * joint->totalLambda;
  // vec2s lambda = massScale * (joint->invEffectiveMass * (bias)) -
  //   impulseScale * joint->totalLambda;

  vec2s newLambda = joint->totalLambda + lambda;

  // Store total impulse
  lambda = newLambda - joint->totalLambda;
  joint->totalLambda = newLambda;
  vec3s impulse = B2xA1 * lambda.raw[0] + C2xA1 * lambda.raw[1];

  applyAxialVelocityStep(bodyA, bodyB, joint, impulse);

  // Motor
  if (fabsf(joint->motorTorque) > 0.1f) {
    f32 K =
        glms_vec3_dot(A1,
                      A1 * joint->base.bodyA->invWorlInertiaTensor) +
        glms_vec3_dot(A1,
                      A1 * joint->base.bodyB->invWorlInertiaTensor);

    f32 cdot = glms_vec3_dot(A1, wB - wA);
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

  mat3s qA = glms_quat_mat3(bodyA->orientationQuat);
  mat3s qB = glms_quat_mat3(bodyB->orientationQuat);

  vec3s A1 =
      glms_vec3_rotate(joint->hingeAxis * qA, joint->localAxisARotation);
  vec3s A2 =
      glms_vec3_rotate(joint->hingeAxis * qB, joint->localAxisBRotation);
  A2 = fixA2(A1, A2);

  vec3s B2 = glms_vec3_norm_perpendicular(A2);
  vec3s C2 = glms_vec3_cross(A2, B2);

  vec3s B2xA1 = glms_vec3_cross(B2, A1);
  vec3s C2xA1 = glms_vec3_cross(C2, A1);

  vec3s axialImpulse =
      B2xA1 * joint->totalLambda.raw[0] + C2xA1 * joint->totalLambda.raw[1];

  if (fabsf(joint->motorTorque) > 0.1f) {
    axialImpulse = axialImpulse + A1 * joint->motorImpulse;
  }

  applyAxialVelocityStep(bodyA, bodyB, joint, axialImpulse);
}
