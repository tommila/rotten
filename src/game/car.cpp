#include "all.h"

// TODO: Pull out collision testing/solving code

#define WHEEL_NUM 4
#define RIGID_BODY_NUM 1 + WHEEL_NUM

typedef struct vs_uniform_params {
  m4x4 modelMat;
  m4x4 viewMat;
  m4x4 projMat;
} vs_uniform_params;

typedef struct read_gltf_node_result {
  i32 meshNum;
  usize vertexTotalSize;
  usize indexTotalSize;
} read_gltf_node_result;

car_properties carProperties = {
  .chassisMass = 1400.f,
  .wheelMass   = 40.f,
  .chassisFriction = 0.6f,
  .frontWheelBaseFriction = 0.8f,
  .rearWheelBaseFriction = 0.9f,
  .slipRatioForceCurve = {
    0.f,
    0.5f,
    0.5f,
    0.5f,
    0.2f,
    0.2f
  },
  .slipRatioForceCoeff = 0.2f,
  .slipAngleForceCurve = {
    0.60f,
    0.70f,
    0.70f,
    0.70f,
    0.70f,
    0.2f
  },
  .slipAngleForceCoeffFW = 0.4f,
  .slipAngleForceCoeffRW = 0.4f,
  .motorTorqueCurve = {
    0.8f,
    0.9f,
    1.0f,
    1.0f,
    0.9f,
    0.6f
  },
  .motorTorque = 450.f,
  .gearRatios = {2.2f, 1.0f, 0.7f, 0.5f, 0.4f},
  .gearNum = 5,
  .localCenter = {0.5f, 0.0f, -0.2f},
  .chassisShape = 
  createBoxShape((v3){-2.15f, -0.8f, 0.4}, (v3){2.15f, 0.8f, 1.2f}),
  .wheelShape = {.p = {0.0f,0.0f}, .radius = 0.3f},
  .suspensionPosHeight = -0.2f,
  .suspensionHz = 10.f,
  .suspensionDamping = 1.0f,
  .engineInertia = 0.05f,
  .differentialRatio = 4.0f,
  .transmissionEfficiency = 1.0f
};

static read_gltf_node_result readGltfNodeData(memory_arena* tempArena,
                                              mesh_data* meshData,
                                              mesh_material_data* matData,
                                              m4x4* transform,
                                              i32* childNum,
                                              cgltf_node* node) {
  read_gltf_node_result result = {0};
  if(node->mesh && node->mesh->primitives_count) {
    gltf_readMeshData(tempArena, node,
                      "./assets/models/low_poly_car/scene.gltf", meshData,
                      transform);
    gltf_readMatData(node, "./assets/models/low_poly_car/scene.gltf", matData);

    *childNum = node->children_count;
    result.meshNum++;
    result.vertexTotalSize += meshData->vertexDataSize;
    result.indexTotalSize += meshData->indexDataSize;
  }

  for (cgltf_size childIndex = 0; childIndex < node->children_count;
  ++childIndex) {
    // Clean this up
    read_gltf_node_result childResult =
      readGltfNodeData(tempArena,
                       meshData + result.meshNum,
                       matData + result.meshNum,
                       transform + result.meshNum,
                       childNum + result.meshNum,
                       node->children[childIndex]);

    result.meshNum += childResult.meshNum;
    result.vertexTotalSize += childResult.vertexTotalSize;
    result.indexTotalSize += childResult.indexTotalSize;
  }
  return result;
}

