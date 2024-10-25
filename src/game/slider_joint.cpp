#include "all.h"
// Constraint derivations from
// Constraints Derivation for Rigid Body Simulation in 3D by Daniel Chappuis

// A slider joint only allows relative translation between two bodies in a single direction. It has
// only one degree of freedom - Chapter 2.3
static void applySliderLinearVelocityStep(rigid_body* bodyA, rigid_body* bodyB, slider_joint* joint,
					  v2 lambda,
					  v3 nA, v3 nB,
					  v3 iA_rAPlusUCrossNA, v3 iA_rAPlusUCrossNB,
					  v3 iB_rBCrossNA, v3 iB_rBCrossNB) {
  f32 mA = joint->base.invMassA, mB = joint->base.invMassB;

  v3 impulse = nA * lambda.x + nB * lambda.y;
  {
    v3 V = mA * impulse;
    v3 W = iA_rAPlusUCrossNA * lambda.x + iA_rAPlusUCrossNB * lambda.y;

    bodyA->velocity = bodyA->velocity - V;
    bodyA->angularVelocity = bodyA->angularVelocity - W;
  }
  {
    v3 V = mB * impulse;
    v3 W = iB_rBCrossNA * lambda.x + iB_rBCrossNB * lambda.y;

    bodyB->velocity = bodyB->velocity + V;
    bodyB->angularVelocity = bodyB->angularVelocity + W;
  }
}

inline void setupSliderJoint(joint* j, rigid_body* rbA, rigid_body* rbB,
			     v3 pivotA, v3 pivotB, v3 axisIn,
			     f32 herz, f32 damping) {
  j->type = joint_type_slider;
  j->slider.base.herz = herz;
  j->slider.base.damping = damping;
  j->slider.base.bodyA = rbA;
  j->slider.base.bodyB = rbB;

  j->slider.base.localOriginAnchorA = {pivotA.x,pivotA.y,pivotA.z};
  j->slider.base.localOriginAnchorB = {pivotB.x,pivotB.y,pivotB.z};
  j->slider.slideAxis = axisIn;
  j->slider.rangeMin = -0.25f;
  j->slider.rangeMax = 0.f;
}

inline void preSolveSliderJoint(slider_joint* joint, f32 h) {
  rigid_body* bodyA = joint->base.bodyA;
  rigid_body* bodyB = joint->base.bodyB;

  joint->base.localAnchorA = joint->base.localOriginAnchorA;
  joint->base.localAnchorB = joint->base.localOriginAnchorB;

  v3 localAnchorA = joint->base.localAnchorA;
  v3 localAnchorB = joint->base.localAnchorB;

  joint->base.invMassB = bodyB->invMass;
  joint->base.invMassA = bodyA->invMass;

  joint->base.invIB = bodyB->invWorlInertiaTensor;
  joint->base.invIA = bodyA->invWorlInertiaTensor;

  f32 invMA = joint->base.invMassA, invMB = joint->base.invMassB;
  m3x3 iA = joint->base.invIA;
  m3x3 iB = joint->base.invIB;

  m3x3 qA = bodyA->orientation;
  m3x3 qB = bodyB->orientation;

  v3 rA = qA * localAnchorA;
  v3 rB = qB * localAnchorB;

  v3 aW = qA * joint->slideAxis;
  v3 nA = perpendicular(aW);
  v3 nB = v3_cross(nA, aW);

  v3 dcA = bodyA->deltaPosition;
  v3 dcB = bodyB->deltaPosition;
  v3 centerDiff = (bodyB->position - bodyA->position);

  v3 u = (dcB - dcA) + centerDiff + rB - rA;

  v3 rAPlusU = rA + u;
  v3 rAPlusUCrossNA = v3_cross(rAPlusU, nA);
  v3 rAPlusUCrossNB = v3_cross(rAPlusU, nB);

  v3 rBCrossNA = v3_cross(rB, nA);
  v3 rBCrossNB = v3_cross(rB, nB);

  v3 iA_rAPlusUCrossNA = iA * rAPlusUCrossNA;
  v3 iA_rAPlusUCrossNB = iA * rAPlusUCrossNB;

  v3 iB_rBCrossNA = iB * rBCrossNA;
  v3 iB_rBCrossNB = iB * rBCrossNB;

  f32 invMass = (invMA + invMB);

  // Constraint mass matrix
  // (eq 59)
  m2x2 K = {invMass + v3_dot(rAPlusUCrossNA, iA_rAPlusUCrossNA) +
                 v3_dot(rBCrossNA, iB_rBCrossNA),
             v3_dot(rAPlusUCrossNA, iA_rAPlusUCrossNB) +
                 v3_dot(rBCrossNA, iB_rBCrossNB),
             v3_dot(rAPlusUCrossNB, iA_rAPlusUCrossNA) +
                 v3_dot(rBCrossNB, iB_rBCrossNA),
             invMass + v3_dot(rAPlusUCrossNB, iA_rAPlusUCrossNB) +
                 v3_dot(rBCrossNB, iB_rBCrossNB)};

  joint->invEffectiveMass = m2x2_inverse(K);
  joint->centerDiff0 = centerDiff;

  calculateSoftConstraintProperties(
      h, joint->base.herz, joint->base.damping, &joint->base.biasCoefficient,
      &joint->base.impulseCoefficient, &joint->base.massCoefficient);
}

