#define baumgarte 0.02f

#define MAX_CONTACTS 32

static f32 Kdl = 0.4257f;        // linear damping factor
static f32 Krr = Kdl * 30.f;
static f32 Kda = 0.05f;        // angular damping factor

static v3 Gravity = {0.0f, 0.0f, -15.0f};

static u16 subStepAmount = 4;

typedef struct rigid_body {
  u32 id;
  f32 mass;
  f32 invMass;
  f32 friction;

  v3 velocity;
  v3 velocity0;
  v3 angularVelocity;

  // Position (center of mass)
  v3 position;
  v3 forwardAxis;
  v3 sideAxis;
  // Body origin (not center of mass)
  v3 origin;

  // Location of center of mass relative to the body origin
  v3 localCenter;

  m3x3 invBodyInertiaTensor;
  quat orientationQuat;
  i32 shapeNum;

  // auxiliary quantities
  m3x3 orientation;
  m3x3 invWorlInertiaTensor;
  v3 deltaPosition;
  v3 acceleration;
} rigid_body;

typedef struct contact_point {
  v3 localPointA;
  v3 localPointB;
  v3 point;
  v3 normal;
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
  v3 localOriginAnchorA;
  v3 localOriginAnchorB;

  // Local anchors relative to center of mass
  v3 localAnchorA;
  v3 localAnchorB;

  f32 invMassA;
  f32 invMassB;
  m3x3 invIA;
  m3x3 invIB;

  // Soft constraint
  f32 herz;
  f32 damping;
  f32 massCoefficient;
  f32 biasCoefficient;
  f32 impulseCoefficient;
} joint_base;

typedef struct hinge_joint {
  joint_base base;

  v4 localAxisARotation;
  v4 localAxisBRotation;

  // constraint axii.
  v3 hingeAxis;

  f32 motorImpulse;
  f32 motorTorque;

  v2 totalLambda;
  m2x2 invEffectiveMass;

} hinge_joint;

typedef struct distance_joint {
  joint_base base;

  // Distance constraint
  v3 totalImpulse;

  v3 centerDiff0;
  m3x3 invEffectiveMass;
} distance_joint;

typedef struct slider_joint {
  joint_base base;

  v3 slideAxis;
  f32 rangeMin;
  f32 rangeMax;

  // Distance constraint
  v2 totalImpulse;
  f32 totalLimitImpulse;

  v3 centerDiff0;
  m2x2 invEffectiveMass;

} slider_joint;

typedef struct axis_joint {
  joint_base base;

  v3 slideAxis;

  f32 totalImpulse;

  v3 centerDiff0;
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

inline void addForceToPoint(v3 f, v3 p, v3 cm, v3* forcesIn, v3* torquesIn) {
  v3 force = *forcesIn;
  v3 torque = *torquesIn;
  force = force + f;
  torque = torque + v3_cross(p - cm, f);
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
