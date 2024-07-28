// Constraint derivations from
// Constraints Derivation for Rigid Body Simulation in 3D by Daniel Chappuis

// Constraints motion along single axis - Chapter 2.3.5

static void applyAxisLimitLinearVelocityStep(rigid_body* bodyA, rigid_body* bodyB, axis_joint* joint,
					     f32 lambda, vec3s aW, vec3s rAPlusUCrossA, vec3s rBCrossA) {
  f32 mA = joint->base.invMassA, mB = joint->base.invMassB;
  mat3s iA = bodyA->invWorlInertiaTensor, iB = bodyB->invWorlInertiaTensor;
  vec3s impulse = aW * lambda;
  {
    vec3s V = mA * impulse;
    vec3s W = iA * rAPlusUCrossA * lambda;

    bodyA->velocity = bodyA->velocity - V;
    bodyA->angularVelocity = bodyA->angularVelocity - W;
  }
  {
    vec3s V = mB * impulse;
    vec3s W = iB * rBCrossA * lambda;

    bodyB->velocity = bodyB->velocity + V;
    bodyB->angularVelocity = bodyB->angularVelocity + W;
  }
}

inline void setupAxisJoint(joint* j, rigid_body* rbA, rigid_body* rbB,
			   vec3s pivotA, vec3s pivotB, vec3s axisIn,
			   f32 rangeMin, f32 rangeMax,
			   f32 herz, f32 damping) {
  j->type = joint_type_axis;
  j->axis.base.herz = herz;
  j->axis.base.damping = damping;
  j->axis.base.bodyA = rbA;
  j->axis.base.bodyB = rbB;

  j->axis.base.localOriginAnchorA = pivotA;
  j->axis.base.localOriginAnchorB = pivotB;
  j->axis.slideAxis = axisIn;
  j->axis.rangeMin = rangeMin;
  j->axis.rangeMax = rangeMax;
}

inline void preSolveAxisJoint(axis_joint* joint, f32 h) {
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

  // Constraint setup
  {
    vec3s aW = qA * joint->slideAxis;

    vec3s dcA = bodyA->deltaPosition;
    vec3s dcB = bodyB->deltaPosition;
    vec3s centerDiff = bodyB->position - bodyA->position;

    vec3s u = (dcB - dcA) + centerDiff + rB - rA;

    vec3s rBCrossA = glms_vec3_cross(rB, aW);
    vec3s rAPlusUCrossA = glms_vec3_cross(rA + u, aW);

    f32 invMass = (invMA + invMB);
    // (eq 59)
    f32 K = invMass +
            glms_vec3_dot(rAPlusUCrossA, iA * rAPlusUCrossA) +
            glms_vec3_dot(rBCrossA, iB * rBCrossA);

    joint->invEffectiveMass = 1.f / K;
    joint->centerDiff0 = centerDiff;
  }

  calculateSoftConstraintProperties(h, joint->base.herz,
				    joint->base.damping,
				    &joint->base.biasCoefficient,
				    &joint->base.impulseCoefficient,
				    &joint->base.massCoefficient);
}

inline void solveAxisJointMaxLimit(axis_joint* joint, f32 h, f32 invH,
                                     f32 range, b32 useBias) {
  rigid_body* bodyA = joint->base.bodyA;
  rigid_body* bodyB = joint->base.bodyB;

  vec3s vA = bodyA->velocity;
  vec3s wA = bodyA->angularVelocity;
  vec3s vB = bodyB->velocity;
  vec3s wB = bodyB->angularVelocity;

  mat3s qA = bodyA->orientation;
  mat3s qB = bodyB->orientation;

  vec3s rA = qA * joint->base.localAnchorA;
  vec3s rB = qB * joint->base.localAnchorB;

  // Constraint setup
  vec3s aW = qA * joint->slideAxis;

  vec3s dcA = bodyA->deltaPosition;
  vec3s dcB = bodyB->deltaPosition;

  vec3s u = (dcB - dcA) + joint->centerDiff0 + rB - rA;

  vec3s rBCrossA = glms_vec3_cross(rB, aW);
  vec3s rAPlusUCrossA = glms_vec3_cross(rA + u, aW);

  f32 bias = 0.f;
  f32 massScale = 1.0f;
  f32 impulseScale = 0.0f;

  // Cmin(s) = dmax - u · a
  // time derivate
  // C'min(s) = -(a · v2 + ω2 · (r2 × a) − a · v1 − ω1 · ((r1 + u) × a))
  //          = -(a · (v2 - v1) + ω2 · (r2 × a) − ω1 · ((r1 + u) × a))
  vec3s a = aW;
  f32 jv = glms_vec3_dot(a, vB - vA) +
	    glms_vec3_dot(wB, rBCrossA) -
	    glms_vec3_dot(wA, rAPlusUCrossA);

  if (useBias) {
    // Calculate translation bias velocity
    // bias = beta / h * CPos
    f32 CPos = joint->rangeMax - range;

    bias = -joint->base.biasCoefficient * CPos;
    massScale = joint->base.massCoefficient;
    impulseScale = joint->base.impulseCoefficient;
  }

  // lambda = -K^-1 (J v + b)
  f32 impulse =
      massScale * (-joint->invEffectiveMass * (jv + bias)) - impulseScale * joint->totalImpulse;
  f32 newImpulse = joint->totalImpulse + impulse;
  // Store total impulse
  impulse = newImpulse - joint->totalImpulse;
  joint->totalImpulse = newImpulse;

  applyAxisLimitLinearVelocityStep(bodyA, bodyB, joint, impulse, aW, rAPlusUCrossA, rBCrossA);
}

