// Constraint derivations from
// Constraints Derivation for Rigid Body Simulation in 3D by Daniel Chappuis

// A slider joint only allows relative translation between two bodies in a single direction. It has
// only one degree of freedom - Chapter 2.3
static void applySliderLinearVelocityStep(rigid_body* bodyA, rigid_body* bodyB, slider_joint* joint,
					  vec2s lambda,
					  vec3s nA, vec3s nB,
					  vec3s iA_rAPlusUCrossNA, vec3s iA_rAPlusUCrossNB,
					  vec3s iB_rBCrossNA, vec3s iB_rBCrossNB) {
  f32 mA = joint->base.invMassA, mB = joint->base.invMassB;

  vec3s impulse = nA * lambda.x + nB * lambda.y;
  {
    vec3s V = mA * impulse;
    vec3s W = iA_rAPlusUCrossNA * lambda.x + iA_rAPlusUCrossNB * lambda.y;

    bodyA->velocity = bodyA->velocity - V;
    bodyA->angularVelocity = bodyA->angularVelocity - W;
  }
  {
    vec3s V = mB * impulse;
    vec3s W = iB_rBCrossNA * lambda.x + iB_rBCrossNB * lambda.y;

    bodyB->velocity = bodyB->velocity + V;
    bodyB->angularVelocity = bodyB->angularVelocity + W;
  }
}

inline void setupSliderJoint(joint* j, rigid_body* rbA, rigid_body* rbB,
			     vec3s pivotA, vec3s pivotB, vec3s axisIn,
			     f32 herz, f32 damping) {
  j->type = joint_type_slider;
  j->slider.base.herz = herz;
  j->slider.base.damping = damping;
  j->slider.base.bodyA = rbA;
  j->slider.base.bodyB = rbB;

  j->slider.base.localOriginAnchorA = pivotA;
  j->slider.base.localOriginAnchorB = pivotB;
  j->slider.slideAxis = axisIn;
  j->slider.rangeMin = -0.25f;
  j->slider.rangeMax = 0.f;
}

inline void preSolveSliderJoint(slider_joint* joint, f32 h) {
  rigid_body* bodyA = joint->base.bodyA;
  rigid_body* bodyB = joint->base.bodyB;

  joint->base.localAnchorA = joint->base.localOriginAnchorA;
  joint->base.localAnchorB = joint->base.localOriginAnchorB;

  joint->base.invMassB = bodyB->invMass;
  joint->base.invMassA = bodyA->invMass;

  joint->base.invIB = bodyB->invWorlInertiaTensor;
  joint->base.invIA = bodyA->invWorlInertiaTensor;

  f32 invMA = joint->base.invMassA, invMB = joint->base.invMassB;
  mat3s iA = joint->base.invIA, iB = joint->base.invIB;

  mat3s qA = bodyA->orientation;
  mat3s qB = bodyB->orientation;

  vec3s rA = qA * joint->base.localAnchorA;
  vec3s rB = qB * joint->base.localAnchorB;

  vec3s aW = qA * joint->slideAxis;
  vec3s nA = glms_vec3_norm_perpendicular(aW);
  vec3s nB = glms_vec3_cross(nA, aW);

  vec3s dcA = bodyA->deltaPosition;
  vec3s dcB = bodyB->deltaPosition;
  vec3s centerDiff = bodyB->position - bodyA->position;

  vec3s u = (dcB - dcA) + centerDiff + rB - rA;

  vec3s rAPlusU = rA + u;
  vec3s rAPlusUCrossNA = glms_vec3_cross(rAPlusU, nA);
  vec3s rAPlusUCrossNB = glms_vec3_cross(rAPlusU, nB);

  vec3s rBCrossNA = glms_vec3_cross(rB, nA);
  vec3s rBCrossNB = glms_vec3_cross(rB, nB);

  vec3s iA_rAPlusUCrossNA = iA * rAPlusUCrossNA;
  vec3s iA_rAPlusUCrossNB = iA * rAPlusUCrossNB;

  vec3s iB_rBCrossNA = iB * rBCrossNA;
  vec3s iB_rBCrossNB = iB * rBCrossNB;

  f32 invMass = (invMA + invMB);

  // Constraint mass matrix
  // (eq 59)
  mat2s K = {invMass + glms_vec3_dot(rAPlusUCrossNA, iA_rAPlusUCrossNA) +
                 glms_vec3_dot(rBCrossNA, iB_rBCrossNA),
             glms_vec3_dot(rAPlusUCrossNA, iA_rAPlusUCrossNB) +
                 glms_vec3_dot(rBCrossNA, iB_rBCrossNB),
             glms_vec3_dot(rAPlusUCrossNB, iA_rAPlusUCrossNA) +
                 glms_vec3_dot(rBCrossNB, iB_rBCrossNA),
             invMass + glms_vec3_dot(rAPlusUCrossNB, iA_rAPlusUCrossNB) +
                 glms_vec3_dot(rBCrossNB, iB_rBCrossNB)};

  joint->invEffectiveMass = glms_mat2_inv(K);
  joint->centerDiff0 = centerDiff;

  calculateSoftConstraintProperties(
      h, joint->base.herz, joint->base.damping, &joint->base.biasCoefficient,
      &joint->base.impulseCoefficient, &joint->base.massCoefficient);
}

