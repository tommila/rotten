#include "cglm/cam.h"
#include "cglm/ivec3.h"
#include "cglm/util.h"
#include "cglm/vec3.h"

static float zoom[] = {0.5f, 0.75f, 1.0f, 1.25f, 1.5f, 1.75f};

void cameraMove(float delta, vec2 direction, camera_state* cam) {
  vec3 offset = {direction[0] * cam->moveSpeed * delta,
                 direction[1] * cam->moveSpeed * delta, 0};
  glm_vec3_add(cam->eye, offset, cam->eye);
}

void cameraZoom(uint32_t z, camera_state* cam) {
  cam->zoomIdx -= z;
  cam->zoomIdx = GLM_MIN(GLM_MAX(cam->zoomIdx, 0), 5);
  cam->eye[2] = zoom[cam->zoomIdx] * 2.0f;
}

void cameraGetViewMat(mat4* viewMtx, camera_state* cam) {
  glm_look(cam->eye, cam->dir, cam->up, *viewMtx);
}

void cameraGetProjectionMat(mat4* projMtx, camera_state* cam) {
  // glm_perspective(1.f, float(1920) / float(1080), 0.1f, 100.0f, proj);
  glm_ortho(-20.f * cam->aspectRatio * zoom[cam->zoomIdx],
            20.f * cam->aspectRatio * zoom[cam->zoomIdx],
            -20.f * zoom[cam->zoomIdx], 20.f * zoom[cam->zoomIdx], -1000.f,
            100.0f, *projMtx);
}
