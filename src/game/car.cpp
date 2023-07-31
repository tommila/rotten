#include "cglm/affine-pre.h"
#include "cglm/struct/quat.h"
#include "cglm/struct/vec3.h"
#include "collision.h"
#include "rotten.h"
#include "collision_solver.cpp"
#include "distance_joint.cpp"
#include "hinge_joint.cpp"

#define WHEEL_NUM 4
#define RIGID_BODY_NUM 1 + WHEEL_NUM

typedef struct car_vs_uniform_params {
  mat4s modelMat;
  mat4s viewMat;
  mat4s projMat;
} car_vs_uniform_params;

static void readGltfNodeData(memory_arena* tempArena,
                             renderer_command_buffer* rendererBuffer,
                             car_object* car, cgltf_node* node) {
  mesh_data meshData = {0};
  material_data matData = {0};
  mat4 meshTransform;
  u32 meshIdx = car->body_model.meshNum;
  gltf_readMeshData(tempArena, node, "./assets/models/low_poly_car/scene.gltf",
                    &meshData, &meshTransform);
  gltf_readMatData(node, "./assets/models/low_poly_car/scene.gltf", &matData);
  model_mesh_data* modelMeshData = car->body_model.meshData + meshIdx;
  model_material_data* modelMatData = car->body_model.matData + meshIdx;

  modelMeshData->elementNum = meshData.indexNum;
  modelMeshData->childNum = node->children_count;
  modelMeshData->transform = glms_mat4_make(meshTransform[0]);

  if (modelMeshData->elementNum > 0) {
    {
      renderer_command_create_vertex_buffer* cmd = renderer_pushCommand(
          rendererBuffer, renderer_command_create_vertex_buffer);
      cmd->vertexData = meshData.vertexData;
      cmd->indexData = meshData.indices;
      cmd->vertexDataSize =
          sizeof(f32) *
          (meshData.vertexNum +
           meshData.normalNum +
           meshData.colorNum +
           meshData.texCoordNum);
      cmd->indexDataSize = sizeof(u32) * meshData.indexNum;
      cmd->isStreamData = false;
      cmd->vertexBufHandle = &modelMeshData->vertexBufferHandle;
      cmd->indexBufHandle = &modelMeshData->indexBufferHandle;
    }
    modelMatData->baseColorValue = glms_vec4_make(matData.baseColor);
  }
  if (strcmp(node->name, "Wheels") == 0) {
    modelMeshData->inWorldSpace = true;
  }
  if (strcmp(node->name, "WheelBL") == 0)
    car->wheel_transforms.wheelBL = &modelMeshData->transform;
  if (strcmp(node->name, "WheelBR") == 0)
    car->wheel_transforms.wheelBR = &modelMeshData->transform;
  if (strcmp(node->name, "WheelFL") == 0)
    car->wheel_transforms.wheelFL = &modelMeshData->transform;
  if (strcmp(node->name, "WheelFR") == 0)
    car->wheel_transforms.wheelFR = &modelMeshData->transform;

  car->body_model.meshNum++;

  for (cgltf_size childIndex = 0; childIndex < node->children_count;
       ++childIndex) {
    readGltfNodeData(tempArena, rendererBuffer, car,
                     node->children[childIndex]);
  }
}

inline void deleteCar(renderer_command_buffer* rendererBuffer,
                      game_state* game) {
  car_object* car = &game->car;
  {
    renderer_command_free_program_pipeline* cmd = renderer_pushCommand(
        rendererBuffer, renderer_command_free_program_pipeline);
    cmd->pipelineHandle = car->body_model.pipelineHandle;
    cmd->shaderProgramHandle = car->body_model.programHandle;

    car->body_model.pipelineHandle = 0;
    car->body_model.programHandle = 0;
  }
  {
    for (u32 i = 0; i < car->body_model.meshNum; i++) {
      model_mesh_data* meshData = &car->body_model.meshData[i];
      renderer_command_free_vertex_buffer* cmd = renderer_pushCommand(
          rendererBuffer, renderer_command_free_vertex_buffer);
      cmd->vertexBufferHandle = meshData->vertexBufferHandle;
      cmd->indexBufferHandle = meshData->indexBufferHandle;

      meshData->vertexBufferHandle = 0;
      meshData->indexBufferHandle = 0;
    }
    car->body_model.meshNum = 0;
  }
}

