#include "all.h"

// Constraint derivations from
// Constraints Derivation for Rigid Body Simulation in 3D by Daniel Chappuis

// Constraints motion along single axis - Chapter 2.3.5

static void applyAxisLimitLinearVelocityStep(rigid_body* bodyA, rigid_body* bodyB, axis_joint* joint,
					     f32 lambda, v3 aW, v3 rAPlusUCrossA, v3 rBCrossA) {
  f32 mA = joint->base.invMassA, mB = joint->base.invMassB;
  m3x3 iA = bodyA->invWorlInertiaTensor;
  m3x3 iB = bodyB->invWorlInertiaTensor;
  v3 impulse = aW * lambda;
  {
    v3 V = mA * impulse;
    v3 W = iA * rAPlusUCrossA * lambda;

    bodyA->velocity = bodyA->velocity - V;
    bodyA->angularVelocity = bodyA->angularVelocity - W;
  }
  {
    v3 V = mB * impulse;
    v3 W = iB * rBCrossA * lambda;

    bodyB->velocity = bodyB->velocity + V;
    bodyB->angularVelocity = bodyB->angularVelocity + W;
  }
}

inline void setupAxisJoint(joint* j, rigid_body* rbA, rigid_body* rbB,
			   v3 pivotA, v3 pivotB, v3 axisIn,
			   f32 herz, f32 damping) {
  j->type = joint_type_axis;
  j->axis.base.herz = herz;
  j->axis.base.damping = damping;
  j->axis.base.bodyA = rbA;
  j->axis.base.bodyB = rbB;

  j->axis.base.localOriginAnchorA = pivotA;
  j->axis.base.localOriginAnchorB = pivotB;
  j->axis.slideAxis = axisIn;

  j->axis.base.localAnchorA = j->axis.base.localOriginAnchorA;
  j->axis.base.localAnchorB = j->axis.base.localOriginAnchorB;
}

inline void preSolveAxisJoint(axis_joint* joint, f32 h) {
  rigid_body* bodyA = joint->base.bodyA;
  rigid_body* bodyB = joint->base.bodyB;

  joint->base.invMassB = bodyB->invMass;
  joint->base.invMassA = bodyA->invMass;

  joint->base.invIB = bodyB->invWorlInertiaTensor;
  joint->base.invIA = bodyA->invWorlInertiaTensor;

  v3 localAnchorA = joint->base.localAnchorA;
  v3 localAnchorB = joint->base.localAnchorB;

  f32 invMA = joint->base.invMassA, invMB = joint->base.invMassB;

  m3x3 iA = joint->base.invIA;
  m3x3 iB = joint->base.invIB;

  m3x3 qA = bodyA->orientation;
  m3x3 qB = bodyB->orientation;

  v3 rA = qA * localAnchorA;
  v3 rB = qB * localAnchorB;

  // Constraint setup
  {
    v3 aW = qA * joint->slideAxis;
    v3 dcA = bodyA->deltaPosition;
    v3 dcB = bodyB->deltaPosition;
    v3 centerDiff = (bodyB->position - bodyA->position);

    v3 u = (dcB - dcA) + centerDiff + (rB - rA);

    v3 rBCrossA = v3_cross(rB, aW);
    v3 rAPlusUCrossA = v3_cross(rA + u, aW);

    f32 invMass = (invMA + invMB);
    // (eq 59)
    f32 K = invMass +
            v3_dot(rAPlusUCrossA, iA * rAPlusUCrossA) +
            v3_dot(rBCrossA, iB * rBCrossA);

    joint->invEffectiveMass = 1.f / K;
    joint->centerDiff0 = centerDiff;
  }

  calculateSoftConstraintProperties(h, joint->base.herz,
				    joint->base.damping,
				    &joint->base.biasCoefficient,
				    &joint->base.impulseCoefficient,
				    &joint->base.massCoefficient);
}

// TODO: axis joint breaks if exposed to large forces.
//       Figure out why.
inline void solveAxisJoint(axis_joint* joint, f32 h, f32 invH, b32 useBias) {
  rigid_body* bodyA = joint->base.bodyA;
  rigid_body* bodyB = joint->base.bodyB;

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
 
  // Constraint setup
  v3 aW = qA * joint->slideAxis;

  v3 dcA = bodyA->deltaPosition;
  v3 dcB = bodyB->deltaPosition;

  v3 u = (dcB - dcA) + joint->centerDiff0 + (rB - rA);

  v3 rBCrossA = v3_cross(rB, aW);
  v3 rAPlusUCrossA = v3_cross(rA + u, aW);

  f32 bias = 0.f;
  f32 massScale = 1.0f;
  f32 impulseScale = 0.0f;
  // Cmin(s) = dmax - u · a
  // time derivate
  // C'min(s) = -(a · v2 + ω2 · (r2 × a) − a · v1 − ω1 · ((r1 + u) × a))
  //          = -(a · (v2 - v1) + ω2 · (r2 × a) − ω1 · ((r1 + u) × a))
  
  f32 jv = v3_dot(aW, vB - vA) +
	    v3_dot(wB, rBCrossA) -
	    v3_dot(wA, rAPlusUCrossA);

  f32 CPos = v3_dot(aW, u);
  if (useBias) {
    // Calculate translation bias velocity
    // bias = beta / h * CPos
    bias = joint->base.biasCoefficient * CPos;
    massScale = joint->base.massCoefficient;
    impulseScale = joint->base.impulseCoefficient;
  }

  // lambda = -K^-1 (J v + b)
  f32 impulse =
  massScale * (-joint->invEffectiveMass * (jv + bias)) - impulseScale * joint->totalImpulse;
  f32 oldImpulse = joint->totalImpulse;
  joint->totalImpulse = joint->totalImpulse + impulse;
  // Store total impulse
  impulse = joint->totalImpulse - oldImpulse;

  applyAxisLimitLinearVelocityStep(bodyA, bodyB, joint, impulse, aW, rAPlusUCrossA, rBCrossA);
}

inline void warmStartAxisJoint(axis_joint* joint) {
  rigid_body* bodyA = joint->base.bodyA;
  rigid_body* bodyB = joint->base.bodyB;

  joint->base.invMassB = bodyB->invMass;
  joint->base.invMassA = bodyA->invMass;

  m3x3 qA = bodyA->orientation;
  m3x3 qB = bodyB->orientation;

  v3 localAnchorA = joint->base.localAnchorA;
  v3 localAnchorB = joint->base.localAnchorB;

  v3 rA = qA * localAnchorA;
  v3 rB = qB * localAnchorB;
 
  // Constraint setup
  v3 aW = qA * joint->slideAxis;

  v3 dcA = bodyA->deltaPosition;
  v3 dcB = bodyB->deltaPosition;

  v3 u = (dcB - dcA) + joint->centerDiff0 + (rB - rA);

  v3 rBCrossA = v3_cross(rB, aW);
  v3 rAPlusUCrossA = v3_cross(rA + u, aW);

  applyAxisLimitLinearVelocityStep(bodyA, bodyB, joint, 
                                   joint->totalImpulse, aW, rAPlusUCrossA, rBCrossA);
}
