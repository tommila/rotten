#include "all.h"

// TODO: Pull out collision testing/solving code

#define WHEEL_NUM 4
#define RIGID_BODY_NUM 1 + WHEEL_NUM

typedef struct vs_uniform_params {
  mat4s modelMat;
  mat4s viewMat;
  mat4s projMat;
} vs_uniform_params;

typedef struct read_gltf_node_result {
  i32 meshNum;
  usize vertexTotalSize;
  usize indexTotalSize;
} read_gltf_node_result;

static read_gltf_node_result readGltfNodeData(memory_arena* tempArena,
                                              mesh_data* meshData,
                                              mesh_material_data* matData,
                                              mat4* transform,
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
                            rt_render_entry_buffer* rendererBuffer,
                            model_data* model,
                            cgltf_node* node) {

  mesh_data *meshData = pushArray(tempArena, 32, mesh_data);
  mesh_material_data *matData = pushArray(tempArena, 32, mesh_material_data);
  mat4 transform[32];
  i32 childNum[32];

  read_gltf_node_result gltfReadResult =
    readGltfNodeData(tempArena, meshData, matData, transform,childNum, node);

  model->meshNum = gltfReadResult.meshNum;

  if (model->vertexArrayHandle == 0) {
    rt_render_entry_create_vertex_buffer* cmd = rt_renderer_pushEntry(
        rendererBuffer, rt_render_entry_create_vertex_buffer);

    cmd->vertexData = NULL;
    cmd->indexData = NULL;
    cmd->vertexDataSize = gltfReadResult.vertexTotalSize;
    cmd->indexDataSize = gltfReadResult.indexTotalSize;
    cmd->isStreamData = false;

    cmd->vertexBufHandle = &model->vertexBufferHandle;
    cmd->indexBufHandle = &model->indexBufferHandle;
    cmd->vertexArrHandle = &model->vertexArrayHandle;

    cmd->vertexAttributes[0] = {.count = 3,
                                .offset = 0,
                                .stride = 6 * sizeof(f32),
                                .type = rt_renderer_data_type_f32},
    cmd->vertexAttributes[1] = {.count = 3,
                                .offset = 3 * sizeof(f32),
                                .stride = 6 * sizeof(f32),
                                .type = rt_renderer_data_type_f32};

    // Flush command buffer as we need for the buffer ids to be ready.
    // They are needed for the data update that happens next
    platformApi->renderer_flushCommandBuffer(rendererBuffer);
  }

  i32 baseElement = 0;
  i32 baseVertex = 0;

  for (i32 meshIdx = 0; meshIdx < gltfReadResult.meshNum; meshIdx++) {
    mesh_data* subMeshData = meshData + meshIdx;
    mesh_material_data* subMatData = matData + meshIdx;
    model->transform[meshIdx] = glms_mat4_make((f32*)(transform + meshIdx));

    index_data* subModelMeshData = model->meshData + meshIdx;
    model_material_data* subModelMaterialData = model->matData + meshIdx;

    rt_render_entry_update_vertex_buffer* cmd = rt_renderer_pushEntry(
        rendererBuffer, rt_render_entry_update_vertex_buffer);
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

    subModelMaterialData->baseColorValue = glms_vec4_make(subMatData->baseColor);

    baseElement += subMeshData->indexNum;
    baseVertex += subMeshData->vertexNum;

    platformApi->renderer_flushCommandBuffer(rendererBuffer);
  }
}