static void createCar(memory_arena* permanentArena, memory_arena* tempArena,
                      renderer_command_buffer* rendererBuffer,
                      game_state* game) {
  //////////////////////////////
  //        MESH INIT        ///
  //////////////////////////////
  game_object* car = &game->car;
  *car = {0};
  cgltf_data* carGlTFData =
      gltf_loadFile(tempArena, "./assets/models/low_poly_car/scene.gltf");
  car->body_model.meshNum = 0;

  for (cgltf_size sceneIndex = 0; sceneIndex < carGlTFData->scenes_count;
       ++sceneIndex) {
    cgltf_scene* scene = &carGlTFData->scenes[sceneIndex];
    for (cgltf_size nodeIndex = 0; nodeIndex < scene->nodes_count;
         nodeIndex++) {
      // scene->nodes_count is the amount that root node has, not the total
      cgltf_node* node = scene->nodes[nodeIndex];
      readGltfNodeData(tempArena, rendererBuffer, car, node);
    }
  }

  //////////////////////////////
  //   SHADER AND PIPELINE   ///
  //////////////////////////////

  shader_data carShader = importShader(tempArena, "./assets/shaders/car.vs",
                                       "./assets/shaders/car.fs");
  {
    renderer_command_create_program_pipeline* cmd = renderer_pushCommand(
        rendererBuffer, renderer_command_create_program_pipeline);

    cmd->fragmentShaderData = (char*)carShader.fsData;
    cmd->vertexShaderData = (char*)carShader.vsData;

    cmd->vertexUniformEntries = rt_renderer_allocUniformBuffer(tempArena, 8);
    cmd->fragmentUniformEntries = rt_renderer_allocUniformBuffer(tempArena, 8);

    rt_renderer_pushUniformEntry(&cmd->vertexUniformEntries,
                                 mat4, "model_matrix");
    rt_renderer_pushUniformEntry(&cmd->vertexUniformEntries,
                                 mat4, "view_matrix");
    rt_renderer_pushUniformEntry(&cmd->vertexUniformEntries,
                                 mat4, "proj_matrix");
    rt_renderer_pushUniformEntry(&cmd->fragmentUniformEntries,
                                 vec4, "u_color");
    cmd->shaderProgramHandle = &car->body_model.programHandle;
    cmd->progPipelineHandle = &car->body_model.pipelineHandle;
    cmd->layout = {
        .position = {.format = vertex_format_float3,
                     .offset = 0},
        .normals = {.format = vertex_format_float3,
                    .offset = sizeof(f32) * 3},
        .uv = {.format = vertex_format_float2,
	       .offset = sizeof(f32) * 6},
        .color = {.layoutIdx = -1}};
    cmd->depthTest = true;
  }

  // chassis rigid body
  shape* chassisShape = &car->bodyShapes[0];
  chassisShape->type = shape_type::SHAPE_BOX;
  shape_box box =
      createBoxShape((vec3s){-2.15f, -0.8f, 0.4}, (vec3s){2.15f, 0.8f, 1.2f});
  vec3s boxVertices[8];
  decomposeBoxShape(box, (f32*)boxVertices);
  for (u32 i = 0; i < 8; i++) {
    car->bodyShapes[i].type = shape_type::SHAPE_POINT;
    car->bodyShapes[i].point.p = boxVertices[i];
  }
  car->body.chassis.shapeNum = 8;
  vec3s size = box.size;
  f32 density = 300.0f;
  f32 mass = density * (size.x * size.y * size.z);
  printf("car mass %f\n", mass);
  car->body.chassis.id = 0;
  car->body.chassis.localCenter = (vec3s){0.5f, 0.0f, -0.5f};
  car->body.chassis.position.z = 1.f;
  // car->body.chassis.Orientation = GLMS_MAT3_IDENTITY;
  car->body.chassis.mass = mass;
  car->body.chassis.invMass = 1.0f / mass;
  car->body.chassis.invBodyInertiaTensor = GLMS_MAT3_IDENTITY;
  // Inertia tensor
  f32 bodyNumerator = 1.f / 12.f;
  // Solid cuboid inertia tensor
  car->body.chassis.invBodyInertiaTensor.m00 =
      bodyNumerator / mass * (size.y * size.y + size.z * size.z);
  car->body.chassis.invBodyInertiaTensor.m11 =
      bodyNumerator / mass * (size.x * size.x + size.z * size.z);
  car->body.chassis.invBodyInertiaTensor.m22 =
      bodyNumerator / mass * (size.x * size.x + size.y * size.y);

  car->body.chassis.orientation = GLMS_MAT3_IDENTITY;
  car->body.chassis.orientationQuat = GLMS_QUAT_IDENTITY_INIT;
  car->body.chassis.friction = 0.3f;
  car->body.chassis.angularVelocity = {0.f, 0.f, 0.f};
  car->body.chassis.invWorlInertiaTensor =
      car->body.chassis.orientation * car->body.chassis.invBodyInertiaTensor *
      glms_mat3_transpose(car->body.chassis.orientation);

  // wheel rigid bodies
  for (u32 i = 0; i < WHEEL_NUM; i++) {
    f32 radius = 0.3f;
    shape* sphere = &car->bodyShapes[car->body.chassis.shapeNum + i];
    sphere->type = shape_type::SHAPE_SPHERE;
    sphere->sphere.radius = radius;
    f32 mass = density * 0.75f *
               (4.f / 3.f * 3.1415926535f * (radius * radius * radius));
    printf("wheel mass %f\n", mass);
    vec3s offset = (vec3s){0.f, 0.f, -0.15f} - car->body.chassis.localCenter;
    vec3s pivot = *car->wheel_transforms.t[i] * offset;
    car->body.wheels[i].position =
        pivot + car->body.chassis.position + car->body.chassis.localCenter;
    car->body.wheels[i].id = 1 + i;
    // car->body.chassis.Orientation = GLMS_MAT3_IDENTITY;wheels[i]
    f32 wheelNumerator = 1.f / 4.f;
    car->body.wheels[i].mass = mass;
    car->body.wheels[i].invMass = 1.0f / mass;
    car->body.wheels[i].invBodyInertiaTensor.m00 =
        wheelNumerator / (mass * (radius * radius));
    car->body.wheels[i].invBodyInertiaTensor.m11 =
        wheelNumerator / (mass * (radius * radius));
    car->body.wheels[i].invBodyInertiaTensor.m22 =
        wheelNumerator / (mass * (radius * radius));
    car->body.wheels[i].orientation = GLMS_MAT3_IDENTITY_INIT;
    car->body.wheels[i].orientationQuat = GLMS_QUAT_IDENTITY_INIT;
    // Coefficient of restitution
    car->body.wheels[i].angularVelocity = {0.f, 0.f, 0.f};

    car->body.wheels[i].invWorlInertiaTensor =
        car->body.wheels[i].orientation *
        car->body.wheels[i].invBodyInertiaTensor *
        glms_mat3_transpose(car->body.wheels[i].orientation);
    car->body.wheels[i].friction = 1.0f;
    car->body.wheels[i].shapeNum = 1;

    setupDistanceJoint(&car->wheelSuspension[i], &car->body.chassis,
                       &car->body.wheels[i], pivot, GLMS_VEC3_ZERO_INIT,
                       GLMS_YUP, GLMS_YUP);
    car->wheelSuspension[i].id = i;

    setupHingeJoint(&car->wheelHinge[i], &car->body.chassis,
                    &car->body.wheels[i], pivot, GLMS_VEC3_ZERO_INIT, GLMS_YUP,
                    GLMS_YUP);
    car->wheelHinge[i].id = i;

    if (i < 2) {
      car->wheelHinge[i].motorEnabled = false;
      car->wheelHinge[i].motorMaxTorque = 500.f;
      car->wheelHinge[i].motorSpeed = 100.f;
    }

    *car->wheel_transforms.t[i] = glms_translate(
        *car->wheel_transforms.t[i], car->body.chassis.position + offset);
  }
}