inline void solveSliderJoint(slider_joint* joint, f32 h, f32 invH,
                             b32 useBias) {
  rigid_body* bodyA = joint->base.bodyA;
  rigid_body* bodyB = joint->base.bodyB;

  vec3s vA = bodyA->velocity;
  vec3s wA = bodyA->angularVelocity;
  vec3s vB = bodyB->velocity;
  vec3s wB = bodyB->angularVelocity;

  mat3s iA = joint->base.invIA, iB = joint->base.invIB;

  {
    vec2s bias = GLMS_VEC2_ZERO_INIT;
    f32 massScale = 1.0f;
    f32 impulseScale = 0.0f;

    vec3s deltaV = vB - vA;

    mat3s qA = bodyA->orientation;
    mat3s qB = bodyB->orientation;

    vec3s rA = qA * joint->base.localAnchorA;
    vec3s rB = qB * joint->base.localAnchorB;

    // The two vectors n1 and n2 are two unit orthogonal vectors that are
    // orthogonal to the slider axis a. At the joint creation, we convert the
    // slider axis a into the local-space of body B1 and we get the vector al .
    // Then, at each frame, we convert the vector aL back to world-space to
    // obtain the vector aW. Then, we create the two orthogonal vectors n1 and
    // n2 that are orthogonal to aW.

    vec3s aW = qA * joint->slideAxis;
    vec3s nA = glms_vec3_norm_perpendicular(aW);
    vec3s nB = glms_vec3_cross(nA, aW);

    vec3s dcA = bodyA->deltaPosition;
    vec3s dcB = bodyB->deltaPosition;

    vec3s u = (dcB - dcA) + joint->centerDiff0 + rB - rA;

    vec3s rAPlusU = rA + u;
    vec3s rAPlusUCrossNA = glms_vec3_cross(rAPlusU, nA);
    vec3s rAPlusUCrossNB = glms_vec3_cross(rAPlusU, nB);

    vec3s rBCrossNA = glms_vec3_cross(rB, nA);
    vec3s rBCrossNB = glms_vec3_cross(rB, nB);

    vec3s iA_rAPlusUCrossNA = iA * rAPlusUCrossNA;
    vec3s iA_rAPlusUCrossNB = iA * rAPlusUCrossNB;

    vec3s iB_rBCrossNA = iB * rBCrossNA;
    vec3s iB_rBCrossNB = iB * rBCrossNB;

    f32 nADotDeltaV = glms_vec3_dot(nA, deltaV);
    f32 nBDotDeltaV = glms_vec3_dot(nB, deltaV);

    // CPos(s) = (x2 + r2 − x1 − r1) · n1
    //            x2 + r2 − x1 − r1) · n2)
    // Time derivate (eq 55)
    // =>
    // CDot = (n1 · v2 + ω2 · (r2 × n1) − n1 · v1 − ω1 · ((r1 + u) × n1)
    //         n2 · v2 + ω2 · (r2 × n2) − n2 · v1 − ω1 · ((r1 + u) × n2))
    //      = (n1 · (v2 - v1) + ω2 · (r2 × n1) − ω1 · ((r1 + u) × n1)
    //         n2 · (v2 - v1) + ω2 · (r2 × n2) − ω1 · ((r1 + u) × n2))
    // Jv = CDot
    vec2s jv = {nADotDeltaV +
		glms_vec3_dot(wB, rBCrossNA) -
		glms_vec3_dot(wA, rAPlusUCrossNA),
                nBDotDeltaV +
		glms_vec3_dot(wB, rBCrossNB) -
		glms_vec3_dot(wA, rAPlusUCrossNB)};

    if (useBias) {
      // Bias velocity vector
      // bias = β / ∆t * CPos(s)
      vec2s CPos = {glms_vec3_dot(u, nA),
                    glms_vec3_dot(u, nB)};

      bias = joint->base.biasCoefficient * CPos;
      massScale = joint->base.massCoefficient;
      impulseScale = joint->base.impulseCoefficient;
    }

    // Calculate lagrange multiplier:
    //
    // lambda = -K^-1 (Jv + b)
    vec2s impulse = massScale * (-1.f * joint->invEffectiveMass * (jv + bias)) -
                    impulseScale * joint->totalImpulse;
    vec2s newImpulse = joint->totalImpulse + impulse;
    // Store total impulse
    impulse = newImpulse - joint->totalImpulse;
    joint->totalImpulse = newImpulse;

    applySliderLinearVelocityStep(bodyA, bodyB, joint, impulse,
				  nA, nB,
                                  iA_rAPlusUCrossNA, iA_rAPlusUCrossNB,
                                  iB_rBCrossNA, iB_rBCrossNB);
  }
}

