// Constraint derivations from
// Constraints Derivation for Rigid Body Simulation in 3D by Daniel Chappuis

// The distance joint only allows arbitrary rotation between two bodies but no translation.
// It has three degrees of freedom.
static void applyLinearVelocityStep(rigid_body* bodyA, rigid_body* bodyB, distance_joint* joint,
                              vec3s impulse) {
  mat3s qA = bodyA->orientation;
  mat3s qB = bodyB->orientation;

  vec3s rA = qA * joint->base.localAnchorA;
  vec3s rB = qB * joint->base.localAnchorB;

  f32 mA = joint->base.invMassA, mB = joint->base.invMassB;
  mat3s iA = joint->base.invIA, iB = joint->base.invIB;

  bodyA->velocity = bodyA->velocity - mA * impulse;
  bodyA->angularVelocity = bodyA->angularVelocity - iA * glms_vec3_cross(rA, impulse);

  bodyB->velocity = bodyB->velocity + mB * impulse;
  bodyB->angularVelocity = bodyB->angularVelocity + iB * glms_vec3_cross(rB, impulse);
}

inline void setupDistanceJoint(joint* j, rigid_body* rbA, rigid_body* rbB,
			       vec3s pivotA, vec3s pivotB, vec3s axisInA, vec3s axisInB,
			       f32 herz, f32 damping) {
  j->type = joint_type_distance;
  j->distance.base.herz = herz;
  j->distance.base.damping = damping;
  j->distance.base.bodyA = rbA;
  j->distance.base.bodyB = rbB;

  j->distance.base.localOriginAnchorA = pivotA;
  j->distance.base.localOriginAnchorB = pivotB;
}

inline void preSolveDistanceJoint(distance_joint* joint, f32 h) {
  rigid_body* bodyA = joint->base.bodyA;
  rigid_body* bodyB = joint->base.bodyB;

  joint->base.localAnchorA = joint->base.localOriginAnchorA;
  joint->base.localAnchorB = joint->base.localOriginAnchorB;

  joint->base.invMassB = bodyB->invMass;
  joint->base.invMassA = bodyA->invMass;

  joint->base.invIB = bodyB->invWorlInertiaTensor;
  joint->base.invIA = bodyA->invWorlInertiaTensor;

  f32 mInvA = joint->base.invMassA, mInvB = joint->base.invMassB;
  mat3s iA = joint->base.invIA, iB = joint->base.invIB;

  mat3s qA = bodyA->orientation;
  mat3s qB = bodyB->orientation;

  vec3s rA = qA * joint->base.localAnchorA;
  vec3s rB = qB * joint->base.localAnchorB;
  // constraint setup
  {
    mat3s S1 = glms_mat3_skew(rA);
    mat3s S2 = glms_mat3_skew(rB);

    mat3s K =
      (mat3s)GLMS_MAT3_IDENTITY_INIT * mInvA +
      S1 * iA * glms_mat3_transpose(S1) +
      (mat3s)GLMS_MAT3_IDENTITY_INIT * mInvB +
      S2 * iB * glms_mat3_transpose(S2);

    joint->invEffectiveMass = glms_mat3_inv(K);
    joint->centerDiff0 = bodyB->position - bodyA->position;

    const f32 zeta = joint->base.damping;      // damping
    f32 omega = 2.0f * PI * joint->base.herz;  // frequency
    joint->base.biasCoefficient = omega / (2.0f * zeta + h * omega);
    f32 c = h * omega * (2.0f * zeta + h * omega);
    joint->base.impulseCoefficient = 1.0f / (1.0f + c);
    joint->base.massCoefficient = c * joint->base.impulseCoefficient;
  }
}

inline void solveDistanceJoint(distance_joint* joint, f32 h, f32 invH,
                              b32 useBias) {
  rigid_body* bodyA = joint->base.bodyA;
  rigid_body* bodyB = joint->base.bodyB;

  // point-to-point constraint
  // This constraint removes three translation degrees of freedom from the system
  if (1) {
    vec3s vA = bodyA->velocity;
    vec3s wA = bodyA->angularVelocity;
    vec3s vB = bodyB->velocity;
    vec3s wB = bodyB->angularVelocity;

    mat3s qA = bodyA->orientation;
    mat3s qB = bodyB->orientation;
    vec3s rA = qA * joint->base.localAnchorA;
    vec3s rB = qB * joint->base.localAnchorB;

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
      vec3s CPos = (dcB - dcA) + joint->centerDiff0 + rB - rA;

      bias = -joint->base.biasCoefficient * CPos;
      massScale = joint->base.massCoefficient;
      impulseScale = joint->base.impulseCoefficient;
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
  rigid_body* bodyA = joint->base.bodyA;
  rigid_body* bodyB = joint->base.bodyB;

  applyLinearVelocityStep(bodyA, bodyB, joint, joint->totalImpulse);
}
