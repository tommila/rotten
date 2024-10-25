#include "all.h"
// Constraint derivations from
// Constraints Derivation for Rigid Body Simulation in 3D by Daniel Chappuis

// The distance joint only allows arbitrary rotation between two bodies but no translation.
// It has three degrees of freedom.
static void applyLinearVelocityStep(rigid_body* bodyA, rigid_body* bodyB, distance_joint* joint,
                              v3 impulse) {
  m3x3 iA = bodyA->invWorlInertiaTensor; 
  m3x3 iB = bodyB->invWorlInertiaTensor; 
  
  v3 localAnchorA = joint->base.localAnchorA; 
  v3 localAnchorB = joint->base.localAnchorB; 
 
  m3x3 qA = bodyA->orientation;
  m3x3 qB = bodyB->orientation;

  v3 rA = qA * localAnchorA;
  v3 rB = qB * localAnchorB;

  f32 mA = joint->base.invMassA, mB = joint->base.invMassB;

  v3 vA = mA * impulse;
  v3 wA = iA * v3_cross(rA, impulse);

  v3 vB = mB * impulse;
  v3 wB = iB * v3_cross(rB, impulse);

  bodyA->velocity = bodyA->velocity - vA;
  bodyA->angularVelocity = bodyA->angularVelocity - wA;
  
  bodyB->velocity = bodyB->velocity + vB;
  bodyB->angularVelocity = bodyB->angularVelocity + wB;
}

inline void setupDistanceJoint(joint* j, rigid_body* rbA, rigid_body* rbB,
			       v3 pivotA, v3 pivotB, v3 axisInA, v3 axisInB,
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

  joint->base.invIA = bodyA->invWorlInertiaTensor; 
  joint->base.invIB = bodyB->invWorlInertiaTensor; 
  
  v3 localAnchorA = joint->base.localAnchorA; 
  v3 localAnchorB = joint->base.localAnchorB; 
 
  m3x3 qA = bodyA->orientation;
  m3x3 qB = bodyB->orientation;

  v3 rA = qA * localAnchorA;
  v3 rB = qB * localAnchorB;

  v3 centerDiff = bodyB->position - bodyA->position;

  // constraint setup
  {
    m3x3 S1 = m3x3_skew_symmetric(rA);
    m3x3 S2 = m3x3_skew_symmetric(rB);

    m3x3 K =
      (m3x3)M3X3_IDENTITY * joint->base.invIA +
      S1 * joint->base.invIA * m3x3_transpose(S1) +
      (m3x3)M3X3_IDENTITY * joint->base.invIB +
      S2 * joint->base.invIB * m3x3_transpose(S2);

    joint->invEffectiveMass = m3x3_inverse(K);
    joint->centerDiff0 = centerDiff;

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
    v3 vA = bodyA->velocity;
    v3 wA = bodyA->angularVelocity;

    v3 vB = bodyB->velocity;
    v3 wB = bodyB->angularVelocity;

    m3x3 qA = bodyA->orientation;
    m3x3 qB = bodyB->orientation;

    v3 localAnchorA = joint->base.localAnchorA;
    v3 localAnchorB = joint->base.localAnchorB;

    v3 rA = qA * localAnchorA;
    v3 rB = qB * localAnchorB;

    v3 bias = V3_ZERO;
    f32 massScale = 1.0f;
    f32 impulseScale = 0.0f;

    // CPos(s) = x2 + r2 + r2 − x1 − r1
    // Time derivate
    // =>
    // CDot = (v2 + ω2 × r2 − v1 − ω1 × r1)
    // Jv = CDot
    v3 jv = ((vA - v3_cross(rA, wA)) - vB + v3_cross(rB, wB));

    if (useBias) {
      v3 dcA = bodyA->deltaPosition;
      v3 dcB = bodyB->deltaPosition;

      // Calculate translation bias velocity
      // bias = beta / h * CPos
      v3 CPos = (dcB - dcA) + joint->centerDiff0 + rB - rA;

      bias = -joint->base.biasCoefficient * CPos;
      massScale = joint->base.massCoefficient;
      impulseScale = joint->base.impulseCoefficient;
      // bias = -baumgarte * invH * CPos;
    }

    // Calculate lagrange multiplier:
    //
    // lambda = -K^-1 (Jv + b)
    m3x3 invEffectiveMass = joint->invEffectiveMass;
    v3 impulse = massScale * (invEffectiveMass * (jv + bias)) -
      impulseScale * joint->totalImpulse;
    v3 newImpulse = joint->totalImpulse + impulse;
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
