#ifndef ALL_H
#define ALL_H

#include <string.h>

#include "../../third_party/cglm/include/cglm/struct.h"

#include "../core/types.h"
#include "../core/core.h"
#include "../common/math.h"
#include "../core/mem.h"
#include "../core/rotten_renderer.h"
#include "../rotten_platform.h"

#include "physics.h"
#include "shapes.h"
#include "collision.h"

#include "car_game.h"

#include "../core/mem_arena.c"
#include "../core/mem_free_list.c"

// Use temporay memory arena stbi/cgltf malloc calls.
// This is okay as the data is used only in that frame.
static memory_arena *stack;

static void* _malloc(usize size) { return memArena_alloc(stack, size);}
static void* _realloc(void* ptr, usize newSize) {
  return memArena_realloc(stack, ptr, newSize);
}
static void _free(void* ptr){}

#define CGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION

#define STBI_MALLOC(size) _malloc(size)
#define STBI_REALLOC(p, newSize) _realloc(p, newSize)
#define STBI_FREE(p) _free(p)

#define CGLTF_MALLOC(size) _malloc(size)
#define CGLTF_FREE(p) _free(p)

#include "../ext/stb_image.h"
#include "../ext/cgltf.h"

#include "shapes.c"

#include "collision_solver.cpp"

#include "distance_joint.cpp"
#include "hinge_joint.cpp"
#include "slider_joint.cpp"
#include "axis_joint.cpp"
#include "joint.cpp"

#include "ui.cpp"
#include "mesh_shape.c"
#include "gltf_import.cpp"
#include "terrain.cpp"
#include "car.cpp"
#include "skybox.cpp"

#endif //ALL_H