static void createModelData(memory_arena* tempArena,
                            rt_command_buffer* rendererBuffer,
                            model_data* model,
                            cgltf_node* node) {

  mesh_data *meshData = pushArray(tempArena, 32, mesh_data);
  mesh_material_data *matData = pushArray(tempArena, 32, mesh_material_data);
  m4x4 transform[32];
  i32 childNum[32];

  read_gltf_node_result gltfReadResult =
    readGltfNodeData(tempArena, meshData, matData, transform, childNum, node);

  model->meshNum = gltfReadResult.meshNum;

  if (model->vertexArrayHandle == 0) {
    rt_command_create_vertex_buffer* cmd = rt_pushRenderCommand(
      rendererBuffer, create_vertex_buffer);

    cmd->vertexData = NULL;
    cmd->indexData = NULL;
    cmd->vertexDataSize = gltfReadResult.vertexTotalSize;
    cmd->indexDataSize = gltfReadResult.indexTotalSize;
    cmd->isStreamData = false;

    cmd->vertexBufHandle = &model->vertexBufferHandle;
    cmd->indexBufHandle = &model->indexBufferHandle;
    cmd->vertexArrHandle = &model->vertexArrayHandle;

     cmd->vertexAttributes[0].count = 3;
     cmd->vertexAttributes[0].offset = 0;
     cmd->vertexAttributes[0].stride = 6 * sizeof(f32);
     cmd->vertexAttributes[0].type = rt_data_type_f32;
     cmd->vertexAttributes[1].count = 3;
     cmd->vertexAttributes[1].offset = 3 * sizeof(f32);
     cmd->vertexAttributes[1].stride = 6 * sizeof(f32);
     cmd->vertexAttributes[1].type = rt_data_type_f32;

    // Flush command buffer as we need for the buffer ids to be ready.
    // They are needed for the data update that happens next
    platformApi->flushCommandBuffer(rendererBuffer);
  }

  i32 baseElement = 0;
  i32 baseVertex = 0;

  for (i32 meshIdx = 0; meshIdx < gltfReadResult.meshNum; meshIdx++) {
    mesh_data* subMeshData = meshData + meshIdx;
    mesh_material_data* subMatData = matData + meshIdx;
    copyType(transform + meshIdx, model->transform[meshIdx].arr, m4x4);

    index_data* subModelMeshData = model->meshData + meshIdx;
    model_material_data* subModelMaterialData = model->matData + meshIdx;

    rt_command_update_vertex_buffer* cmd = rt_pushRenderCommand(
      rendererBuffer, update_vertex_buffer);
    cmd->vertexData = subMeshData->vertexData;
    cmd->indexData = subMeshData->indices;
    cmd->vertexDataSize = subMeshData->vertexDataSize;

    cmd->indexDataSize = subMeshData->indexDataSize;
    cmd->vertexBufHandle = model->vertexBufferHandle;
    cmd->indexBufHandle = model->indexBufferHandle;

    cmd->vertexDataOffset = baseVertex * subMeshData->vertexComponentNum * sizeof(f32);
    cmd->indexDataOffset = baseElement * sizeof(u32);

    subModelMeshData->elementNum = subMeshData->indexNum;
    subModelMeshData->baseElement = baseElement;
    subModelMeshData->baseVertex = baseVertex;
    subModelMeshData->childNum = childNum[meshIdx];

    subModelMaterialData->baseColorValue = subMatData->baseColor;

    baseElement += subMeshData->indexNum;
    baseVertex += subMeshData->vertexNum;

    platformApi->flushCommandBuffer(rendererBuffer);
  }
}

static void carCreateModel(memory_arena* permanentArena, memory_arena* tempArena,
                           rt_command_buffer* rendererBuffer,
                           car_game_state* game, utime assetModTime) {
  // Model mesh
  car_state* car = &game->car;
  utime modelModTime =
    platformApi->readFileModTime("./assets/models/low_poly_car/scene.gltf");
  if (assetModTime < modelModTime) {
    cgltf_data* gltfData =
      gltf_loadFile(tempArena, string8("./assets/models/low_poly_car/scene.gltf"));
    car->chassisModel.meshNum = 0;
    car->wheelModel[0].meshNum = 0;
    car->wheelModel[1].meshNum = 0;
    car->wheelModel[2].meshNum = 0;
    car->wheelModel[3].meshNum = 0;

    // scene->nodes_count is the amount that root node has, not the total
    for (cgltf_size nodeIndex = 0; nodeIndex < gltfData->nodes_count;
    nodeIndex++) {
      cgltf_node* node = gltfData->nodes + nodeIndex;
      if (strcmp(node->name, "Hull") == 0) {
        createModelData(tempArena, rendererBuffer, &car->chassisModel, node);
      } else if (strcmp(node->name, "WheelBL") == 0) {
        createModelData(tempArena, rendererBuffer, &car->wheelModel[0], node);
      } else if (strcmp(node->name, "WheelBR") == 0) {
        createModelData(tempArena, rendererBuffer, &car->wheelModel[1], node);
      } else if (strcmp(node->name, "WheelFL") == 0) {
        createModelData(tempArena, rendererBuffer, &car->wheelModel[2], node);
      } else if (strcmp(node->name, "WheelFR") == 0) {
        createModelData(tempArena, rendererBuffer, &car->wheelModel[3], node);
      }
    }
  }

  // Shaders
  utime vsShaderModTime =
    platformApi->readFileModTime("./assets/shaders/car.vs");
  utime fsShaderModTime =
    platformApi->readFileModTime("./assets/shaders/car.fs");
  if (assetModTime < vsShaderModTime || assetModTime < fsShaderModTime) {
    rt_shader_data carShader = importShader(tempArena, "./assets/shaders/car.vs",
                                         "./assets/shaders/car.fs");
    if (car->programHandle == 0) {
      rt_command_create_shader_program* cmd = rt_pushRenderCommand(
        rendererBuffer, create_shader_program);

      cmd->fragmentShaderData = carShader.fsData;
      cmd->vertexShaderData = carShader.vsData;
      cmd->shaderProgramHandle = &car->programHandle;
    } else {
      rt_command_update_shader_program* cmd = rt_pushRenderCommand(
        rendererBuffer, update_shader_program);

      cmd->fragmentShaderData = carShader.fsData;
      cmd->vertexShaderData = carShader.vsData;
      cmd->shaderProgramHandle = &car->programHandle;
    }
  }
}

