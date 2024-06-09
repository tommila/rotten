// Soft constraint contact resolver.
// Adapted to Rotten engine from Erin Catto Solver2d library

// SPDX-FileCopyrightText: 2024 Erin Catto
// SPDX-License-Identifier: MIT

#include "physics.h"

typedef struct contact_constraint_point {

  // local anchor relative center of mass
  vec3s localAnchorA, localAnchorB;
  f32 separation;
  f32 adjustedSeparation;
  f32 normalImpulse;
  f32 normalMass;
  f32 tangentImpulse[2];
  f32 tangentMass[2];

  // Soft constraint
  f32 massCoefficient;
  f32 biasCoefficient;
  f32 impulseCoefficient;
} contact_constraint_point;

typedef struct contact_constraint {
  rigid_body* bodyA;
  rigid_body* bodyB;
  contact_constraint_point point;
  u32 manifoldIdx;
  vec3s normal;
  vec3s tangents[2];
  f32 friction;
  b32 touching;
} contact_constraint;

static i32 findOldContact(contact_manifold* manifolds, u32 manifoldNum, i32 contactIdx) {
  for (u32 i = 0; i < manifoldNum; i++) {
    contact_manifold* man = manifolds + i;
    if (man->contactIdx == contactIdx) {
      return i;
    }
  }
  return -1;
}

static void preSolve(contact_manifold* manifolds,
			    contact_constraint* constraints,
			    u32 contactNum,
			    float h)
{
  u16 constraintIdx = 0;
  for (u16 idx = 0; idx < contactNum; idx++) {
    contact_manifold* man = manifolds + idx;
    if (man->contactIdx == -1) {
      continue;
    }
    contact_constraint* constraint = constraints + constraintIdx++;
    constraint->bodyA = man->bodyA;
    constraint->bodyB = man->bodyB;
    constraint->manifoldIdx = idx;
    constraint->normal = man->normal;
    constraint->tangents[0] = man->tangents[0];
    constraint->tangents[1] = man->tangents[1];

    constraint->friction = sqrtf(man->bodyA->friction + man->bodyB->friction);

    f32 mA = constraint->bodyA->invMass;
    mat3s iA = constraint->bodyA->invWorlInertiaTensor;
    f32 mB = constraint->bodyB->invMass;
    mat3s iB = constraint->bodyB->invWorlInertiaTensor;

    contact_manifold_point* mp = &man->point;
    contact_constraint_point* cp = &constraint->point;

    cp->normalImpulse = mp->normalImpulse;
    cp->tangentImpulse[0] = mp->tangentImpulse[0];
    cp->tangentImpulse[1] = mp->tangentImpulse[1];

    cp->localAnchorA = mp->localPointA;
    cp->localAnchorB = mp->localPointB;

    vec3s rA = cp->localAnchorA;
    vec3s rB = cp->localAnchorB;

    vec3s normal = constraint->normal;

    cp->separation = mp->separation;
    cp->adjustedSeparation = mp->separation - glms_vec3_dot((rB - rA), normal);

    vec3s rnA = glms_vec3_cross(rA, normal);
    vec3s rnB = glms_vec3_cross(rB, normal);
    f32 nm = mA + mB;
    f32 tm[2] = {nm, nm};
    f32 kNormal = nm + glms_vec3_dot(rnA, iA * rnA ) + glms_vec3_dot(rnB, iB * rnB );
    cp->normalMass = 1.0f / kNormal;

    for (u32 i = 0; i < 2; i++) {
      vec3s rtA = glms_vec3_cross(constraint->tangents[i], rA);
      vec3s rtB = glms_vec3_cross(constraint->tangents[i], rB);
      tm[i] += glms_vec3_dot(rtA, iA * rtA) + glms_vec3_dot(rtB, iB * rtB);
      cp->tangentMass[i] = tm[i] > 0.0f ? 1.0f / tm[i] : 0.0f;
    }

    // soft contact
    f32 zeta = 1.0f;                      // damping ratio
    f32 herz = 20.f;                      // cycler per second
    f32 omega = 2.f * PI * herz;           // angular frequency
    f32 shared = 2.0f * zeta + h * omega;  // shared expression
    float c = h * omega * shared;
    cp->biasCoefficient = omega / shared;  // omega / (2.0f * zeta + omegaDelta);
    cp->impulseCoefficient = 1.0f / (1.0f + c);
    cp->massCoefficient = c * cp->impulseCoefficient;
  }
}

