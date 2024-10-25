#include "all.h"
// Temporal Gauss-Seidel (TGS) with soft constraint contact resolver.
// Adapted from Erin Catto Solver2d library
// https://box2d.org/posts/2024/02/solver2d/

// SPDX-FileCopyrightText: 2024 Erin Catto
// SPDX-License-Identifier: MIT
typedef struct contact_constraint_point {
  contact_point point;
  f32 adjustedSeparation;
  f32 normalMass;
  f32 tangentMass[2];

  // Soft constraint
  f32 massCoefficient;
  f32 biasCoefficient;
  f32 impulseCoefficient;
} contact_constraint_point;

typedef struct contact_constraint {
  rigid_body* bodyA;
  rigid_body* bodyB;
  contact_constraint_point ccp;
  v3 normal;
  v3 tangents[2];
  f32 friction;
} contact_constraint;

static i32 findOldContact(contact_point* contactPoints, u32 size, i32 shapeIdx) {
  for (u32 i = 0; i < size; i++) {
    contact_point* cp = contactPoints + i;
    if (cp->shapeIdx == shapeIdx) {
      return i;
    }
  }
  return -1;
}

static void preSolve(contact_manifold* man,
                     contact_constraint* constraint, u32 contactNum, float h) {
  constraint->bodyA = man->bodyA;
  constraint->bodyB = man->bodyB;
  compute_basis(man->point.normal, constraint->tangents, constraint->tangents + 1);
  constraint->normal = man->point.normal;

  constraint->friction = sqrtf(man->bodyA->friction + man->bodyB->friction);

  f32 mA = constraint->bodyA->invMass;
  m3x3 iA = constraint->bodyA->invWorlInertiaTensor;
  f32 mB = constraint->bodyB->invMass;
  m3x3 iB = constraint->bodyB->invWorlInertiaTensor;

  contact_constraint_point* ccp = &constraint->ccp;
  ccp->point = man->point;
  v3 rA = ccp->point.localPointA;
  v3 rB = ccp->point.localPointB;

  v3 normal = constraint->normal;

  ccp->adjustedSeparation = ccp->point.separation - v3_dot((rB - rA), normal);

  v3 rnA = v3_cross(rA, normal);
  v3 rnB = v3_cross(rB, normal);
  f32 nm = mA + mB;
  f32 tm[2] = {nm, nm};
  f32 kNormal =
      nm + v3_dot(rnA, iA * rnA) + v3_dot(rnB, iB * rnB);
  ccp->normalMass = 1.0f / kNormal;

  for (u32 i = 0; i < 2; i++) {
    v3 rtA = v3_cross(constraint->tangents[i], rA);
    v3 rtB = v3_cross(constraint->tangents[i], rB);
    tm[i] += v3_dot(rtA, iA * rtA) + v3_dot(rtB, iB * rtB);
    ccp->tangentMass[i] = tm[i] > 0.0f ? 1.0f / tm[i] : 0.0f;
  }

  // soft contact
  f32 zeta = 0.7f;                       // damping ratio
  f32 herz = 30.f;                       // cycler per second
  f32 omega = 2.f * PI * herz;           // angular frequency
  f32 shared = 2.0f * zeta + h * omega;  // shared expression
  float c = h * omega * shared;
  ccp->biasCoefficient = omega / shared;  // omega / (2.0f * zeta + omegaDelta);
  ccp->impulseCoefficient = 1.0f / (1.0f + c);
  ccp->massCoefficient = c * ccp->impulseCoefficient;
}

static void warmStart(contact_constraint* constraint, u32 constraintNum) {
  contact_constraint_point* cp = &constraint->ccp;

  v3 normal = constraint->normal;
  v3 P = (cp->point.normalImpulse * normal);

  float mA = constraint->bodyA->invMass;
  m3x3 iA = constraint->bodyA->invWorlInertiaTensor;

  v3 vA = constraint->bodyA->velocity;
  v3 wA = constraint->bodyA->angularVelocity;

  v3 rA = cp->point.localPointA;

  float mB = constraint->bodyB->invMass;
  m3x3 iB = constraint->bodyB->invWorlInertiaTensor;

  v3 vB = constraint->bodyB->velocity;
  v3 wB = constraint->bodyB->angularVelocity;

  v3 rB = cp->point.localPointB;

  for (u32 i = 0; i < 2; i++) {
    P = P + constraint->tangents[i] * cp->point.tangentImpulse[i];
  }

  vA = vA - mA * P;
  wA = wA - iA * v3_cross(rA, P);

  vB = vB + mB * P;
  wB = wB + iB * v3_cross(rB, P);

  constraint->bodyA->velocity = vA;
  constraint->bodyA->angularVelocity = wA;
  constraint->bodyB->velocity = vB;
  constraint->bodyB->angularVelocity = wB;
}