static void carSetInitialState(car_game_state* game) {
  car_state* car = &game->car;
  car->properties = carProperties;
  car->body.chassis.id = 0;
  car->body.chassis.forwardAxis = V3_X_UP;
  car->body.chassis.orientation = M3X3_IDENTITY;
  car->body.chassis.orientationQuat = QUAT_IDENTITY;
  car->body.chassis.angularVelocity = (v3){0.f, 0.f, 0.f};
  car->body.chassis.localCenter = carProperties.localCenter;
  car->body.chassis.position = (v3){-27.175f, -150.252f, -44.260f};
  car->body.chassis.origin =
    car->body.chassis.position -
    car->body.chassis.orientation * car->body.chassis.localCenter;

  for (i32 i = 0; i < WHEEL_NUM; i++) {
    car->body.wheels[i].orientation = M3X3_IDENTITY;
    car->body.wheels[i].orientationQuat = QUAT_IDENTITY;
    car->body.wheels[i].id = 1 + i;

    v3 offset = (v3){0.f, 0.f, carProperties.suspensionPosHeight} - car->body.chassis.localCenter;
    v3 pivot = car->wheelModel[i].transform[0] * offset;
    car->body.wheels[i].position =
      pivot + car->body.chassis.position;

    car->body.wheels[i].origin =
      car->body.wheels[i].position -
      car->body.wheels[i].localCenter * car->body.wheels[i].orientation;
  }
}

static void carSetupBody(car_game_state* game) {
  car_state* car = &game->car;
  shape* chassisShape = &car->bodyShapes[0];
  chassisShape->type = shape_type::SHAPE_BOX;
  shape_box box = carProperties.chassisShape;
  v3 boxVertices[8];
  u32 shapeNum = arrayLen(boxVertices);
  decomposeBoxShape(box, (f32*)boxVertices);
  for (u32 i = 0; i < shapeNum; i++) {
    car->bodyShapes[i].type = shape_type::SHAPE_POINT;
    car->bodyShapes[i].point.p = boxVertices[i];
  }
  car->body.chassis.shapeNum = shapeNum;
  v3 size = box.size;
  f32 mass = carProperties.chassisMass;  // kg

  car->body.chassis.localCenter = carProperties.localCenter;
  car->body.chassis.mass = mass;
  car->body.chassis.invMass = 1.0f / mass;

  // Solid cuboid
  car->body.chassis.invBodyInertiaTensor.m00 =
    (1.f / 8.f) * mass * (size.y * size.y + size.z * size.z);
  car->body.chassis.invBodyInertiaTensor.m11 =
    (1.f / 8.f) * mass * (size.x * size.x + size.z * size.z);
  car->body.chassis.invBodyInertiaTensor.m22 =
    (1.f / 8.f) * mass * (size.x * size.x + size.y * size.y);

  car->body.chassis.invBodyInertiaTensor =
    m3x3_inverse(car->body.chassis.invBodyInertiaTensor);

  car->body.chassis.friction = 0.3f;
  m3x3 bI = car->body.chassis.invBodyInertiaTensor;
  car->body.chassis.invWorlInertiaTensor =
    car->body.chassis.orientation * bI *
    m3x3_transpose(car->body.chassis.orientation);

  // Car wheels body setup
  for (u32 i = 0; i < WHEEL_NUM; i++) {
    shape* sphere = &car->bodyShapes[car->body.chassis.shapeNum + i];
    sphere->type = shape_type::SHAPE_SPHERE;
    sphere->sphere = carProperties.wheelShape;
    f32 wheelRadius = sphere->sphere.radius;
    f32 mass = carProperties.wheelMass;  // kg
    v3 offset = (v3){0.f, 0.f, carProperties.suspensionPosHeight} - car->body.chassis.localCenter;
    v3 pivot = car->wheelModel[i].transform[0] * offset;
    car->body.wheels[i].mass = mass;
    car->body.wheels[i].invMass = 1.0f / mass;

    // Solid Sylinder
    f32 wheelWidth = wheelRadius * 0.5f;
    car->body.wheels[i].invBodyInertiaTensor.m00 =
      (1.f / 12.f) * mass *
      (3.f * (wheelRadius * wheelRadius) + (wheelWidth * wheelWidth));
    car->body.wheels[i].invBodyInertiaTensor.m11 =
      (1.f / 2.f) * mass * (wheelRadius * wheelRadius);
    car->body.wheels[i].invBodyInertiaTensor.m22 =
      (1.f / 12.f) * mass *
      (3.f * (wheelRadius * wheelRadius) + (wheelWidth * wheelWidth));

    m3x3 wbI = car->body.wheels[i].invBodyInertiaTensor;
    car->body.wheels[i].invWorlInertiaTensor =
      car->body.wheels[i].orientation *
      wbI *
      m3x3_transpose(car->body.wheels[i].orientation);

    car->body.wheels[i].friction = i < 2 ?
      carProperties.rearWheelBaseFriction :
      carProperties.frontWheelBaseFriction;
    car->body.wheels[i].shapeNum = 1;

    setupHingeJoint(&car->joints[i].wheelHinge, &car->body.chassis,
                    &car->body.wheels[i], pivot, {0.f,0.f,0.f},
                    {0.0f,1.0f,0.0f}, 30.f, 1.0f);
    setupSliderJoint(&car->joints[i].wheelSuspension, &car->body.chassis,
                     &car->body.wheels[i], {pivot.x,pivot.y,pivot.z}, {0.f,0.f,0.f},
                     {0.f,0.f,1.f}, 30.f, 1.0f);
    setupAxisJoint(&car->joints[i].wheelSuspensionLimits, &car->body.chassis,
                   &car->body.wheels[i], pivot, {0.f,0.f,0.f},
                   {0.0f,0.0f,1.0f}, 
                   carProperties.suspensionHz, carProperties.suspensionDamping);
  }
}