inline void solveAxisJointMinLimit(axis_joint* joint, f32 h, f32 invH,
                                     f32 range, b32 useBias) {
  rigid_body* bodyA = joint->base.bodyA;
  rigid_body* bodyB = joint->base.bodyB;

  vec3s vA = bodyA->velocity;
  vec3s wA = bodyA->angularVelocity;
  vec3s vB = bodyB->velocity;
  vec3s wB = bodyB->angularVelocity;

  mat3s qA = bodyA->orientation;
  mat3s qB = bodyB->orientation;

  vec3s rA = qA * joint->base.localAnchorA;
  vec3s rB = qB * joint->base.localAnchorB;

  // Constraint setup
  vec3s aW = qA * joint->slideAxis;

  vec3s dcA = bodyA->deltaPosition;
  vec3s dcB = bodyB->deltaPosition;

  vec3s u = (dcB - dcA) + joint->centerDiff0 + rB - rA;

  vec3s rBCrossA = glms_vec3_cross(rB, aW);
  vec3s rAPlusUCrossA = glms_vec3_cross(rA + u, aW);

  f32 bias = 0.f;
  f32 massScale = 1.0f;
  f32 impulseScale = 0.0f;

  // Cmin(s) = u · a − dmin
  // time derivate
  // C'min(s) = a · v2 + ω2 · (r2 × a) − a · v1 − ω1 · ((r1 + u) × a)
  //          = a · (v2 - v1) + ω2 · (r2 × a) − ω1 · ((r1 + u) × a)
  vec3s a = aW;
  f32 jv = (glms_vec3_dot(a, vB - vA) +
	    glms_vec3_dot(wB, rBCrossA) -
	    glms_vec3_dot(wA, rAPlusUCrossA));

  if (useBias) {
    // Calculate translation bias velocity
    // bias = beta / h * CPos
    f32 CPos = range - joint->rangeMin;

    bias = joint->base.biasCoefficient * CPos;
    massScale = joint->base.massCoefficient;
    impulseScale = joint->base.impulseCoefficient;
  }

  // lambda = -K^-1 (J v + b)
  f32 impulse =
      massScale * (-joint->invEffectiveMass * (jv + bias)) - impulseScale * joint->totalImpulse;
  f32 newImpulse = joint->totalImpulse + impulse;
  // Store total impulse
  impulse = newImpulse - joint->totalImpulse;
  joint->totalImpulse = newImpulse;

  applyAxisLimitLinearVelocityStep(bodyA, bodyB, joint, impulse, aW, rAPlusUCrossA, rBCrossA);
}

inline void solveAxisJoint(axis_joint* joint, f32 h, f32 invH,
                             b32 useBias) {
  // limits
  rigid_body* bodyA = joint->base.bodyA;
  rigid_body* bodyB = joint->base.bodyB;

  mat3s qA = bodyA->orientation;
  mat3s qB = bodyB->orientation;

  vec3s rA = qA * joint->base.localAnchorA;
  vec3s rB = qB * joint->base.localAnchorB;

  vec3s dcA = bodyA->deltaPosition;
  vec3s dcB = bodyB->deltaPosition;

  vec3s u = (dcB - dcA) + joint->centerDiff0 + rB - rA;

  vec3s aW = qA * joint->slideAxis;
  f32 range = glms_vec3_dot(aW, u);
  if (range < joint->rangeMin) {
    solveAxisJointMinLimit(joint, h, invH, range, useBias);
  }
  else if (range > joint->rangeMax) {
    solveAxisJointMaxLimit(joint, h, invH, range, useBias);
  }
}

inline void warmStartAxisJoint(axis_joint* joint) {
  rigid_body* bodyA = joint->base.bodyA;
  rigid_body* bodyB = joint->base.bodyB;

  joint->base.localAnchorA = joint->base.localOriginAnchorA;
  joint->base.localAnchorB = joint->base.localOriginAnchorB;

  joint->base.invMassB = bodyB->invMass;
  joint->base.invMassA = bodyA->invMass;

  joint->base.invIB = bodyB->invWorlInertiaTensor;
  joint->base.invIA = bodyA->invWorlInertiaTensor;

  mat3s qA = bodyA->orientation;
  mat3s qB = bodyB->orientation;

  vec3s rA = qA * joint->base.localAnchorA;
  vec3s rB = qB * joint->base.localAnchorB;

  // Constraint setup
  vec3s aW = qA * joint->slideAxis;

  vec3s dcA = bodyA->deltaPosition;
  vec3s dcB = bodyB->deltaPosition;

  vec3s u = (dcB - dcA) + joint->centerDiff0 + rB - rA;

  vec3s rBCrossA = glms_vec3_cross(rB, aW);
  vec3s rAPlusUCrossA = glms_vec3_cross(rA + u, aW);

  f32 range = glms_vec3_dot(aW, u);
  if (range < joint->rangeMin || range > joint->rangeMax) {
    applyAxisLimitLinearVelocityStep(bodyA, bodyB, joint, joint->totalImpulse, aW, rAPlusUCrossA, rBCrossA);
  }
}
