#include "all.h"

static void jointPreSolve(joint* j, f32 h) {
  ASSERT(j->type);
  switch (j->type) {
    case joint_type_hinge:
      preSolveHingeJoint(&j->hinge, h);
      break;
    case joint_type_distance:
      preSolveDistanceJoint(&j->distance, h);
      break;
    case joint_type_slider:
      preSolveSliderJoint(&j->slider, h);
      break;
    case joint_type_axis:
      preSolveAxisJoint(&j->axis, h);
      break;
      InvalidDefaultCase;
  };
}

static void jointSolve(joint* j, f32 h, f32 invH,
			    b32 useBias) {
  ASSERT(j->type);
  switch (j->type) {
    case joint_type_hinge:
      solveHingeJoint(&j->hinge, h, invH, useBias);
      break;
    case joint_type_distance:
      solveDistanceJoint(&j->distance, h, invH, useBias);
      break;
    case joint_type_slider:
      solveSliderJoint(&j->slider, h, invH, useBias);
      break;
    case joint_type_axis:
      solveAxisJoint(&j->axis, h, invH, useBias);
      break;
      InvalidDefaultCase;
  };
}

static void jointWarmStart(joint* j) {
  ASSERT(j->type);
  switch (j->type) {
    case joint_type_hinge:
      warmStartHingeJoint(&j->hinge);
      break;
    case joint_type_distance:
      warmStartDistanceJoint(&j->distance);
      break;
    case joint_type_slider:
      warmStartSliderJoint(&j->slider);
      break;
    case joint_type_axis:
      warmStartAxisJoint(&j->axis);
      break;
      InvalidDefaultCase;
  };
}
