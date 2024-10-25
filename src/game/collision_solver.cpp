#include "all.h"
// Soft constraint contact resolver.
// Adapted to Rotten engine from Erin Catto Solver2d library

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
  vec3s normal;
  vec3s tangents[2];
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
  mat3s iA = constraint->bodyA->invWorlInertiaTensor;
  f32 mB = constraint->bodyB->invMass;
  mat3s iB = constraint->bodyB->invWorlInertiaTensor;

  contact_constraint_point* ccp = &constraint->ccp;
  ccp->point = man->point;
  vec3s rA = ccp->point.localPointA;
  vec3s rB = ccp->point.localPointB;

  vec3s normal = constraint->normal;

  ccp->adjustedSeparation = ccp->point.separation - glms_vec3_dot((rB - rA), normal);

  vec3s rnA = glms_vec3_cross(rA, normal);
  vec3s rnB = glms_vec3_cross(rB, normal);
  f32 nm = mA + mB;
  f32 tm[2] = {nm, nm};
  f32 kNormal =
      nm + glms_vec3_dot(rnA, iA * rnA) + glms_vec3_dot(rnB, iB * rnB);
  ccp->normalMass = 1.0f / kNormal;

  for (u32 i = 0; i < 2; i++) {
    vec3s rtA = glms_vec3_cross(constraint->tangents[i], rA);
    vec3s rtB = glms_vec3_cross(constraint->tangents[i], rB);
    tm[i] += glms_vec3_dot(rtA, iA * rtA) + glms_vec3_dot(rtB, iB * rtB);
    ccp->tangentMass[i] = tm[i] > 0.0f ? 1.0f / tm[i] : 0.0f;
  }

  // soft contact
  f32 zeta = 1.0f;                       // damping ratio
  f32 herz = 60.f;                       // cycler per second
  f32 omega = 2.f * PI * herz;           // angular frequency
  f32 shared = 2.0f * zeta + h * omega;  // shared expression
  float c = h * omega * shared;
  ccp->biasCoefficient = omega / shared;  // omega / (2.0f * zeta + omegaDelta);
  ccp->impulseCoefficient = 1.0f / (1.0f + c);
  ccp->massCoefficient = c * ccp->impulseCoefficient;
}

static void warmStart(contact_constraint* constraint, u32 constraintNum) {
  contact_constraint_point* cp = &constraint->ccp;

  vec3s normal = constraint->normal;
  vec3s P = (cp->point.normalImpulse * normal);

  float mA = constraint->bodyA->invMass;
  mat3s iA = constraint->bodyA->invWorlInertiaTensor;

  vec3s vA = constraint->bodyA->velocity;
  vec3s wA = constraint->bodyA->angularVelocity;

  vec3s rA = cp->point.localPointA;

  float mB = constraint->bodyB->invMass;
  mat3s iB = constraint->bodyB->invWorlInertiaTensor;

  vec3s vB = constraint->bodyB->velocity;
  vec3s wB = constraint->bodyB->angularVelocity;

  vec3s rB = cp->point.localPointB;

  for (u32 i = 0; i < 2; i++) {
    P = P + constraint->tangents[i] * cp->point.tangentImpulse[i];
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

static void solve(contact_constraint* constraint, u32 constraintNum, float invH,
                  b32 useBias) {
  contact_constraint_point* cp = &constraint->ccp;

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
  vec3s rA = cp->point.localPointA;
  vec3s rB = cp->point.localPointB;

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
    } else if (useBias) {
      // TODO: original was MAX(cp->biasCoefficient * separation, -4.0f);
      bias = MIN(cp->biasCoefficient * separation, 4.0f);
      massScale = cp->massCoefficient;
      impulseScale = cp->impulseCoefficient;
    }

    f32 vn = glms_vec3_dot(dv, normal);

    // Compute normal impulse
    f32 impulse = -cp->normalMass * massScale * (vn + bias) -
                  impulseScale * cp->point.normalImpulse;
    // Clamp the accumulated impulse
    f32 newImpulse = MAX(cp->point.normalImpulse + impulse, 0.0f);
    impulse = newImpulse - cp->point.normalImpulse;
    cp->point.normalImpulse = newImpulse;

    // Apply contact impulse
    vec3s P = impulse * normal;
    vA = vA - mA * P;
    wA = wA - iA * glms_vec3_cross(rA, P);
    vB = vB + mB * P;
    wB = wB + iB * glms_vec3_cross(rB, P);
  }

  // Friction
  {
    contact_constraint_point* cp = &constraint->ccp;
    for (u32 i = 0; i < 2; ++i) {
      // Compute tangent force
      f32 lambda =
          -glms_vec3_dot(dv, constraint->tangents[i]) * cp->tangentMass[i];

      // Clamp the accumulated force
      f32 maxFriction = friction * cp->point.normalImpulse;
      f32 oldLambda = cp->point.tangentImpulse[i];
      cp->point.tangentImpulse[i] =
          CLAMP(oldLambda + lambda, -maxFriction, maxFriction);
      lambda = cp->point.tangentImpulse[i] - oldLambda;

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

static void integrateVelocities(float h,
				rigid_body* body,
				vec3s extForces,
				vec3s extTorques) {
  // Integrate velocities
  if (body->id == 0) {
    //printf("F %f %f %f\n T %f %f %f\n",extForces.x,extForces.y,extForces.z,extTorques.x,extTorques.y,extTorques.z);
  }
  vec3s cmForce = -Kdl * body->velocity * glms_vec3_norm(body->velocity)
    + Gravity * body->mass
    + extForces;
  // Dampings
  vec3s torque = -Kda * body->angularVelocity + extTorques;

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

static void storeContactImpulse(contact_point* point,
                                contact_constraint* constraint,
                                u32 constrainNum) {
  *point = constraint->ccp.point;
}

static void finalizePositions(rigid_body* body, f32 h) {
  vec3s a = (body->velocity0 - body->velocity) / h;
  body->acceleration = a;

  body->position = body->position + body->deltaPosition;
  body->deltaPosition = GLMS_VEC3_ZERO;
  body->velocity0 = body->velocity;
}