static void prepareRenderCommands(model_data* model,
                                  u32 meshIdx,
                                  vs_uniform_params* vsParams,
                                  rt_shader_program_handle programHandle,
                                  rt_command_buffer* rendererBuffer) {
  {
    rt_command_apply_uniforms* cmd =
      rt_pushRenderCommand(rendererBuffer, apply_uniforms);

    cmd->shaderProgram = programHandle;

    cmd->uniforms[0] = (rt_uniform_data){rt_uniform_type_mat4,
      STR("model_matrix"),
      &vsParams[meshIdx].modelMat};
    cmd->uniforms[1] = (rt_uniform_data){rt_uniform_type_mat4,
      STR("view_matrix"),
      &vsParams[meshIdx].viewMat};
    cmd->uniforms[2] = (rt_uniform_data){rt_uniform_type_mat4,
      STR("proj_matrix"),
      &vsParams[meshIdx].projMat};

    cmd->uniforms[3] = (rt_uniform_data){
      rt_uniform_type_vec4,
      STR("color"),
      &model->matData[meshIdx].baseColorValue};
  }
  {
    rt_command_draw_elements* cmd =
      rt_pushRenderCommand(rendererBuffer, draw_elements);
    cmd->numElement = model->meshData[meshIdx].elementNum;
    cmd->baseElement = model->meshData[meshIdx].baseElement;
    cmd->baseVertex = model->meshData[meshIdx].baseVertex;
  }
}