inline void warmStartSliderJoint(slider_joint* joint) {
  rigid_body* bodyA = joint->base.bodyA;
  rigid_body* bodyB = joint->base.bodyB;

  mat3s iA = joint->base.invIA, iB = joint->base.invIB;

  mat3s qA = bodyA->orientation;
  mat3s qB = bodyB->orientation;

  vec3s rA = qA * joint->base.localAnchorA;
  vec3s rB = qB * joint->base.localAnchorB;

  // constraint setup
  vec3s aW = qA * joint->slideAxis;
  vec3s nA = glms_vec3_norm_perpendicular(aW);
  vec3s nB = glms_vec3_cross(nA, aW);

  vec3s dcA = bodyA->deltaPosition;
  vec3s dcB = bodyB->deltaPosition;
  vec3s centerDiff = bodyB->position - bodyA->position;

  vec3s u = (dcB - dcA) + centerDiff + rB - rA;

  vec3s rAPlusU = rA + u;
  vec3s rAPlusUCrossNA = glms_vec3_cross(rAPlusU, nA);
  vec3s rAPlusUCrossNB = glms_vec3_cross(rAPlusU, nB);

  vec3s rBCrossNA = glms_vec3_cross(rB, nA);
  vec3s rBCrossNB = glms_vec3_cross(rB, nB);

  vec3s iA_rAPlusUCrossNA = iA * rAPlusUCrossNA;
  vec3s iA_rAPlusUCrossNB = iA * rAPlusUCrossNB;

  vec3s iB_rBCrossNA = iB * rBCrossNA;
  vec3s iB_rBCrossNB = iB * rBCrossNB;

  applySliderLinearVelocityStep(bodyA, bodyB, joint, joint->totalImpulse, nA,
                                nB, iA_rAPlusUCrossNA, iA_rAPlusUCrossNB,
                                iB_rBCrossNA, iB_rBCrossNB);
}
