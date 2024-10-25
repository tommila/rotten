#include "all.h"

#define VERSION "v0.0.2"

static loggerPtr LOG;

#define ASSERT(cond) ASSERT_(cond, #cond, __FUNCTION__, __LINE__, __FILE__)
static assertPtr ASSERT_;

static void* (*MALLOC)(usize size);
static void* (*REALLOC)(void *p, usize newSize);
static void (*FREE)(void *p);

struct memory_arena;

#define copyType(from, to, type) memcpy(to, from, sizeof(type))

#define setButtonState(b) b ? \
  button_state_pressed : button_state_released

enum button_state {
  button_state_up = 0,
  button_state_released = 1,
  button_state_pressed = 2,
  button_state_held = 3
};

typedef struct mouse_state {
  button_state button[3];
  u32 position[2];
  i32 movement[2];
  i32 wheel;
} mouse_state;

typedef struct input_state {
  union {
    struct {
      button_state moveUp;
      button_state moveDown;
      button_state moveLeft;
      button_state moveRight;
      button_state reset;
      button_state quit;
    };
    button_state buttons[11];
  };
  mouse_state mouse;
  b32 pausePhysics;
} input_state;

// Camera
typedef struct camera_state {
  v3 position;
  f32 yaw;
  f32 pitch;
  u32 fov;
} camera_state;

typedef struct mesh_data {
  f32* vertexData;
  u32* indices;

  usize vertexDataSize;
  usize indexDataSize;

  u32 vertexNum;
  u32 vertexComponentNum;
  u32 indexNum;
} mesh_data;

typedef struct mesh_material_data {
  char imagePath[64];
  v4 baseColor;
} mesh_material_data;

typedef struct index_data {
  u32 elementNum;
  i32 baseElement;
  i32 baseVertex;
  u32 childNum;
} index_data;

typedef struct model_material_data {
  rt_image_handle textureHandle;
  rt_sampler_handle samplerHandle;
  v4 baseColorValue;
} mode_material_data;

typedef struct terrain_object {
  struct {
    rt_vertex_array_handle vertexArrayHandle;
    rt_vertex_buffer_handle vertexBufferHandle;
    rt_index_buffer_handle indexBufferHandle;
    rt_image_handle terrainTexHandle;
    rt_image_handle heightMapTexHandle;
    rt_sampler_handle terrainSamplerHandle;
    rt_sampler_handle heightMapSamplerHandle;
    rt_shader_program_handle programHandle;
    u32 elementNum;
    u32 vsUniformSize;
  } terrain_model;
  struct {
    rt_vertex_buffer_handle vertexBufferHandle;
    rt_index_buffer_handle indexBufferHandle;
    rt_vertex_array_handle vertexArrayHandle;
    rt_shader_program_handle programHandle;
    u32 elementNum;
  } geometry_model;
  rt_image_data heightMapImg;
  v4 *geometry;
  b32 initialized;
} terrain_object;

typedef struct skybox_object {
  rt_vertex_buffer_handle vertexBufferHandle;
  rt_index_buffer_handle indexBufferHandle;
  rt_vertex_array_handle vertexArrayHandle;

  rt_image_handle texHandle;
  rt_sampler_handle samplerHandle;
  rt_shader_program_handle programHandle;
  u32 elementNum;
  u32 vsUniformSize;
  b32 initialized;
} skybox_object;

typedef struct model_data {
  rt_vertex_buffer_handle vertexBufferHandle;
  rt_index_buffer_handle indexBufferHandle;
  rt_vertex_array_handle vertexArrayHandle;

  index_data meshData[32];
  model_material_data matData[32];
  m4x4 transform[32];
  u32 meshNum;
} model_data;

typedef struct car_properties {
  f32 chassisMass;
  f32 wheelMass;
  f32 chassisFriction;
  f32 frontWheelBaseFriction;
  f32 rearWheelBaseFriction;
  f32 slipRatioForceCurve[6]; 
  f32 slipRatioForceCoeff;
  f32 slipAngleForceCurve[6]; 
  f32 slipAngleForceCoeffFW;
  f32 slipAngleForceCoeffRW;

  f32 motorTorqueCurve[6];
  f32 motorTorque;
  f32 gearRatios[6];
  i32 gearNum;
  v3 localCenter;
  shape_box chassisShape;
  shape_sphere wheelShape;
  f32 suspensionPosHeight;
  f32 suspensionHz;
  f32 suspensionDamping;
  f32 engineInertia;
  f32 differentialRatio;
  f32 transmissionEfficiency;
} car_properties;