static void renderCar(car_game_state* game, memory_arena* tempArena,
                      rt_command_buffer* rendererBuffer,
                      ui_widget_context* widgetContext,
                      m4x4 view, m4x4 proj) {
  car_state* car = &game->car;
    // Max eight levels deep nested children, should be enough.
  if (isBitSet(game->debug.visibilityState, visibility_state_car)) {
    v4 origin = v4_from_v3(car->body.chassis.origin - game->camera.position, 1.f);
    m4x4 orientation = m4x4_from_m3x3(car->body.chassis.orientation);
    m4x4 modelMat = m4x4_translate_make(origin) * orientation;

    m4x4 matStack[8];
    u32 childStack[8];

    u32 matStackIdx = 0;
    u32 childNum = 0;

    matStack[matStackIdx] = modelMat;
    childStack[matStackIdx] = 0;

    {
      rt_command_apply_program* cmd = rt_pushRenderCommand(
        rendererBuffer, apply_program);
      cmd->programHandle = car->programHandle;
      cmd->ccwFrontFace = true;
      cmd->enableBlending = true;
      cmd->enableCull = true;
      cmd->enableDepthTest = true;
    }

    {
      rt_command_apply_bindings* cmd =
        rt_pushRenderCommand(rendererBuffer, apply_bindings);
      cmd->indexBufferHandle =
        car->chassisModel.indexBufferHandle;
      cmd->vertexBufferHandle =
        car->chassisModel.vertexBufferHandle;
      cmd->vertexArrayHandle =
        car->chassisModel.vertexArrayHandle;
    }

    vs_uniform_params* vsParams = pushArray(tempArena, car->chassisModel.meshNum, vs_uniform_params);
    for (u32 meshIdx = 0; meshIdx < car->chassisModel.meshNum; meshIdx++) {
      m4x4 m = car->chassisModel.transform[meshIdx];
      m = matStack[matStackIdx] * m;

      if (car->chassisModel.meshData[meshIdx].childNum > 0) {
        matStackIdx++;
        matStack[matStackIdx] = m;
        childStack[matStackIdx] = childNum;
        childNum = car->chassisModel.meshData[meshIdx].childNum;
      } else {
        if (childNum > 0) {
          childNum--;
          if (childNum == 0) {
            if (matStackIdx > 0) {
              childNum = childStack[matStackIdx];
              matStackIdx--;
            }
          }
        }
      }
      if (car->chassisModel.meshData[meshIdx].elementNum == 0) {
        continue;
      }

      vsParams[meshIdx].modelMat = m;
      vsParams[meshIdx].viewMat = view;
      vsParams[meshIdx].projMat = proj;

      prepareRenderCommands(&car->chassisModel, meshIdx, vsParams, car->programHandle, rendererBuffer);
    }

    for (u32 wheelIdx = 0; wheelIdx < WHEEL_NUM; wheelIdx++) {
      model_data* model = car->wheelModel + wheelIdx;

      {
        rt_command_apply_bindings* cmd = rt_pushRenderCommand(
          rendererBuffer, apply_bindings);
        cmd->indexBufferHandle = model->indexBufferHandle;
        cmd->vertexBufferHandle = model->vertexBufferHandle;
        cmd->vertexArrayHandle = model->vertexArrayHandle;
      }

      matStackIdx = 0;
      childNum = 0;

      v4 originVec = v4_from_v3(car->body.wheels[wheelIdx].origin - game->camera.position, 1.f);

      m4x4 origin = m4x4_translate_make(originVec);
      m4x4 orientation = m4x4_from_m3x3(car->body.wheels[wheelIdx].orientation);

      modelMat = origin * orientation;

      matStack[matStackIdx] = modelMat;
      childStack[matStackIdx] = 0;

      vsParams = pushArray(tempArena, model->meshNum, vs_uniform_params);
      // Do not use first mesh transform matrix, it's only used to define
      // the physical wheels inital position
      for (size_t meshIdx = 0; meshIdx < model->meshNum; meshIdx++) {
        m4x4 m = meshIdx ? model->transform[meshIdx]
          : (m4x4)M4X4_IDENTITY;
        m = matStack[matStackIdx] * m;

        if (model->meshData[meshIdx].childNum > 0) {
          matStackIdx++;
          matStack[matStackIdx] = m;
          childStack[matStackIdx] = childNum;
          childNum = model->meshData[meshIdx].childNum;
        } else {
          if (childNum > 0) {
            childNum--;
            if (childNum == 0) {
              if (matStackIdx > 0) {
                childNum = childStack[matStackIdx];
                matStackIdx--;
              }
            }
          }
        }

        if (model->meshData[meshIdx].elementNum == 0) {
          continue;
        }

        vsParams[meshIdx].modelMat = m;
        vsParams[meshIdx].viewMat = view;
        vsParams[meshIdx].projMat = proj;

        prepareRenderCommands(model, meshIdx, vsParams,
                              car->programHandle, rendererBuffer);
      }
    }
  }

  if (isBitSet(game->debug.visibilityState, visibility_state_car_colliders))
  {
    v3 vertices[8];
    for (u32 i = 0; i < 8; i++) {
      vertices[i] = car->bodyShapes[i].point.p;
    }
    shape_box box = createBoxShape((f32*)vertices);
    {
      v4 originVec = v4_from_v3(car->body.chassis.origin - game->camera.position, 1.f);
      m4x4 origin = m4x4_translate_make(originVec);
      m4x4 orientation = m4x4_from_m3x3(car->body.chassis.orientation);

      m4x4 modelMat = origin * orientation;

      rt_command_render_simple_box* cmd =
        rt_pushRenderCommand(rendererBuffer, render_simple_box);

      cmd->min = box.min;
      cmd->max = box.max;
      v4 c = (v4){0.0f, 0.0f, 0.4f, 0.4f};
      cmd->color = c;
      cmd->projView = proj * view;
      cmd->model = modelMat;
    }
    // Note: in greater speeds the contact points may visually lag behind.
    //       This is because the velocity integration happens after contact test
    for (u32 i = 0; i < car->contactPointNum; i++) {
      contact_point* contactPoint = car->contactPointStorage + i;
      if (contactPoint->shapeIdx == -1) continue;
      v3 point = (v3){contactPoint->point.x,contactPoint->point.y,contactPoint->point.z};
      shape_box box =
        createBoxShape((v3){-0.05f, -0.05f, -0.05f},
                       (v3){0.05f, 0.05f, 0.1f});
      rt_command_render_simple_box* cmd =
        rt_pushRenderCommand(rendererBuffer, render_simple_box);

      cmd->min = box.min;
      cmd->max = box.max;
      v4 c = (v4){1.0f, 1.0f, 0.0f, 0.8f};
      cmd->color = c;
      cmd->projView = proj * view;
      cmd->model = m4x4_translate_make(v4_from_v3(point, 1.f));
     }
  }
  if (isBitSet(game->debug.visibilityState, visibility_state_slip_angle))
  {
    for (u32 i = 0; i < WHEEL_NUM; i++){
      f32 slipAngle = car->stats.slipAngle[i];

      rt_command_render_simple_arrow* cmd =
        rt_pushRenderCommand(rendererBuffer, render_simple_arrow);
      m4x4 m = m4x4_translate_make(
        v4_from_v3(car->body.wheels[i].position - game->camera.position, 1.0f)) *
        m4x4_rotate_make({0.0f,0.0f, -slipAngle}) *
        m4x4_from_m3x3(car->body.chassis.orientation) *
        m4x4_scale_make({1.0f + fabsf(slipAngle),1.0f,1.0f});

      v4 c = LERP(
        ((v4){0.2f, 0.8f, 0.4f, 1.0f}),
        ((v4){0.8f, 0.2f, 0.4f, 1.0f}), fabsf(slipAngle));

      cmd->size = 0.2;
      cmd->length = 0.5f;
      cmd->color = c;
      cmd->projView = proj * view;
      cmd->model = m;
    }
  }
}


static f32 torqueFromRpm(car_state* car, i32 rpm, f32 throttle) {
  f32 rpmN = (f32)abs(rpm) / 7000.f;
  f32 torque = (bezier6n(carProperties.motorTorqueCurve, rpmN) * 
    carProperties.motorTorque * carProperties.differentialRatio * 
    carProperties.gearRatios[car->stats.gear] * 
    carProperties.transmissionEfficiency) * throttle;

  return torque;
}

