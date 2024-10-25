#define baumgarte 0.02f

#define MAX_CONTACTS 32

static f32 Kdl = 0.3f;        // linear damping factor
static f32 Kda = 0.001f;        // angular damping factor

static vec3s Gravity = {0.0f, 0.0f, -15.0f};

static u16 subStepAmount = 8;

typedef struct rigid_body {
  u32 id;
  f32 mass;
  f32 invMass;
  f32 friction;

  vec3s velocity;
  vec3s velocity0;
  vec3s angularVelocity;

  // Position (center of mass)
  vec3s position;
  vec3s forwardAxis;
  vec3s sideAxis;
  // Body origin (not center of mass)
  vec3s origin;

  // Location of center of mass relative to the body origin
  vec3s localCenter;

  mat3s invBodyInertiaTensor;
  quat orientationQuat;
  i32 shapeNum;

  // auxiliary quantities
  mat3s orientation;
  mat3s invWorlInertiaTensor;
  vec3s deltaPosition;
  vec3s acceleration;
} rigid_body;

typedef struct contact_point {
  vec3s localPointA;
  vec3s localPointB;
  vec3s point;
  vec3s normal;
  f32 separation;
  f32 normalImpulse;
  f32 tangentImpulse[2];
  i32 shapeIdx;

} contact_point;

typedef struct contact_manifold {
  contact_point point;
  rigid_body *bodyA;
  rigid_body *bodyB;
} contact_manifold;

typedef struct joint_base {
  rigid_body *bodyA;
  rigid_body *bodyB;

  u32 id;
  vec3s localOriginAnchorA;
  vec3s localOriginAnchorB;

  // Local anchors relative to center of mass
  vec3s localAnchorA;
  vec3s localAnchorB;

  f32 invMassA;
  f32 invMassB;
  mat3s invIA;
  mat3s invIB;

  // Soft constraint
  f32 herz;
  f32 damping;
  f32 massCoefficient;
  f32 biasCoefficient;
  f32 impulseCoefficient;
} joint_base;

typedef struct hinge_joint {
  joint_base base;

  vec4s localAxisARotation;
  vec4s localAxisBRotation;

  // constraint axii.
  vec3s hingeAxis;

  f32 motorImpulse;
  f32 motorTorque;

  vec2s totalLambda;
  mat2s invEffectiveMass;

} hinge_joint;

typedef struct distance_joint {
  joint_base base;

  // Distance constraint
  vec3s totalImpulse;

  vec3s centerDiff0;
  mat3s invEffectiveMass;
} distance_joint;

typedef struct slider_joint {
  joint_base base;

  vec3s slideAxis;
  f32 rangeMin;
  f32 rangeMax;

  // Distance constraint
  vec2s totalImpulse;
  f32 totalLimitImpulse;

  vec3s centerDiff0;
  mat2s invEffectiveMass;

} slider_joint;

typedef struct axis_joint {
  joint_base base;

  vec3s slideAxis;
  f32 rangeMin;
  f32 rangeMax;

  f32 totalImpulse;

  vec3s centerDiff0;
  f32 invEffectiveMass;

} axis_joint;

enum joint_type {
  joint_type_undefined = 0,
  joint_type_hinge,
  joint_type_distance,
  joint_type_slider,
  joint_type_axis,
};

typedef struct joint {
  joint_type type;
  union {
    hinge_joint hinge;
    distance_joint distance;
    slider_joint slider;
    axis_joint axis;
  };
} joint;

inline void addForceToPoint(vec3s f, vec3s p, vec3s cm, vec3s* forcesIn, vec3s* torquesIn) {
  vec3s force = *forcesIn;
  vec3s torque = *torquesIn;
  force = force + f;
  torque = torque + glms_vec3_cross(p - cm, f);

  *forcesIn = force;
  *torquesIn = torque;
}

inline void calculateSoftConstraintProperties(f32 h, f32 hz, f32 damping,
                                              f32* biasCoeffOut,
                                              f32* impulseCoeffOut,
                                              f32* massCoeffOut) {
  f32 zeta = damping;          // damping
  f32 omega = 2.0f * PI * hz;  // frequency
  f32 biasCoefficient = omega / (2.0f * zeta + h * omega);
  f32 c = h * omega * (2.0f * zeta + h * omega);
  f32 impulseCoefficient = 1.0f / (1.0f + c);
  f32 massCoefficient = c * impulseCoefficient;

  *biasCoeffOut = biasCoefficient;
  *impulseCoeffOut = impulseCoefficient;
  *massCoeffOut = massCoefficient;
}