static void solve(contact_constraint* constraint, u32 constraintNum, float invH,
                  b32 useBias) {
  contact_constraint_point* cp = &constraint->ccp;

  float mA = constraint->bodyA->invMass;
  m3x3 iA = constraint->bodyA->invWorlInertiaTensor;

  v3 vA = constraint->bodyA->velocity;
  v3 wA = constraint->bodyA->angularVelocity;

  v3 dcA = constraint->bodyA->deltaPosition;

  float mB = constraint->bodyB->invMass;
  m3x3 iB = constraint->bodyB->invWorlInertiaTensor;

  v3 vB = constraint->bodyB->velocity;
  v3 wB = constraint->bodyB->angularVelocity;

  v3 dcB = constraint->bodyB->deltaPosition;

  v3 normal = constraint->normal;

  float friction = constraint->friction;

  // compute current separation
  v3 rA = cp->point.localPointA;
  v3 rB = cp->point.localPointB;

  // Relative velocity at contact
  v3 vrA = v3_cross(wA, rA);
  v3 vrB = v3_cross(wB, rB);

  v3 dv = (vB + vrB) - (vA + vrA);
  // Normal
  {
    v3 ds = (dcB - dcA) + (rB - rA);
    f32 separation = v3_dot(ds, normal) + cp->adjustedSeparation;

    f32 bias = 0.0f;
    f32 massScale = 1.0f;
    f32 impulseScale = 0.0f;
    if (separation > 0.0f) {
      // Speculative
      bias = separation * invH;
    } else if (useBias) {
      // TODO: original was MAX(cp->biasCoefficient * separation, -4.0f);
      bias = MIN(cp->biasCoefficient * separation, 4.0f);
      massScale = cp->massCoefficient;
      impulseScale = cp->impulseCoefficient;
    }

    f32 vn = v3_dot(dv, normal);

    // Compute normal impulse
    f32 impulse = -cp->normalMass * massScale * (vn + bias) -
                  impulseScale * cp->point.normalImpulse;
    // Clamp the accumulated impulse
    f32 newImpulse = MAX(cp->point.normalImpulse + impulse, 0.0f);
    impulse = newImpulse - cp->point.normalImpulse;
    cp->point.normalImpulse = newImpulse;

    // Apply contact impulse
    v3 P = impulse * normal;
    vA = vA - mA * P;
    wA = wA - iA * v3_cross(rA, P);
    vB = vB + mB * P;
    wB = wB + iB * v3_cross(rB, P);
  }

  // Friction
  {
    contact_constraint_point* cp = &constraint->ccp;
    for (u32 i = 0; i < 2; ++i) {
      // Compute tangent force
      f32 lambda =
          -v3_dot(dv, constraint->tangents[i]) * cp->tangentMass[i];

      // Clamp the accumulated force
      f32 maxFriction = friction * cp->point.normalImpulse;
      f32 oldLambda = cp->point.tangentImpulse[i];
      cp->point.tangentImpulse[i] =
          CLAMP(oldLambda + lambda, -maxFriction, maxFriction);
      lambda = cp->point.tangentImpulse[i] - oldLambda;

      // Apply contact impulse
      v3 impulse = constraint->tangents[i] * lambda;

      vA = vA - mA * impulse;
      wA = wA - iA * v3_cross(rA, impulse);
      vB = vB + mB * impulse;
      wB = wB + iB * v3_cross(rB, impulse);
    }
  }

  constraint->bodyA->velocity = vA;
  constraint->bodyA->angularVelocity = wA;
  constraint->bodyB->velocity = vB;
  constraint->bodyB->angularVelocity = wB;
}

static void integrateVelocities(float h,
				rigid_body* body,
				v3 extForces,
				v3 extTorques) {
  // Integrate velocities
  v3 cmForce = - (Kdl * body->velocity * v3_normalize(body->velocity))
    - (Krr * body->velocity)
    + (Gravity * body->mass)
    + extForces;
  // Dampings
  v3 torque = -Kda * body->angularVelocity + extTorques;

  // vCM = vCM + h * (FT/M)
  v3 acc = body->invMass * cmForce;
  body->velocity = body->velocity + acc * h;
  // ICM-1 = A * _I-1 * AT
  body->invWorlInertiaTensor = body->orientation *
                                   body->invBodyInertiaTensor *
                                   m3x3_transpose(body->orientation);

  body->angularVelocity = body->angularVelocity + body->invWorlInertiaTensor * torque * h;
}

static void integratePositions(float h,
			       rigid_body* body) {
  body->deltaPosition = body->deltaPosition + body->velocity * h;
  quat q = {0.0f,
            body->angularVelocity.x * h * 0.5f,
            body->angularVelocity.y * h * 0.5f,
            body->angularVelocity.z * h * 0.5f};
  body->orientationQuat =
      body->orientationQuat + q * body->orientationQuat;
  body->orientationQuat = quat_normalize(body->orientationQuat);
  //mat3s o = glms_quat_mat3(body->orientationQuat);
  //copyType(o.raw, body->orientation.arr, m3x3);
  body->orientation = m3x3_from_quat(body->orientationQuat);
}

static void storeContactImpulse(contact_point* point,
                                contact_constraint* constraint,
                                u32 constrainNum) {
  *point = constraint->ccp.point;
}

static void finalizePositions(rigid_body* body, f32 h) {
  v3 a = (body->velocity0 - body->velocity) / h;
  body->acceleration = a;

  body->position = body->position + body->deltaPosition;
  body->deltaPosition = V3_ZERO;
  body->velocity0 = body->velocity;
}