static i32 rpmFromAngVel(car_state* car, f32 angVel) {
  i32 rpm = angVel * carProperties.differentialRatio * 60 / 2 * PI
    * carProperties.gearRatios[car->stats.gear];
  return rpm;
}

static void carUpdate(car_game_state* game, 
                      memory_arena* tempArena,
                      rt_command_buffer* rendererBuffer,
                      f32 delta) { 
  car_state* car = &game->car;
  input_state* input = &game->input;
  v3 extraForces[RIGID_BODY_NUM];
  v3 extraTorques[RIGID_BODY_NUM];
  for (u32 i = 0; i < RIGID_BODY_NUM; i++) {
    extraForces[i] = V3_ZERO;
    extraTorques[i] = V3_ZERO;
  }

  f32 angVelMSecMap[] = {0.f, 5.0f, 15.f, 30.f, 50.f, 100.f};

  i32 angVelMapLast = arrayLen(angVelMSecMap) - 1;

  v2 axis = {-(f32)(input->moveLeft >= button_state_pressed) +
    (f32)(input->moveRight >= button_state_pressed),
    -(f32)(input->moveDown >= button_state_pressed) +
    (f32)(input->moveUp >= button_state_pressed)};

  f32 longSpeed = v3_dot(car->body.chassis.forwardAxis, car->body.chassis.velocity);
  f32 longSpeedN = fabs(longSpeed / 50.f);

  v3 hingeAxis = car->joints[0].wheelHinge.hinge.hingeAxis;
  v4 localAxisARot = car->joints[0].wheelHinge.hinge.localAxisARotation;
  f32 rotationRate = 0.f;
  for (i32 i = 0; i < 2; i++) {
    v3 wheelWAxis =
      v3_rotate_axis_angle(hingeAxis * car->body.wheels[i].orientation,
                           localAxisARot.w, localAxisARot.xyz);
  
    rotationRate += v3_dot(wheelWAxis, car->body.wheels[i].angularVelocity);
  }
 
  f32 flywheelRadius = 0.9f;
  f32 angVelMSec = (rotationRate / 2.f) * 
    carProperties.wheelShape.radius * flywheelRadius;
  b32 shiftingGears = car->stats.gearShiftT > 0.0f;

  car->stats.gearShiftT -= delta;
  car->stats.gearShiftT = MAX(0.0, car->stats.gearShiftT);
  b32 breaking = axis.y < -0.1;
  b32 reversing = breaking && longSpeed < 0.1;
  b32 accelerating = axis.y > 0.1f;
  f32 throttle = axis.y; 

  i32 rpm = rpmFromAngVel(car, angVelMSec);

  rpm = reversing ? 
      CLAMP(rpm, -7000, 0) :
      CLAMP(rpm, 1600, 7000);
  if (car->stats.gearShiftT > 0.f) {
    rpm -= 2000 * delta;
  }
  car->joints[0].wheelHinge.hinge.motorTorque = -0.f;
  car->joints[1].wheelHinge.hinge.motorTorque = -0.f;
  car->stats.accelerating = accelerating;
  f32 trueRpm = LERP(car->stats.rpm, rpm, carProperties.engineInertia);
  f32 torque = torqueFromRpm(car, rpm, throttle);

  
  if (!shiftingGears) {
    if (trueRpm >= 5000 && car->stats.gear < carProperties.gearNum - 1) {
      car->stats.gear++;
      car->stats.gearShiftT = 0.25f;
    } 
    else if (trueRpm < 2800 && car->stats.gear > 0) {
      car->stats.gear--;
      car->stats.gearShiftT = 0.25f;
    }
  }    
  // Motor break force
  if (accelerating) {
    i32 angVelIdx = 0;
    angVelMSec = MAX(angVelMSec, axis.y * 5.f);
    while (angVelMSec >= angVelMSecMap[angVelIdx] && angVelIdx < angVelMapLast) {
      angVelIdx++;
    };
  }
  else {
    if (!reversing) {
      torque = breaking ? -2000: -MAX((rpm - 1600) / 6 ,0);
    }
  }

  car->joints[0].wheelHinge.hinge.motorTorque = torque;
  car->joints[1].wheelHinge.hinge.motorTorque = torque;
  car->stats.motorTorque = torque;
  car->stats.rpm = trueRpm;
  f32 turnRate =
    0.15f *
    (fabsf(axis.y) > 0.1f ? LERP(1.0, 1.0, 1.0f - longSpeedN) : 1.f);
  car->turnAngle = LERP(car->turnAngle, 0.35f * axis.x, turnRate);

  car->joints[2].wheelHinge.hinge.localAxisARotation = {0.0f, 0, 1.0f, -car->turnAngle};
  car->joints[3].wheelHinge.hinge.localAxisARotation = {0.0f, 0, 1.0f, -car->turnAngle};

  car->joints[2].wheelHinge.hinge.localAxisBRotation = {0.0f, 0, 1.0f, car->turnAngle};
  car->joints[3].wheelHinge.hinge.localAxisBRotation = {0.0f, 0, 1.0f, car->turnAngle};

  f32 stepDelta = delta;

  f32 currentTime = 0;
  f32 targetTime = stepDelta;

   if (input->reset == button_state_pressed) {
    carSetInitialState(game);
    car->body.chassis.velocity = (v3){0.0f,0.0f,0.0f};
    car->body.chassis.angularVelocity = (v3){0.0f,0.0f,0.0f};
    game->input.pausePhysics = false;
  }

  static rigid_body terrainBody = {0};
  terrainBody.id = 100;
  terrainBody.friction = 0.6f;
  terrainBody.orientationQuat = QUAT_IDENTITY;

  contact_manifold manifold[MAX_CONTACTS];
  // TODO: CCD
  while (currentTime < stepDelta) {
    // Calculate forces
    f32 itDelta = targetTime - currentTime;

    // Body contacts
    i32 shapeIdx = 0;
    u32 contactIdx = 0;
    for (u32 i = 0; i < RIGID_BODY_NUM; i++) {
      rigid_body* body = car->body.bodies + i;
      for (i32 d = 0; d < body->shapeNum; d++, shapeIdx++) {
        shape* bShape = car->bodyShapes + shapeIdx;
        f32 depth = 0.f;
        v3 normal;
        v3 u = {0};
        if (bShape->type == shape_type::SHAPE_POINT) {
          u = (body->orientation * (bShape->point.p - body->localCenter));
          v3 worldU = u + (v3){body->position.x,body->position.y,body->position.z};
          depth = getGeometryHeight(worldU, game, &normal);
        } else if (bShape->type == shape_type::SHAPE_SPHERE) {
          v3 worldU = {
            body->position.x + body->localCenter.x,
            body->position.y + body->localCenter.y,
            body->position.z + body->localCenter.z};
          depth = getGeometryHeight(worldU, game,
                                    &normal);
          depth = depth - bShape->sphere.radius;
          u = normal * (depth - bShape->sphere.radius);
        }
        i32 oldContact =
          findOldContact(car->contactPointStorage, contactIdx, shapeIdx);

        if (depth < 0.f) {
          v3 uu = (v3){u.x,u.y,u.z};
          contact_manifold* man = manifold + contactIdx++;
          man->point.separation = depth;
          man->point.localPointA = body->position + uu;
          man->point.localPointB = uu;
          man->point.point = body->position + uu;
          man->point.shapeIdx = shapeIdx;
          man->point.normal = normal;
          man->point.normalImpulse = 0.f;
          man->point.tangentImpulse[0] = 0.f;
          man->point.tangentImpulse[1] = 0.f;
          man->bodyB = body;
          man->bodyA = &terrainBody;
          if (oldContact != -1) {
            man->point = car->contactPointStorage[oldContact];
          }
        }
        if (oldContact != -1) {
          car->contactPointStorage[oldContact].shapeIdx = -1;
        }
      }
    }
    car->contactPointNum = contactIdx;

    f32 h = itDelta / subStepAmount;
    f32 invH = 1.0f / h;

    // Prepare contacts
    u32 constraintNum = contactIdx;
    u32 jointNum = arrayLen(car->jointsAll);
    contact_constraint constraints[contactIdx];
    u16 constraintIdx = 0;
    for (u16 idx = 0; idx < constraintNum; idx++) {
      contact_manifold* man = manifold + idx;
      if (man->point.shapeIdx == -1) {
        continue;
      }
      contact_constraint* constraint = constraints + constraintIdx++;
      preSolve(man, constraint, constraintNum, h);
      if (constraint->bodyB->id > 0) {
        rigid_body *wheelBody = man->bodyB;
        u32 id = wheelBody->id - 1;
        hinge_joint* wheelJoint = &car->joints[id].wheelHinge.hinge;

        v3 hA = wheelJoint->hingeAxis;
        v4 locABRot = wheelJoint->localAxisBRotation;
        v4 locAARot = wheelJoint->localAxisARotation;

        v3 sideAxis = v3_rotate_axis_angle(hA * car->body.chassis.orientation, locABRot.w, locABRot.xyz);
        v3 upAxis = man->point.normal;
        v3 forwardAxis = v3_cross(sideAxis, upAxis);

        v3 wheelWAxis = v3_rotate_axis_angle(hA * wheelBody->orientation, locAARot.w, locAARot.xyz);
        f32 wheelAngSpeed = v3_dot(wheelWAxis, wheelBody->angularVelocity) * car->properties.wheelShape.radius;
        b32 isWheelStopped = fabs(wheelAngSpeed) < 0.001f;
        f32 slipRatio = 0.f;
        
        if (fabsf(longSpeed) > 0.1f) {
          f32 slideSgn =
            isWheelStopped ? SIGNF(longSpeed) : SIGNF(wheelAngSpeed);
          f32 slip = MAX((wheelAngSpeed - longSpeed) * slideSgn, 0.f);
          slipRatio = slip / fabs(longSpeed);
        }
        f32 slipT = CLAMP(slipRatio / 2.f, 0.0f, 1.0f);
        f32 slipRatioForce = bezier6n(
          carProperties.slipRatioForceCurve,
          slipT);

        f32 wLatSpeed = v3_dot(wheelBody->velocity, sideAxis);
        f32 wLongSpeed = v3_dot(wheelBody->velocity, forwardAxis);

        f32 slipAngle = wLongSpeed > 0.05f ? atan2f(wLatSpeed, wLongSpeed) : 0.f;

        f32 t = longSpeed > 10.f ? CLAMP(slipAngle / 2.f,-1.0,1.f) : 0.f;
        f32 slipAngleForce = bezier6n(
          carProperties.slipAngleForceCurve,
          fabsf(t));
        
        f32 slipAngleFriction = ((id > 1) ? 
          carProperties.slipAngleForceCoeffFW : 
          carProperties.slipAngleForceCoeffRW
        ) * slipAngleForce;

        f32 slipRatioFriction = slipRatioForce * carProperties.slipRatioForceCoeff;
        f32 frictionAdjustment= MAX(slipRatioFriction + slipAngleFriction, 0.0f);
        constraint->friction = constraint->friction + frictionAdjustment;

        car->stats.frictionAdjustment[id] = frictionAdjustment;
        car->stats.slipRatio[id] = slipRatio;
        car->stats.slipAngle[id] = slipAngle; 
      }

    }
       // printf("force %f %f %f torque %f %f %f\n", 
       //  extraForces[0].x, extraForces[0].y, extraForces[0].z,
       //  extraTorques[0].x, extraTorques[0].y, extraTorques[0].z);

    for (u32 i = 0; i < jointNum; ++i) {
      jointPreSolve(car->jointsAll + i, h);
    }

    // Solve
    // body 2 * substepCount
    // constraint 2 * substepCount (merge warm starting)
    for (i32 substep = 0; substep < subStepAmount; ++substep) {
      // Integrate velocities
      for (u32 i = 0; i < RIGID_BODY_NUM; i++) {
        integrateVelocities(h, &car->body.bodies[i], extraForces[i], extraTorques[i]);
      }

      // Warm start
      for (u32 i = 0; i < jointNum; ++i) {
        jointWarmStart(car->jointsAll + i);
      }

      // Warm start
      for (u32 i = 0; i < constraintNum; ++i) {
        contact_constraint* constraint = constraints + i;
        warmStart(constraint, constraintNum);
      }
      for (u32 i = 0; i < jointNum; ++i) {
        jointSolve(car->jointsAll + i, h, invH, true);
      }

      // Solve velocities using position bias
      for (u32 i = 0; i < constraintNum; ++i) {
        contact_constraint* constraint = constraints + i;
        solve(constraint, constraintNum, invH, true);
      }
      // Integrate positions using biased velocities
      for (u32 i = 0; i < RIGID_BODY_NUM; i++) {
        integratePositions(h, &car->body.bodies[i]);
      }

      // Relax biased velocities and impulses.
      // Relaxing the impulses reduces warm starting overshoot.

      for (u32 i = 0; i < jointNum; ++i) {
        jointSolve(car->jointsAll + i, h, invH, false);
      }

      // Solve velocities using position bias
      for (u32 i = 0; i < constraintNum; ++i) {
        contact_constraint* constraint = constraints + i;
        solve(constraint, constraintNum, invH, false);
      }
    }

    // finalizePositions(&car->body.chassis);
    for (u32 i = 0; i < RIGID_BODY_NUM; i++) {
      finalizePositions(&car->body.bodies[i], h);
    }

    for (u32 i = 0; i < constraintNum; ++i) {
      contact_constraint* constraint = constraints + i;
      storeContactImpulse(car->contactPointStorage + i, constraint, constraintNum);
    }
    car->body.chassis.origin =
      car->body.chassis.position -
      car->body.chassis.orientation * car->body.chassis.localCenter;

    car->body.chassis.forwardAxis = car->body.chassis.orientation * (v3){1.f,0.0f,0.0f};
    car->body.chassis.sideAxis = car->body.chassis.orientation * (v3){0.0f,1.0f,0.0f};

    for (u32 i = 0; i < WHEEL_NUM; i++) {
      car->body.wheels[i].origin =
        car->body.wheels[i].position -
        car->body.wheels[i].localCenter * car->body.wheels[i].orientation;
    }

    currentTime = targetTime;
    targetTime = itDelta;
  }
}

static void createCar(car_game_state* game,
                    memory_arena* permanentArena, memory_arena* tempArena,
                    rt_command_buffer* rendererBuffer,
                    b32 reload, f32 assetModTime) {

  if (!game->car.initialized || reload) {
    carCreateModel(permanentArena, tempArena, rendererBuffer, game,
                   assetModTime);
    if (!game->car.initialized) carSetInitialState(game);
    carSetupBody(game);
    game->car.initialized = true;
  }
}

static void updateCar(car_game_state* game,
                               memory_arena* tempArena,
                               rt_command_buffer* rendererBuffer,
                               ui_widget_context* widgetContext,
                               m4x4 view, m4x4 proj, f32 delta) {
  // Car simulation
  profilerBegin(game->profiler, simulation);
  carUpdate(game, tempArena, rendererBuffer, delta);
  profilerEnd(game->profiler, simulation);
}