typedef f32 audio_filter_state[4];

typedef struct car_audio_state{
  f64 engineCycle;
  f64 windCycle;
  i32 prevRpm;
  audio_filter_state intakeFilter;
  audio_filter_state engineFilter;
  audio_filter_state gravelFilter;
  audio_filter_state windFilter;
  i32 sampleIndex;
} audio_state;

typedef struct car_state {
  car_properties properties;
  model_data chassisModel;
  model_data wheelModel[4];
  rt_shader_program_handle programHandle;
  car_audio_state audioState;
  union {
    rigid_body bodies[5];
    struct {
      rigid_body chassis;
      rigid_body wheels[4];
    };
  } body;
  shape bodyShapes[12];
  union {
    joint jointsAll[12];
    struct {
      joint wheelHinge;
      joint wheelSuspension;
      joint wheelSuspensionLimits;
    }joints[4];
  };
  struct {
    f32 frictionAdjustment[4];
    f32 slipAngle[4];
    f32 slipRatio[4];
    f32 motorTorque;
    f32 rpm;
    i32 gear;
    b32 accelerating;
    f32 gearShiftT;
  } stats; 
  contact_point contactPointStorage[MAX_CONTACTS];
  u32 contactPointNum;
  f32 turnAngle;
  b32 initialized;
} car_state;

#define isBitSet(drawState, bit) (drawState & bit)

#define setBit(drawState, bit, set) (set ? (drawState | bit) : (drawState & ~bit))

#define toggleBit(drawState, bit) (drawState ^ bit)

enum visibility_state_bit {
  visibility_state_car              = 1 << 0,
  visibility_state_car_colliders    = 1 << 1,
  visibility_state_terrain          = 1 << 2,
  visibility_state_terrain_geometry = 1 << 3,
  visibility_state_geometry_normals = 1 << 4,
  visibility_state_slip_angle       = 1 << 4,
};

typedef struct debug_draw_state {
  b32 freeCameraView;
  b32 drawDebugPanel;
  u16 visibilityState;
  v4 tireFrictions;
} debug_draw_state;

enum profiler_counter_entry {
  profiler_counter_entry_simulation,
  profiler_counter_entry_rendering,
  profiler_counter_entry_ui,
  profiler_counter_entry_audio,
  profiler_counter_entry_total,
  _profiler_counter_entry_num,
};

typedef struct profiler_state {
  u64 accumulated[_profiler_counter_entry_num];
  u64 prevAccumulated[_profiler_counter_entry_num];
  u64 counter[_profiler_counter_entry_num]; 
  f64 average[_profiler_counter_entry_num]; 
  f32 elapsedTime;
} profiler_state;


#define profilerBegin(profiler, entry) \
  profiler.counter[profiler_counter_entry_##entry] = platformApi->getPerformanceCounter()
#define profilerEnd(profiler, entry) \
  profiler.accumulated[profiler_counter_entry_##entry] += \
  platformApi->getPerformanceCounter() - profiler.counter[profiler_counter_entry_##entry]

#define profilerAverage(profiler, delta, stepSec) \
  profiler.elapsedTime += delta; \
  if (profiler.elapsedTime > stepSec) { \
    u64 freq = platformApi->getPerformanceFrequency(); \
    profiler.average[profiler_counter_entry_total] = 0.f; \
    for(i32 i = 0; i < profiler_counter_entry_total; i++) { \
      profiler.average[i] = 1000.f * (f64)(profiler.accumulated[i] - profiler.prevAccumulated[i]) /\
        (stepSec * 60.0) / freq; \
      profiler.prevAccumulated[i] = profiler.accumulated[i]; \
      profiler.average[profiler_counter_entry_total] += profiler.average[i]; \
    } \
    profiler.elapsedTime = profiler.elapsedTime - stepSec; \
  }

enum debug_state {
  game_state_intro = 0,
  game_state_game,
  _game_state_num
};

typedef struct car_game_state {
  b32 initialized;
  debug_state state;
  rt_audio_data ambient;
  input_state input;
  camera_state camera;
  skybox_object skybox;
  terrain_object terrain;
  car_state car;
  debug_draw_state debug;
  profiler_state profiler;
} car_game_state;

static platform_api *platformApi = NULL;
rt_shader_data importShader(memory_arena *arena, const char *shaderVsFilePath,
                         const char *shaderFsFilePath);
