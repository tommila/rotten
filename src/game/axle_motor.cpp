#include "physics.h"


inline void syncWheelAngularSpeed(hinge_joint* leftWheelJoint,
				  hinge_joint* rightWheelJoint,
				  b32 accelerate) {

  // rigid_body* carBody = axleJoint->body;
  rigid_body* leftWheel = leftWheelJoint->bodyB;
  rigid_body* rightWheel = rightWheelJoint->bodyB;

  f32 CdotA = glms_vec3_dot(leftWheel->angularVelocity, leftWheelJoint->mA1);
  f32 CdotB = glms_vec3_dot(rightWheel->angularVelocity, rightWheelJoint->mA1);

  leftWheel->angularVelocity = accelerate ? (CdotA > CdotB)
    ? leftWheel->angularVelocity : rightWheel->angularVelocity
    : (CdotA < CdotB) ? leftWheel->angularVelocity : rightWheel->angularVelocity;
  rightWheel->angularVelocity = accelerate
    ? (CdotA > CdotB) ? leftWheel->angularVelocity : rightWheel->angularVelocity
    : (CdotA < CdotB) ? leftWheel->angularVelocity : rightWheel->angularVelocity;
}