static void carCreateModel(memory_arena* permanentArena, memory_arena* tempArena,
			   rt_render_entry_buffer* rendererBuffer,
			   game_state* game, utime assetModTime) {
  // Model mesh
  game_object* car = &game->car;
  utime modelModTime =
      platformApi->diskIOReadModTime("./assets/models/low_poly_car/scene.gltf");
  if (assetModTime < modelModTime) {
    cgltf_data* gltfData =
        gltf_loadFile(tempArena, "./assets/models/low_poly_car/scene.gltf");
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
      platformApi->diskIOReadModTime("./assets/shaders/car.vs");
  utime fsShaderModTime =
      platformApi->diskIOReadModTime("./assets/shaders/car.fs");
  if (assetModTime < vsShaderModTime || assetModTime < fsShaderModTime) {
    shader_data carShader = importShader(tempArena, "./assets/shaders/car.vs",
                                         "./assets/shaders/car.fs");
    if (car->programHandle == 0) {
      rt_render_entry_create_shader_program* cmd = rt_renderer_pushEntry(
          rendererBuffer, rt_render_entry_create_shader_program);

      cmd->fragmentShaderData = (char*)carShader.fsData;
      cmd->vertexShaderData = (char*)carShader.vsData;
      cmd->shaderProgramHandle = &car->programHandle;
    } else {
      rt_render_entry_update_shader_program* cmd = rt_renderer_pushEntry(
          rendererBuffer, rt_render_entry_update_shader_program);

      cmd->fragmentShaderData = (char*)carShader.fsData;
      cmd->vertexShaderData = (char*)carShader.vsData;
      cmd->shaderProgramHandle = &car->programHandle;
    }
  }
}


vec3s kCarLocalCenter = (vec3s){0.5f, 0.0f, -0.5f};

static void carSetInitialState(game_state* game) {
    game_object* car = &game->car;
    vec3s _notUsed;
    car->body.chassis.id = 0;
    car->body.chassis.forwardAxis = GLMS_XUP;
    car->body.chassis.orientation = GLMS_MAT3_IDENTITY;
    car->body.chassis.orientationQuat = GLMS_QUAT_IDENTITY_INIT;
    car->body.chassis.angularVelocity = {0.f, 0.f, 0.f};
    car->body.chassis.localCenter = kCarLocalCenter;

    car->body.chassis.position.z =
        -getGeometryHeight((vec3s){0.0f, 0.0f, -0.0f}, game, &_notUsed);
    car->body.chassis.origin =
        car->body.chassis.position -
        car->body.chassis.orientation * car->body.chassis.localCenter;

    for (i32 i = 0; i < WHEEL_NUM; i++) {
      car->body.wheels[i].orientation = GLMS_MAT3_IDENTITY_INIT;
      car->body.wheels[i].orientationQuat = GLMS_QUAT_IDENTITY_INIT;
      car->body.wheels[i].id = 1 + i;

      vec3s offset = (vec3s){0.f, 0.f, -0.15f} - car->body.chassis.localCenter;
      vec3s pivot = car->wheelModel[i].transform[0] * offset;
      car->body.wheels[i].position =
        pivot + car->body.chassis.position;

      car->body.wheels[i].origin =
          car->body.wheels[i].position -
          car->body.wheels[i].localCenter * car->body.wheels[i].orientation;
    }
}

static void carSetupBody(game_state* game) {
  car_object* car = &game->car;
  shape* chassisShape = &car->bodyShapes[0];
  chassisShape->type = shape_type::SHAPE_BOX;
  shape_box box =
      createBoxShape((vec3s){-2.15f, -0.8f, 0.4}, (vec3s){2.15f, 0.8f, 1.2f});
  vec3s boxVertices[8];
  u32 shapeNum = arrayLen(boxVertices);
  decomposeBoxShape(box, (f32*)boxVertices);
  for (u32 i = 0; i < shapeNum; i++) {
    car->bodyShapes[i].type = shape_type::SHAPE_POINT;
    car->bodyShapes[i].point.p = boxVertices[i];
  }
  car->body.chassis.shapeNum = shapeNum;
  vec3s size = box.size;
  f32 mass = 1400;  // kg

  car->body.chassis.localCenter = kCarLocalCenter;
  car->body.chassis.mass = mass;
  car->body.chassis.invMass = 1.0f / mass;

  // Solid cuboid
  car->body.chassis.invBodyInertiaTensor.m00 =
      (1.f / 12.f) * mass * (size.y * size.y + size.z * size.z);
  car->body.chassis.invBodyInertiaTensor.m11 =
      (1.f / 12.f) * mass * (size.x * size.x + size.z * size.z);
  car->body.chassis.invBodyInertiaTensor.m22 =
      (1.f / 12.f) * mass * (size.x * size.x + size.y * size.y);
  car->body.chassis.invBodyInertiaTensor =
      glms_mat3_inv(car->body.chassis.invBodyInertiaTensor);

  car->body.chassis.friction = 0.3f;
  car->body.chassis.invWorlInertiaTensor =
      car->body.chassis.orientation * car->body.chassis.invBodyInertiaTensor *
      glms_mat3_transpose(car->body.chassis.orientation);

  // Car wheels body setup
  for (u32 i = 0; i < WHEEL_NUM; i++) {
    f32 wheelRadius = 0.3f;
    shape* sphere = &car->bodyShapes[car->body.chassis.shapeNum + i];
    sphere->type = shape_type::SHAPE_SPHERE;
    sphere->sphere.radius = wheelRadius;
    f32 mass = 20.f;  // kg
    vec3s offset = (vec3s){0.f, 0.f, -0.15f} - car->body.chassis.localCenter;
    vec3s pivot = car->wheelModel[i].transform[0] * offset;
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

    car->body.wheels[i].invWorlInertiaTensor =
        car->body.wheels[i].orientation *
        car->body.wheels[i].invBodyInertiaTensor *
        glms_mat3_transpose(car->body.wheels[i].orientation);

    car->body.wheels[i].friction = 0.9f;
    car->body.wheels[i].shapeNum = 1;

    setupHingeJoint(&car->wheelHinge[i], &car->body.chassis,
                    &car->body.wheels[i], pivot, GLMS_VEC3_ZERO_INIT,
                    GLMS_YUP, 30.f, 1.0f);
    setupSliderJoint(&car->wheelSuspension[i], &car->body.chassis,
		     &car->body.wheels[i], pivot, GLMS_VEC3_ZERO_INIT,
		     GLMS_ZUP, 30.f, 1.0f);
    setupAxisJoint(&car->wheelSuspensionLimits[i], &car->body.chassis,
		   &car->body.wheels[i], pivot, GLMS_VEC3_ZERO_INIT,
		   GLMS_ZUP, -0.25f, 0.0f, 10.0f, 1.0f);
  }
}

static void prepareRenderCommands(model_data* model,
				  u32 meshIdx,
				  vs_uniform_params* vsParams,
				  shader_program_handle programHandle,
                                  rt_render_entry_buffer* rendererBuffer) {
    {
      rt_render_entry_apply_uniforms* cmd =
          rt_renderer_pushEntry(rendererBuffer, rt_render_entry_apply_uniforms);

      cmd->shaderProgram = programHandle;

      cmd->uniforms[0] = (uniform_entry){.type = uniform_type_mat4,
                                         .name = "model_matrix",
                                         .data = &vsParams[meshIdx].modelMat};
      cmd->uniforms[1] = (uniform_entry){.type = uniform_type_mat4,
                                         .name = "view_matrix",
                                         .data = &vsParams[meshIdx].viewMat};
      cmd->uniforms[2] = (uniform_entry){.type = uniform_type_mat4,
                                         .name = "proj_matrix",
                                         .data = &vsParams[meshIdx].projMat};

      cmd->uniforms[3] = (uniform_entry){
          .type = uniform_type_vec4,
          .name = "color",
          .data = &model->matData[meshIdx].baseColorValue};
    }
    {
      rt_render_entry_draw_elements* cmd =
          rt_renderer_pushEntry(rendererBuffer, rt_render_entry_draw_elements);
      cmd->numElement = model->meshData[meshIdx].elementNum;
      cmd->baseElement = model->meshData[meshIdx].baseElement;
      cmd->baseVertex = model->meshData[meshIdx].baseVertex;
    }
}

static void carRender(game_state* game, memory_arena* tempArena,
                    rt_render_entry_buffer* rendererBuffer,
		    mat4s view, mat4s proj) {
  car_object* car = &game->car;
  mat4s modelMat =
        glms_translate_make(car->body.chassis.origin) *
        glms_mat3_make_mat4(car->body.chassis.orientation);

  // Max eight levels deep nested children, should be enough.
  if (isBitSet(game->deve.drawState, DRAW_CAR)) {
    mat4s matStack[8];
    u32 childStack[8];

    u32 matStackIdx = 0;
    u32 childNum = 0;

    matStack[matStackIdx] = modelMat;
    childStack[matStackIdx] = 0;

    {
      rt_render_entry_apply_program* cmd = rt_renderer_pushEntry(
          rendererBuffer, rt_render_entry_apply_program);
      cmd->programHandle = car->programHandle;
      cmd->ccwFrontFace = true;
      cmd->enableBlending = true;
      cmd->enableCull = true;
      cmd->enableDepthTest = true;
    }

    {
      rt_render_entry_apply_bindings* cmd =
          rt_renderer_pushEntry(rendererBuffer, rt_render_entry_apply_bindings);
      cmd->indexBufferHandle =
          car->chassisModel.indexBufferHandle;
      cmd->vertexBufferHandle =
          car->chassisModel.vertexBufferHandle;
      cmd->vertexArrayHandle =
          car->chassisModel.vertexArrayHandle;
    }

    vs_uniform_params* vsParams = pushArray(tempArena, car->chassisModel.meshNum, vs_uniform_params);
    for (u32 meshIdx = 0; meshIdx < car->chassisModel.meshNum; meshIdx++) {
      mat4s m = car->chassisModel.transform[meshIdx];
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
        rt_render_entry_apply_bindings* cmd = rt_renderer_pushEntry(
            rendererBuffer, rt_render_entry_apply_bindings);
        cmd->indexBufferHandle = model->indexBufferHandle;
        cmd->vertexBufferHandle = model->vertexBufferHandle;
        cmd->vertexArrayHandle = model->vertexArrayHandle;
      }

      matStackIdx = 0;
      childNum = 0;

      modelMat = glms_translate_make(car->body.wheels[wheelIdx].origin) *
                 glms_mat3_make_mat4(car->body.wheels[wheelIdx].orientation);

      matStack[matStackIdx] = modelMat;
      childStack[matStackIdx] = 0;


      vsParams = pushArray(tempArena, model->meshNum, vs_uniform_params);
      // Do not use first mesh transform matrix, it's only used to define
      // the physical wheels inital position
      for (size_t meshIdx = 0; meshIdx < model->meshNum; meshIdx++) {
        mat4s m = meshIdx ? model->transform[meshIdx] : (mat4s)GLMS_MAT4_IDENTITY_INIT;
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

  if (isBitSet(game->deve.drawState, DRAW_CAR_COLLIDERS))
  {
    vec3s vertices[8];
    for (u32 i = 0; i < 8; i++) {
      vertices[i] = car->bodyShapes[i].point.p;
    }
    shape_box box = createBoxShape((f32*)vertices);

    {
      mat4s modelMat = glms_translate_make(car->body.chassis.origin) *
	      glms_mat3_make_mat4(car->body.chassis.orientation);
      mat4s mvp = proj * view * modelMat;
      rt_render_simple_box* cmd =
          rt_renderer_pushEntry(rendererBuffer, rt_render_simple_box);

      copyType(box.min.raw, cmd->min, vec3s);
      copyType(box.max.raw, cmd->max, vec3s);
      vec4s c = (vec4s){0.0f, 0.0f, 0.4f, 0.4f};
      copyType(c.raw, cmd->color, vec4s);
      copyType(mvp.raw, cmd->mvp, mat4s);
    }
    // Note: in greater speeds the contact points may visually lag behind.
    //       This is because the velocity integration happens after contact test
    for (u32 i = 0; i < car->contactPointNum; i++) {
      contact_point* contactPoint = car->contactPointStorage + i;
      mat4s mvp = proj * view;
      if (contactPoint->shapeIdx == -1) continue;
      shape_box box =
          createBoxShape((vec3s){-0.05f, -0.05f, -0.05f} +
                         contactPoint->point,
                         (vec3s){0.05f, 0.05f, 0.1f} +
        		 contactPoint->point);
      rt_render_simple_box* cmd =
          rt_renderer_pushEntry(rendererBuffer, rt_render_simple_box);

      copyType(box.min.raw, cmd->min, vec3s);
      copyType(box.max.raw, cmd->max, vec3s);
      vec4s c = (vec4s){1.0f, 1.0f, 0.0f, 0.8f};
      copyType(c.raw, cmd->color, vec4s);
      copyType(mvp.raw, cmd->mvp, mat4s);
    }
  }
}

static f32 calcSlipRatio(f32 angVel, f32 radius, f32 longtitudalVel) {
  f32 result = 0.f;
  result = (angVel * radius - longtitudalVel) / fabsf(longtitudalVel);
  return result;
}

static f32 pacejka(f32 x, f32 B, f32 C, f32 D, f32 E) {
  return D * sinf(C * atanf(B * x - E * (B * x - atanf(B * x))));
}

static void carUpdate(game_state* game, f32 delta) {
  car_object* car = &game->car;
  input_state* input = &game->input;
  vec3s extraForces[RIGID_BODY_NUM];
  vec3s extraTorques[RIGID_BODY_NUM];
  for (u32 i = 0; i < RIGID_BODY_NUM; i++) {
    extraForces[i] = GLMS_VEC3_ZERO;
    extraTorques[i] = GLMS_VEC3_ZERO;
  }

  f32 torqueMap[] = {0.f, 2000.f, 1000.f, 800.f,  550.f, 550.f};
  f32 angVelMSecMap[] = {0.f, 5.0f, 15.f, 30.f, 50.f, 100.f};

  i32 angVelMapLast = arrayLen(angVelMSecMap) - 1;

  vec2s axis = {-(f32)(input->moveLeft >= BUTTON_PRESSED) +
                    (f32)(input->moveRight >= BUTTON_PRESSED),
                -(f32)(input->moveDown >= BUTTON_PRESSED) +
                    (f32)(input->moveUp >= BUTTON_PRESSED)};

  f32 longSpeed = glms_vec3_dot(car->body.chassis.forwardAxis, car->body.chassis.velocity);
  f32 longSpeedN = CLAMP(longSpeed / 100.f, 0.0f, 1.0f);
  vec3s wheelWAxis =
    glms_vec3_rotate(car->wheelHinge[0].hinge.hingeAxis * car->body.wheels[0].orientation,
		     car->wheelHinge[0].hinge.localAxisARotation);
  f32 angVelMSec = glms_vec3_dot(wheelWAxis, car->body.wheels[0].angularVelocity) * 0.3f;
  b32 accelerating = fabsf(axis.y) > 0.1f;

  car->wheelHinge[0].hinge.motorTorque = -0.f;
  car->wheelHinge[1].hinge.motorTorque = -0.f;

  if (accelerating) {
    if (axis.y > 0) {
      i32 angVelIdx = 0;

      angVelMSec = MAX(angVelMSec, axis.y * 5.f);
      while (angVelMSec >= angVelMSecMap[angVelIdx] && angVelIdx < angVelMapLast) {
        angVelIdx++;
      };

      i32 currIdx = angVelIdx - 1;
      f32 currAngVel = angVelMSecMap[currIdx];
      f32 nextAngVel = angVelMSecMap[MIN(angVelMapLast, angVelIdx)];
      f32 subAngVelT = (currIdx ? MAX(currAngVel - angVelMSec, 0.f) : angVelMSec) /
                       (nextAngVel - currAngVel);
      f32 torque = LERP(torqueMap[currIdx],
                        torqueMap[MIN(angVelMapLast, angVelIdx)], subAngVelT);
      car->wheelHinge[0].hinge.motorTorque = torque;
      car->wheelHinge[1].hinge.motorTorque = torque;
    } else if (axis.y < 0) {
      car->wheelHinge[0].hinge.motorTorque = -1000.f;
      car->wheelHinge[1].hinge.motorTorque = -1000.f;
    }
  }

  f32 turnRate =
      0.15f *
      (fabsf(axis.y) > 0.1f ? LERP(1.0, 1.0, 1.0f - longSpeedN) : 1.f);

  car->turnAngle = LERP(car->turnAngle, 0.35f * axis.x, turnRate);

  car->wheelHinge[2].hinge.localAxisARotation = {0.0f, 0, 1.0f, -car->turnAngle};
  car->wheelHinge[3].hinge.localAxisARotation = {0.0f, 0, 1.0f, -car->turnAngle};

  car->wheelHinge[2].hinge.localAxisBRotation = {0.0f, 0, 1.0f, car->turnAngle};
  car->wheelHinge[3].hinge.localAxisBRotation = {0.0f, 0, 1.0f, car->turnAngle};

  f32 stepDelta = delta;

  f32 currentTime = 0;
  f32 targetTime = stepDelta;

  if (input->reset == BUTTON_RELEASED) {
    carSetInitialState(game);
    car->body.chassis.velocity = (vec3s){0.0f,0.0f,10.0f};
  }

  static rigid_body terrainBody = {0};
  terrainBody.id = 100;
  terrainBody.friction = 0.6f;
  terrainBody.orientationQuat = GLMS_QUAT_IDENTITY_INIT;

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
        vec3s normal;
        vec3s u = {0};
        if (bShape->type == shape_type::SHAPE_POINT) {
          u = (body->orientation * (bShape->point.p - body->localCenter));
          vec3s worldU = u + body->position;
          depth = getGeometryHeight(worldU, game, &normal);
        } else if (bShape->type == shape_type::SHAPE_SPHERE) {
          depth = getGeometryHeight(body->position + body->localCenter, game,
                                    &normal);
          depth = depth - bShape->sphere.radius;
          u = normal * (depth - bShape->sphere.radius);
        }
        i32 oldContact =
            findOldContact(car->contactPointStorage, contactIdx, shapeIdx);

        if (depth < 0.f) {
          contact_manifold* man = manifold + contactIdx++;
          man->point.separation = depth;
          man->point.localPointA = body->position + u;
          man->point.localPointB = u;
	  man->point.point = body->position + u;
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

    for (u32 i = 0; i < contactIdx; i++) {
      contact_manifold* man = manifold + i;
      if (man->bodyB->id > 0) {
	rigid_body *wheelBody = man->bodyB;
	hinge_joint* wheelJoint = &car->wheelHinge[wheelBody->id - 1].hinge;

        vec3s sideAxis = glms_vec3_rotate(wheelJoint->hingeAxis *
                                    car->body.chassis.orientation,
                                    wheelJoint->localAxisBRotation);
        vec3s upAxis = man->point.normal;
        vec3s forwardAxis = glms_vec3_cross(sideAxis, upAxis);

        // Is wheel
        // Extra downward force
	vec3s extraForce = GLMS_VEC3_ZERO_INIT;
        extraForce = -powf(MAX(longSpeed / 30.f, 0.f), 1.0) * 1000.f * upAxis;
	vec3s wheelWAxis =
	  glms_vec3_rotate(wheelJoint->hingeAxis * wheelBody->orientation,
		     wheelJoint->localAxisARotation);
	f32 wheelAngSpeed = glms_vec3_dot(wheelWAxis,
					  wheelBody->angularVelocity) * 0.3f;
	b32 isWheelStopped = fabs(wheelAngSpeed) < 0.001f;
	f32 slipRatio = 0.f;
        if (fabsf(longSpeed) > 0.1f) {
          f32 slideSgn =
              isWheelStopped ? SIGNF(longSpeed) : SIGNF(wheelAngSpeed);
          f32 slip = MAX((wheelAngSpeed - longSpeed) * slideSgn, 0.f);
          slipRatio = slip / fabs(longSpeed);
        }
        f32 slipPacejka = pacejka(slipRatio, 10.f, 1.9f, 1.0f, 0.97f);
	// wheelJoint->slipRatio = slipRatio;

	// TODO weight transfer
	// addForceToPoint(slipPacejka * 10000.f * -upAxis,
	// 		(vec3s){-2.00f, 0.0f, -0.5f} * car->body.chassis.orientation,
	// 		GLMS_VEC3_ZERO_INIT,
	// 		&extraForces[0],
	// 		&extraTorques[0]);

	wheelBody->friction = 0.9f + slipPacejka * 1.0f;

        f32 wLatSpeed = glms_vec3_dot(wheelBody->velocity, sideAxis);
        f32 wLongSpeed = glms_vec3_dot(wheelBody->velocity, forwardAxis);

        f32 slipAngle = atan2f(wLatSpeed, wLongSpeed);
        f32 latPacejka = pacejka(slipAngle, 0.714  // Stiffness factor
                                 ,1.40  // Shape factor 1.4 .. 1.8
                                 ,1.00  // Peak factor
                                 ,-0.20);  // Curve
        f32 tireLoad = 10000.f;

        extraForce = extraForce - car->body.chassis.sideAxis * tireLoad * latPacejka;
        extraForces[wheelBody->id] = extraForces[wheelBody->id] + extraForce;

      }
    }

    // Prepare contacts
    u32 constraintNum = contactIdx;
    u32 jointNum = arrayLen(car->joints);
    contact_constraint constraints[contactIdx];
    u16 constraintIdx = 0;
    for (u16 idx = 0; idx < constraintNum; idx++) {
      contact_manifold* man = manifold + idx;
      if (man->point.shapeIdx == -1) {
        continue;
      }
      contact_constraint* constraint = constraints + constraintIdx++;
      preSolve(man, constraint, constraintNum, h);
    }

    for (u32 i = 0; i < jointNum; ++i) {
      jointPreSolve(car->joints + i, h);
    }

    // Solve
    // body 2 * substepCount
    // constraint 2 * substepCount (merge warm starting)
    for (int substep = 0; substep < subStepAmount; ++substep) {
      // Integrate velocities
      for (u32 i = 0; i < RIGID_BODY_NUM; i++) {
        integrateVelocities(h, &car->body.bodies[i], extraForces[i], extraTorques[i]);
      }

      // Warm start
      for (u32 i = 0; i < jointNum; ++i) {
	jointWarmStart(car->joints + i);
      }

      // Warm start
      for (u32 i = 0; i < constraintNum; ++i) {
        contact_constraint* constraint = constraints + i;
        warmStart(constraint, constraintNum);
      }
      for (u32 i = 0; i < jointNum; ++i) {
	jointSolve(car->joints + i, h, invH, true);
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
	jointSolve(car->joints + i, h, invH, false);
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

    car->body.chassis.forwardAxis = glms_normalize(glms_quat_rotatev(car->body.chassis.orientationQuat, GLMS_XUP));
    car->body.chassis.sideAxis = glms_normalize(glms_quat_rotatev(car->body.chassis.orientationQuat, GLMS_YUP));

    for (u32 i = 0; i < WHEEL_NUM; i++) {
      car->body.wheels[i].origin =
          car->body.wheels[i].position -
          car->body.wheels[i].localCenter * car->body.wheels[i].orientation;
    }

    currentTime = targetTime;
    targetTime = itDelta;
  }
}

static void carInit(game_state* game,
		    memory_arena* permanentArena, memory_arena* tempArena,
		    rt_render_entry_buffer* rendererBuffer,
		    b32 reload, f32 assetModTime) {

  if (!game->car.initialized || reload) {
      carCreateModel(permanentArena, tempArena, rendererBuffer, game,
                     assetModTime);
    if (!game->car.initialized) carSetInitialState(game);
    carSetupBody(game);
    game->car.initialized = true;
  }
}

static void renderAndUpdateCar(game_state* game,
			       memory_arena* tempArena,
			       rt_render_entry_buffer* rendererBuffer,
			       mat4s view, mat4s proj, f32 delta) {
  // Car simulation
  u64 simStartCount = platformApi->getPerformanceCounter();
  carUpdate(game, delta);
  game->profiler.simulationAcc +=
      platformApi->getPerformanceCounter() - simStartCount;
  carRender(game, tempArena, rendererBuffer, view, proj);
}