static void warmStart(contact_constraint* constraints,
			      u32 constraintNum) {
  for (u32 i = 0; i < constraintNum; ++i) {
    contact_constraint* constraint = constraints + i;
    contact_constraint_point* cp = &constraint->point;

    vec3s normal = constraint->normal;
    vec3s P = (cp->normalImpulse * normal);

    float mA = constraint->bodyA->invMass;
    mat3s iA = constraint->bodyA->invWorlInertiaTensor;

    vec3s vA = constraint->bodyA->velocity;
    vec3s wA = constraint->bodyA->angularVelocity;

    vec3s rA = cp->localAnchorA;

    float mB = constraint->bodyB->invMass;
    mat3s iB = constraint->bodyB->invWorlInertiaTensor;

    vec3s vB = constraint->bodyB->velocity;
    vec3s wB = constraint->bodyB->angularVelocity;

    vec3s rB = cp->localAnchorB;

    for (u32 i = 0; i < 2; i++) {
      P = P + constraint->tangents[i] * cp->tangentImpulse[i];
    }

    vA = vA - mA * P;
    wA = wA - iA * glms_vec3_cross(rA, P);

    vB = vB + mB * P;
    wB = wB + iB * glms_vec3_cross(rB, P);

    constraint->bodyA->velocity = vA;
    constraint->bodyA->angularVelocity = wA;
    constraint->bodyB->velocity = vB;
    constraint->bodyB->angularVelocity = wB;
  }
}

static void solve(contact_constraint* constraints,
			  u32 constraintNum,
			  float invH,
			  b32 useBias)
{
  for (u32 i = 0; i < constraintNum; ++i) {
    contact_constraint* constraint = constraints + i;
    contact_constraint_point* cp = &constraint->point;

    float mA = constraint->bodyA->invMass;
    mat3s iA = constraint->bodyA->invWorlInertiaTensor;

    vec3s vA = constraint->bodyA->velocity;
    vec3s wA = constraint->bodyA->angularVelocity;

    vec3s dcA = constraint->bodyA->deltaPosition;

    float mB = constraint->bodyB->invMass;
    mat3s iB = constraint->bodyB->invWorlInertiaTensor;

    vec3s vB = constraint->bodyB->velocity;
    vec3s wB = constraint->bodyB->angularVelocity;

    vec3s dcB = constraint->bodyB->deltaPosition;

    vec3s normal = constraint->normal;

    float friction = constraint->friction;

    // compute current separation
    vec3s rA = cp->localAnchorA;
    vec3s rB = cp->localAnchorB;

    // Relative velocity at contact
    vec3s vrA = glms_vec3_cross(wA, rA);
    vec3s vrB = glms_vec3_cross(wB, rB);

    vec3s dv = (vB + vrB) - (vA + vrA);
    // Normal
    {

      vec3s ds = (dcB - dcA) + (rB - rA);
      f32 separation = glms_vec3_dot(ds, normal) + cp->adjustedSeparation;

      f32 bias = 0.0f;
      f32 massScale = 1.0f;
      f32 impulseScale = 0.0f;
      if (separation > 0.0f) {
        // Speculative
        bias = separation * invH;
      }
      else if (useBias) {
	// TODO: original was MIN(cp->biasCoefficient * s, 4.0f)
        bias = MIN(cp->biasCoefficient * separation, 4.0f);
        massScale = cp->massCoefficient;
        impulseScale = cp->impulseCoefficient;
      }

      f32 vn = glms_vec3_dot(dv, normal);

      // Compute normal impulse
      f32 impulse = -cp->normalMass * massScale * (vn + bias) -
	impulseScale * cp->normalImpulse;
      // Clamp the accumulated impulse
      f32 newImpulse = MAX(cp->normalImpulse + impulse, 0.0f);
      impulse = newImpulse - cp->normalImpulse;
      cp->normalImpulse = newImpulse;

      // Apply contact impulse
      vec3s P = impulse * normal;
      vA = vA - mA * P;
      wA = wA - iA * glms_vec3_cross(rA, P);
      vB = vB + mB * P;
      wB = wB + iB * glms_vec3_cross(rB, P);
    }

    // Friction
    if(1){
      contact_constraint_point* cp = &constraint->point;
      // vec3s rA = cp->localAnchorA;
      // vec3s rB = cp->localAnchorB;

      // vec3s vrA = glms_vec3_cross(wA, rA);
      // vec3s vrB = glms_vec3_cross(wB, rB);

      // vec3s dv = vA + vrA - vB - vrB;

      for (u32 i = 0; i < 2; ++i) {
        // Compute tangent force
        f32 lambda = -glms_vec3_dot(dv, constraint->tangents[i]) * cp->tangentMass[i];

        // Clamp the accumulated force
        f32 maxFriction = friction * cp->normalImpulse;
	f32 oldLambda = cp->tangentImpulse[i];
        cp->tangentImpulse[i] =
            CLAMP(oldLambda + lambda, -maxFriction, maxFriction);
        lambda = cp->tangentImpulse[i] - oldLambda;

        // Apply contact impulse
        vec3s impulse = constraint->tangents[i] * lambda;

        vA = vA - mA * impulse;
        wA = wA - iA * glms_vec3_cross(rA, impulse);
        vB = vB + mB * impulse;
        wB = wB + iB * glms_vec3_cross(rB, impulse);
      }
    }

    constraint->bodyA->velocity = vA;
    constraint->bodyA->angularVelocity = wA;
    constraint->bodyB->velocity = vB;
    constraint->bodyB->angularVelocity = wB;
  }
}