static void drawCar(game_state* game, memory_arena* tempArena,
                    renderer_command_buffer* rendererBuffer,
		    mat4s view, mat4s proj) {
  car_object* car = &game->car;
  mat4s modelMat = car->body_model.transform;

  // platformApi->rendererSetPipeline(car->body_model.pipelineHandle);

  // Max eight levels deep nested children, should be enough.
  if (isBitSet(game->deve.drawState, DRAW_CAR)) {
    mat4s matStack[8];
    u32 childStack[8];

    u32 matStackIdx = 0;
    u32 childNum = 0;

    matStack[matStackIdx] = modelMat;
    childStack[matStackIdx] = 0;

    {
      renderer_command_apply_program_pipeline* cmd = renderer_pushCommand(
          rendererBuffer, renderer_command_apply_program_pipeline);
      cmd->pipelineHandle = car->body_model.pipelineHandle;
    }
    car_vs_uniform_params* vsParams = pushArray(tempArena, car->body_model.meshNum, car_vs_uniform_params);
    for (size_t meshIdx = 0; meshIdx < car->body_model.meshNum; meshIdx++) {
      mat4s m = car->body_model.meshData[meshIdx].transform;
      m = (car->body_model.meshData[meshIdx].inWorldSpace
               ? car->body_model.meshData[meshIdx].transform
               : matStack[matStackIdx] * m);

      if (car->body_model.meshData[meshIdx].childNum > 0) {
        matStackIdx++;
        matStack[matStackIdx] = m;
        childStack[matStackIdx] = childNum;
        childNum = car->body_model.meshData[meshIdx].childNum;
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

      // m = proj * view * m;
      if (car->body_model.meshData[meshIdx].elementNum > 0) {
        {
          renderer_command_apply_bindings* cmd = renderer_pushCommand(
              rendererBuffer, renderer_command_apply_bindings);
          cmd->indexBufferHandle =
              car->body_model.meshData[meshIdx].indexBufferHandle;
          cmd->vertexBufferHandle =
              car->body_model.meshData[meshIdx].vertexBufferHandle;
          cmd->bufferLayout =
              car->body_model.meshData[meshIdx].vertexBufferLayout;
        }

        {
          renderer_command_apply_uniforms* cmd = renderer_pushCommand(
              rendererBuffer, renderer_command_apply_uniforms);

          vsParams[meshIdx].modelMat = m;
          vsParams[meshIdx].viewMat = view;
          vsParams[meshIdx].projMat = proj;

          cmd->vertexUniformDataBuffer.buffer = vsParams + meshIdx;
	  cmd->vertexUniformDataBuffer.bufferSize = sizeof(car_vs_uniform_params);

	  cmd->fragmentUniformDataBuffer.buffer = &car->body_model.matData[meshIdx].baseColorValue;
	  cmd->fragmentUniformDataBuffer.bufferSize = sizeof(vec4s);
        }
        {
          renderer_command_draw_elements* cmd = renderer_pushCommand(
              rendererBuffer, renderer_command_draw_elements);
          cmd->baseElement = 0;
          cmd->numElement = car->body_model.meshData[meshIdx].elementNum;
          cmd->numInstance = 1;
        }
      }
    }
  }

  if (isBitSet(game->deve.drawState, DRAW_CAR_COLLIDERS)) {
    vec3s vertices[8];
    for (u32 i = 0; i < 8; i++) {
      vertices[i] = car->bodyShapes[i].point.p;
    }
    mat4s mvp = proj * view * car->body_model.transform;
    shape_box box = createBoxShape((f32*)vertices);

    {
      renderer_simple_draw_box* cmd =
          renderer_pushCommand(rendererBuffer, renderer_simple_draw_box);

      copyType(box.min.raw, cmd->min, vec3s);
      copyType(box.max.raw, cmd->max, vec3s);
      vec4s c = (vec4s){0.0f, 0.0f, 0.4f, 0.4f};
      copyType(c.raw, cmd->color, vec4s);
      copyType(mvp.raw, cmd->mvp, mat4s);
    }
    for (u32 i = 0; i < car->contactNum; i++) {
      contact_manifold* man = car->manifolds + i;
      shape_box box =
          createBoxShape((vec3s){-0.05f, -0.05f, -0.05f} +
                             man->bodyB->position + man->point.localPointB,
                         (vec3s){0.05f, 0.05f, 0.1f} + man->bodyB->position +
                             man->point.localPointB);
      renderer_simple_draw_box* cmd =
          renderer_pushCommand(rendererBuffer, renderer_simple_draw_box);

      copyType(box.min.raw, cmd->min, vec3s);
      copyType(box.max.raw, cmd->max, vec3s);
      vec4s c = (vec4s){0.0f, 1.0f, 0.0f, 0.8f};
      copyType(c.raw, cmd->color, vec4s);
      copyType(mvp.raw, cmd->mvp, mat4s);
    }
  }
}

#define accelerateFunc(n) (1.0f - MAX(0, n * n * 2.f - 0.5f))
#define decelerateFunc(n) (-1.0f)

static f32 pacejka(f32 slipAngleDegrees) {
  f32 x = slipAngleDegrees;
  f32 B = 0.714;
  f32 C = 1.40;
  f32 D = 1.00;
  f32 E = -0.20;

  return D * sinf(C * atanf(B * x - E * (B * x - atanf(B * x))));
}

static void simulateCar(game_state* game, input_state* input, f32 delta) {
  car_object* car = &game->car;

  vec2s axis = {-(f32)(input->moveLeft >= BUTTON_PRESSED) +
                    (f32)(input->moveRight >= BUTTON_PRESSED),
                -(f32)(input->moveDown >= BUTTON_PRESSED) +
                    (f32)(input->moveUp >= BUTTON_PRESSED)};

  f32 speed = glms_vec3_dot(car->body.chassis.orientation * GLMS_XUP,
			    car->body.chassis.velocity);

  f32 turnRate =
      0.15f *
      (fabsf(axis.x) > 0.1f ? LERP(0.25, 1.0, 1.0f - speed / 35.f) : 1.f);

  car->wheelHinge[0].motorEnabled = axis.y > 0.4f;
  car->wheelHinge[1].motorEnabled = axis.y > 0.4f;

  car->turnAngle = LERP(car->turnAngle, 0.35f * axis.x, turnRate);

  car->wheelHinge[2].localAxisARotation = {0.0f, 0, 1.0f, -car->turnAngle};
  car->wheelHinge[3].localAxisARotation = {0.0f, 0, 1.0f, -car->turnAngle};

  car->wheelHinge[2].localAxisBRotation = {0.0f, 0, 1.0f, car->turnAngle};
  car->wheelHinge[3].localAxisBRotation = {0.0f, 0, 1.0f, car->turnAngle};

  f32 stepDelta = delta;

  f32 currentTime = 0;
  f32 targetTime = stepDelta;

  if (input->reset == BUTTON_RELEASED) {
    car->body.chassis.position.x = 0;
    car->body.chassis.position.y = 0;
    car->body.chassis.position.z = 10;
    // car->body.chassis.velocity.y = 20;
    car->body.chassis.orientation = GLMS_MAT3_IDENTITY_INIT;
    car->body.chassis.orientationQuat = {0.0, 0.0, 0.0, 1.0};
    // car->body.chassis.angularVelocity = (vec3s){0.0f,1.f,-1.f};

    for (i32 i = 0; i < WHEEL_NUM; i++) {
      // car->joints[i].localOriginAnchorA.z = -1.25f;
      car->body.wheels[i].position = car->body.chassis.position +
                                     car->wheelSuspension[i].localOriginAnchorA;
      *car->wheel_transforms.t[i] =
          glms_translate_make(car->body.wheels[i].position) *
          glms_quat_mat4(car->body.wheels[i].orientationQuat);
    }
  }

  static rigid_body terrainBody = {0};
  terrainBody.id = 100;
  terrainBody.friction = 0.25f;
  terrainBody.orientationQuat = GLMS_QUAT_IDENTITY_INIT;

  // TODO: CCD
  while (currentTime < stepDelta) {
    // Calculate forces
    // f32 itDelta = (targetTime - currentTime);
    f32 itDelta = targetTime - currentTime;

    // Body contacts
    i32 shapeIdx = 0;
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
          depth = getGeometryHeight(worldU, game, &normal, false);
        } else if (bShape->type == shape_type::SHAPE_SPHERE) {
          depth = getGeometryHeight(body->position + body->localCenter, game,
                                    &normal, body->id == 2);
          depth = depth - bShape->sphere.radius;
          u = normal * (depth - bShape->sphere.radius);
        }
        i32 oldContact =
            findOldContact(car->manifolds, car->contactNum, shapeIdx);

        if (depth < 0.f) {
          contact_manifold* man =
              car->manifolds +
              ((oldContact != -1) ? oldContact : car->contactNum++);
          contact_manifold_point* mp1 = &man->point;
          man->normal = normal;
          compute_basis(man->normal, man->tangents, man->tangents + 1);
          mp1->separation = depth;
          mp1->localPointA = u;
          mp1->localPointB = u;
          man->bodyB = body;
          man->bodyA = &terrainBody;
          man->contactIdx = shapeIdx;
          if (oldContact == -1) {
            mp1->normalImpulse = 0.f;
            mp1->tangentImpulse[0] = 0.0f;
            mp1->tangentImpulse[1] = 0.0f;
          }
        } else {
          if (oldContact != -1) {
            car->manifolds[oldContact] = car->manifolds[car->contactNum - 1];
            car->manifolds[car->contactNum - 1].contactIdx = -1;
            car->contactNum--;
          }
        }
      }
    }

    f32 h = itDelta / subStepAmount;
    f32 invH = 1.0f / h;

    vec3s extraForces[RIGID_BODY_NUM];
    for (u32 i = 0; i < RIGID_BODY_NUM; i++) {
      extraForces[i] = GLMS_VEC3_ZERO;
    }

    for (u32 i = 0; i < car->contactNum; i++) {
      contact_manifold* man = car->manifolds + i;

      // Is wheel
      if (man->bodyB->id > 0) {
	// Rolling resistance
	vec3s v = man->bodyB->velocity;
        extraForces[i] = -Kdl * 30.f * v;
	f32 longitudinalSpeed = glms_vec3_dot(GLMS_XUP * car->body.chassis.orientation, v);
	// Extra downward force
	extraForces[i] = extraForces[i] + -powf(MAX(longitudinalSpeed / 30.f,0.f), 1.0) * 250.f * man->normal;
      }
    }

    // Prepare contacts
    u32 constraintNum = car->contactNum;
    contact_constraint constraints[car->contactNum];
    preSolve(car->manifolds, constraints, constraintNum, h);

    for (int i = 0; i < WHEEL_NUM; ++i) {
      // Slip ratio
      bool warmStart = true;
      distance_joint* hj = car->wheelSuspension + i;
      preSolveDistanceJoint(hj, h, warmStart);
      hinge_joint* dj = car->wheelHinge + i;
      preSolveHingeJoint(dj, h, warmStart);
      f32 wheelSpinSpeed = glms_vec3_dot(dj->mA1,
					     dj->bodyB->angularVelocity) *
                       car->bodyShapes[car->body.chassis.shapeNum + 1].sphere.radius;
      f32 longitudinalSpeed = glms_vec3_dot(GLMS_XUP * dj->bodyA->orientation, dj->bodyB->velocity);

      // TODO: add pacejka
      // f32 lateralSpeed = glms_vec3_dot(dj->mA1, dj->bodyB->velocity);
      // f32 slipAngleRadians = atan2(lateralSpeed, longitudinalSpeed);//slipangle in radians

      b32 isWheelStopped = fabs(wheelSpinSpeed) < 0.001f;
      f32 slideSgn = isWheelStopped ? SIGNF(longitudinalSpeed) : SIGNF(wheelSpinSpeed);
      f32 slip = (wheelSpinSpeed - longitudinalSpeed) * slideSgn;
      dj->slipRatio = slip / fabs(longitudinalSpeed);
    }

    // Solve
    // body 2 * substepCount
    // constraint 2 * substepCount (merge warm starting)
    for (int substep = 0; substep < subStepAmount; ++substep) {
      // Integrate velocities
      for (u32 i = 0; i < RIGID_BODY_NUM; i++) {
        integrateVelocities(h, &car->body.bodies[i], extraForces[i]);
      }

      // Warm start
      for (int i = 0; i < WHEEL_NUM; ++i) {
        distance_joint* dj = car->wheelSuspension + i;
        warmStartDistanceJoint(dj);
        hinge_joint* hj = car->wheelHinge + i;
        warmStartHingeJoint(hj);
      }

      // Warm start
      warmStart(constraints, constraintNum);

      for (int i = 0; i < WHEEL_NUM; ++i) {
        distance_joint* dj = car->wheelSuspension + i;
        solveDistanceJoint(dj, h, invH, true);
        hinge_joint* hj = car->wheelHinge + i;
        solveHingeJoint(hj, h, invH, true);
      }

      // Solve velocities using position bias
      solve(constraints, constraintNum, invH, true);

      // Integrate positions using biased velocities
      for (u32 i = 0; i < RIGID_BODY_NUM; i++) {
        integratePositions(h, &car->body.bodies[i]);
      }

      // Relax biased velocities and impulses.
      // Relaxing the impulses reduces warm starting overshoot.

      for (int i = 0; i < WHEEL_NUM; ++i) {
        distance_joint* dj = car->wheelSuspension + i;
        solveDistanceJoint(dj, h, invH, false);
        hinge_joint* hj = car->wheelHinge + i;
        solveHingeJoint(hj, h, invH, false);
      }

      // Solve velocities using position bias
      solve(constraints, constraintNum, invH, false);
    }

    // finalizePositions(&car->body.chassis);
    for (u32 i = 0; i < RIGID_BODY_NUM; i++) {
      finalizePositions(&car->body.bodies[i]);
    }

    storeContactImpulse(car->manifolds, constraints, constraintNum);

    car->body.chassis.origin =
        car->body.chassis.position -
        car->body.chassis.orientation * car->body.chassis.localCenter;

    car->body_model.transform =
        glms_translate_make(car->body.chassis.origin) *
        glms_mat3_make_mat4(car->body.chassis.orientation);
    for (u32 i = 0; i < WHEEL_NUM; i++) {
      car->body.wheels[i].origin =
          car->body.wheels[i].position -
          car->body.wheels[i].localCenter * car->body.wheels[i].orientation;

      *car->wheel_transforms.t[i] =
          glms_translate_make(car->body.wheels[i].origin) *
          glms_mat3_make_mat4(car->body.wheels[i].orientation);
    }

    currentTime = targetTime;
    targetTime = itDelta;
  }
}