inline void solveSliderJoint(slider_joint* joint, f32 h, f32 invH,
                             b32 useBias) {
  rigid_body* bodyA = joint->base.bodyA;
  rigid_body* bodyB = joint->base.bodyB;

  v3 vA = bodyA->velocity;
  v3 wA = bodyA->angularVelocity;

  v3 vB = bodyB->velocity;
  v3 wB = bodyB->angularVelocity;

  m3x3 iA = joint->base.invIA;
  m3x3 iB = joint->base.invIB;

  v3 localAnchorA = joint->base.localAnchorA;
  v3 localAnchorB = joint->base.localAnchorB;
  {
    v2 bias = V2_ZERO;
    f32 massScale = 1.0f;
    f32 impulseScale = 0.0f;

    v3 deltaV = vB - vA;

    m3x3 qA = bodyA->orientation;
    m3x3 qB = bodyB->orientation;

    v3 rA = qA * localAnchorA;
    v3 rB = qB * localAnchorB;

    // The two vectors n1 and n2 are two unit orthogonal vectors that are
    // orthogonal to the slider axis a. At the joint creation, we convert the
    // slider axis a into the local-space of body B1 and we get the vector al .
    // Then, at each frame, we convert the vector aL back to world-space to
    // obtain the vector aW. Then, we create the two orthogonal vectors n1 and
    // n2 that are orthogonal to aW.

    v3 aW = qA * joint->slideAxis;
    v3 nA = perpendicular(aW);
    v3 nB = v3_cross(nA, aW);

    v3 dcA = bodyA->deltaPosition;
    v3 dcB = bodyB->deltaPosition;

    v3 u = (dcB - dcA) + joint->centerDiff0 + rB - rA;

    v3 rAPlusU = rA + u;
    v3 rAPlusUCrossNA = v3_cross(rAPlusU, nA);
    v3 rAPlusUCrossNB = v3_cross(rAPlusU, nB);

    v3 rBCrossNA = v3_cross(rB, nA);
    v3 rBCrossNB = v3_cross(rB, nB);

    v3 iA_rAPlusUCrossNA = iA * rAPlusUCrossNA;
    v3 iA_rAPlusUCrossNB = iA * rAPlusUCrossNB;

    v3 iB_rBCrossNA = iB * rBCrossNA;
    v3 iB_rBCrossNB = iB * rBCrossNB;

    f32 nADotDeltaV = v3_dot(nA, deltaV);
    f32 nBDotDeltaV = v3_dot(nB, deltaV);

    // CPos(s) = (x2 + r2 − x1 − r1) · n1
    //            x2 + r2 − x1 − r1) · n2)
    // Time derivate (eq 55)
    // =>
    // CDot = (n1 · v2 + ω2 · (r2 × n1) − n1 · v1 − ω1 · ((r1 + u) × n1)
    //         n2 · v2 + ω2 · (r2 × n2) − n2 · v1 − ω1 · ((r1 + u) × n2))
    //      = (n1 · (v2 - v1) + ω2 · (r2 × n1) − ω1 · ((r1 + u) × n1)
    //         n2 · (v2 - v1) + ω2 · (r2 × n2) − ω1 · ((r1 + u) × n2))
    // Jv = CDot
    v2 jv = {nADotDeltaV +
		v3_dot(wB, rBCrossNA) -
		v3_dot(wA, rAPlusUCrossNA),
                nBDotDeltaV +
		v3_dot(wB, rBCrossNB) -
		v3_dot(wA, rAPlusUCrossNB)};

    if (useBias) {
      // Bias velocity vector
      // bias = β / ∆t * CPos(s)
      v2 CPos = {v3_dot(u, nA), v3_dot(u, nB)};

      bias = joint->base.biasCoefficient * CPos;
      massScale = joint->base.massCoefficient;
      impulseScale = joint->base.impulseCoefficient;
    }

    // Calculate lagrange multiplier:
    //
    // lambda = -K^-1 (Jv + b)
    v2 impulse = massScale * (-1.f * joint->invEffectiveMass * (jv + bias)) -
                    impulseScale * joint->totalImpulse;
    v2 newImpulse = joint->totalImpulse + impulse;
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

  m3x3 iA = joint->base.invIA;
  m3x3 iB = joint->base.invIB;

  m3x3 qA = bodyA->orientation;
  m3x3 qB = bodyB->orientation;

  v3 localAnchorA = joint->base.localAnchorA;
  v3 localAnchorB = joint->base.localAnchorB;

  v3 rA = qA * localAnchorA;
  v3 rB = qB * localAnchorB;

  // constraint setup
  v3 aW = qA * joint->slideAxis;
  v3 nA = perpendicular(aW);
  v3 nB = v3_cross(nA, aW);

  v3 dcA = bodyA->deltaPosition;
  v3 dcB = bodyB->deltaPosition;

  v3 pA = bodyA->position;
  v3 pB = bodyB->position;

  v3 centerDiff = pB - pA;

  v3 u = (dcB - dcA) + centerDiff + rB - rA;

  v3 rAPlusU = rA + u;
  v3 rAPlusUCrossNA = v3_cross(rAPlusU, nA);
  v3 rAPlusUCrossNB = v3_cross(rAPlusU, nB);

  v3 rBCrossNA = v3_cross(rB, nA);
  v3 rBCrossNB = v3_cross(rB, nB);

  v3 iA_rAPlusUCrossNA = iA * rAPlusUCrossNA;
  v3 iA_rAPlusUCrossNB = iA * rAPlusUCrossNB;

  v3 iB_rBCrossNA = iB * rBCrossNA;
  v3 iB_rBCrossNB = iB * rBCrossNB;

  applySliderLinearVelocityStep(bodyA, bodyB, joint, joint->totalImpulse, nA,
                                nB, iA_rAPlusUCrossNA, iA_rAPlusUCrossNB,
                                iB_rBCrossNA, iB_rBCrossNB);
}