static void integrateVelocities(float h,
				rigid_body* body,
				vec3s extForces) {
  // Integrate velocities
  vec3s cmForce = -Kdl * body->velocity * glms_vec3_norm(body->velocity) +
    Gravity * body->mass + extForces;
  // Dampings
  vec3s torque = -Kda * body->angularVelocity;

  // vCM = vCM + h * (FT/M)
  vec3s acc = body->invMass * cmForce;
  body->velocity = body->velocity + acc * h;

  // ICM-1 = A * _I-1 * AT
  body->invWorlInertiaTensor = body->orientation *
                                   body->invBodyInertiaTensor *
                                   glms_mat3_transpose(body->orientation);

  body->angularVelocity = body->angularVelocity + body->invWorlInertiaTensor * torque * h;
}

static void integratePositions(float h,
			       rigid_body* body) {
  body->deltaPosition = body->deltaPosition + body->velocity * h;
  // Compute auxiliary quantities
  quat q = {body->angularVelocity.x * h * 0.5f,
            body->angularVelocity.y * h * 0.5f,
            body->angularVelocity.z * h * 0.5f, 0.0f};

  body->orientationQuat =
      glms_quat_add(body->orientationQuat, q * body->orientationQuat);
  body->orientationQuat = glms_quat_normalize(body->orientationQuat);

  body->orientation = glms_quat_mat3(body->orientationQuat);
}

static void storeContactImpulse(contact_manifold* manifolds,
                                contact_constraint* constraints,
				u32 constrainNum) {
  for (u32 i = 0; i < constrainNum; ++i) {
    contact_constraint* constraint = constraints + i;
    contact_manifold* man = manifolds + constraint->manifoldIdx;

    man->point.normalImpulse = constraint->point.normalImpulse;
    man->point.tangentImpulse[0] = constraint->point.tangentImpulse[0];
    man->point.tangentImpulse[1] = constraint->point.tangentImpulse[1];
  }
}

static void finalizePositions(rigid_body* body) {
  body->position = body->position + body->deltaPosition;
  body->deltaPosition = GLMS_VEC3_ZERO;
}
