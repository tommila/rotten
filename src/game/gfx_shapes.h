#include "rotten.h"

static float sQuadVertices[] = {
    -0.5f, 0.5f, 0.0f, 0.5f, 0.5f, 0.0f, 0.5f, -0.5f, 0.0f, -0.5f, -0.5f, 0.0f,
};

static float sQuadColors[] = {0.5f, 0.5f, 0.0f, 1.f, 0.5f, 0.5f, 0.0f, 1.f,
                              0.5f, 0.5f, 0.0f, 1.f, 0.5f, 0.5f, 0.0f, 1.f};

static float sQuadTexCoords[] = {
    0.0f, 0.0f, 1.f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f,
};

static uint16_t sQuadIndices[6] = {0, 1, 2, 2, 3, 0};

static mesh_data sQuadMeshData{sQuadVertices,
                               sQuadTexCoords,
                               sQuadColors,
                               sQuadIndices,
                               ARR_LEN(sQuadVertices) / 3,
                               ARR_LEN(sQuadColors) / 4,
                               ARR_LEN(sQuadTexCoords) / 2,
                               ARR_LEN(sQuadIndices)};
