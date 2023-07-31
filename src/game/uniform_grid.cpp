#include <cassert>
#include <cmath>
#include <cstdlib>

#include "cglm/ivec2.h"
#include "cglm/types.h"
#include "cglm/vec2.h"
#include "cglm/vec3.h"
#include "cglm/vec4.h"
#include "rotten.h"

static size_t toCellIdx(vec2 pos, uniform_grid *grid, vec2 *out = nullptr) {
  vec2 cell_xy_fvec = GLM_VEC2_ZERO_INIT;
  ivec2 cell_xy_pos = GLM_IVEC2_ZERO_INIT;
  vec2 inv_cell_size_vec = {grid->invCellW, grid->invCellH};

  glm_vec2_mul(pos, inv_cell_size_vec, cell_xy_fvec);
  // ivec4 cell_xy_vec = (rect - rect_offset) * inv_cell_size_vec;
  ivec2 clamp_vec{grid->numCols - 1, grid->numRows - 1};

  cell_xy_pos[0] = (int)cell_xy_fvec[0];
  cell_xy_pos[1] = (int)cell_xy_fvec[1];

  ivec2 zero = GLM_IVEC2_ZERO_INIT;

  glm_ivec2_maxv(cell_xy_pos, zero, cell_xy_pos);
  glm_ivec2_minv(cell_xy_pos, clamp_vec, cell_xy_pos);

  if (out) {
    (*out)[0] = (float)cell_xy_pos[0] * grid->cellW;
    (*out)[1] = (float)cell_xy_pos[1] * grid->cellH;
  }

  return cell_xy_pos[1] * grid->numCols + cell_xy_pos[0];
}

// // TODO: Need optimizing
int uniformGridgetNeighborCells(int idx, uniform_grid *grid, int *neighbors) {
  ivec2 gridPos = {idx % grid->numRows, idx / grid->numRows};
  int neighborNum = 0;
  for (int yt = -1; yt < 2; yt++) {
    int y = gridPos[1] + yt;
    if (y < 0 || y >= grid->numCols) {
      continue;
    }
    for (int xt = -1; xt < 2; xt++) {
      int x = gridPos[0] + xt;
      if (x < 0 || x >= grid->numRows) {
        continue;
      }
      int nextIdx = (y * grid->numRows) + x;
      if (idx == nextIdx) {
        continue;
      }
      const size_t neighborIdx = (yt + 1) * 3 + (xt + 1);
      if (grid->elements[nextIdx] == -1) {
        neighbors[neighborIdx] = -1;
        continue;
      }
      neighbors[neighborIdx] = grid->elements[nextIdx];
      neighborNum++;
    }
  }

  /// TOP
  // int nextIdx =
  //     fmin(fmax(startIdx + (0 * sGrid->numRows) + 0, 0), sGrid->numCells);
  // neighbors[0] = sGrid->elements[sGrid->indices[nextIdx]].id;
  // neighborNum += bool(neighbors[0] + 1);

  // nextIdx = fmin(fmax(startIdx + (0 * sGrid->numRows) + 1, 0),
  // sGrid->numCells); neighbors[1] =
  // sGrid->elements[sGrid->indices[nextIdx]].id; neighborNum +=
  // bool(neighbors[1] + 1);

  // nextIdx = fmin(fmax(startIdx + (0 * sGrid->numRows) + 2, 0),
  // sGrid->numCells); neighbors[2] =
  // sGrid->elements[sGrid->indices[nextIdx]].id; neighborNum +=
  // bool(neighbors[2] + 1);

  // /// MIDDLE
  // nextIdx = fmin(fmax(startIdx + (1 * sGrid->numRows) + 0, 0),
  // sGrid->numCells); neighbors[3] =
  // sGrid->elements[sGrid->indices[nextIdx]].id; neighborNum +=
  // bool(neighbors[3] + 1); nextIdx = fmin(fmax(startIdx + (1 *
  // sGrid->numRows)
  // + 2, 0), sGrid->numCells); neighbors[5] =
  // sGrid->elements[sGrid->indices[nextIdx]].id; neighborNum +=
  // bool(neighbors[5] + 1);

  // /// BOTTOM
  // nextIdx = fmin(fmax(startIdx + (2 * sGrid->numRows) + 0, 0),
  // sGrid->numCells); neighbors[6] =
  // sGrid->elements[sGrid->indices[nextIdx]].id; neighborNum +=
  // bool(neighbors[6] + 1);

  // nextIdx = fmin(fmax(startIdx + (2 * sGrid->numRows) + 1, 0),
  // sGrid->numCells); neighbors[7] =
  // sGrid->elements[sGrid->indices[nextIdx]].id; neighborNum +=
  // bool(neighbors[7] + 1);

  // nextIdx = fmin(fmax(startIdx + (2 * sGrid->numRows) + 2, 0),
  // sGrid->numCells); neighbors[8] =
  // sGrid->elements[sGrid->indices[nextIdx]].id; neighborNum +=
  // bool(neighbors[8] + 1);

  return neighborNum;
}

void uniformGridCreate(uniform_grid_params params, uniform_grid *grid) {
  grid->posX = params.position[0];
  grid->posY = params.position[1];
  grid->numCols = params.cellsNumXY[0];
  grid->numRows = params.cellsNumXY[1];
  grid->numCells = grid->numCols * grid->numRows;
  grid->cellW = params.cellSize;
  grid->cellH = params.cellSize;
  grid->invCellW = 1.0f / params.cellSize;
  grid->invCellH = 1.0f / params.cellSize;

  assert(MAX_ELEMENTS > grid->numCells);
  for (int i = 0; i < grid->numCells; i++) {
    grid->elements[i] = -1;
  }
}

// void uniformGridClean(StaticGrid *sGrid) { free(sGrid->indices); }

size_t uniformGridInsert(int id, vec2 pos, uniform_grid *grid) {
  vec2 gridXY = {pos[0] - grid->posX, pos[1] - grid->posY};
  size_t nextIdx = toCellIdx(gridXY, grid);
  grid->elements[nextIdx] = id;
  return nextIdx;
}

size_t uniformGridMove(int id, vec2 pos, vec2 newPos, uniform_grid *grid) {
  size_t prevIdx = toCellIdx(pos, grid);
  size_t nextIdx = toCellIdx(newPos, grid);

  if (prevIdx != nextIdx) {
    assert(grid->elements[nextIdx] == -1);
    grid->elements[prevIdx] = -1;
    grid->elements[nextIdx] = id;
  }
  return nextIdx;
}

void uniformGridMoveCell(int id, size_t fromIdx, size_t toIdx,
                         uniform_grid *grid) {
  assert(grid->elements[toIdx] == -1);
  grid->elements[fromIdx] = -1;
  grid->elements[toIdx] = id;
}

// float colors[5][4] = {{0.f, 0.0f, 0.0f, 1.f},
//                       {0.f, 0.8f, 0.0f, 1.f},
//                       {0.8f, 0.0f, 0.0f, 1.f},
//                       {0.f, 0.0f, 0.8f, 1.f},
//                       {1.f, 0.8f, 0.4f, 1.f}};

void uniformGridGetPosition(int idx, uniform_grid *grid, vec2 *pos) {
  ivec2 posXY = {idx % grid->numRows, idx / grid->numRows};
  (*pos)[0] = ((float)posXY[0] * grid->cellW) + grid->posX;
  (*pos)[1] = ((float)posXY[1] * grid->cellH) + grid->posY;
}

// void uniformGridDebugDraw(StaticGrid *sGrid) {
//   // Dyn grid
//   // LOCATION GRID //
//   vec3 positions[sGrid->numCells];
//   vec4 colors[sGrid->numCells];
//   size_t num = 0;
//   for (int ty = 0; ty < sGrid->numCols; ty++) {
//     for (int tx = 0; tx < sGrid->numRows; tx++) {
//       int idx = ty * sGrid->numCols + tx;
//       int tcell_idx = sGrid->indices[idx];
//       vec3 center = {
//           (float)tx * sGrid->cellW + sGrid->posX + sGrid->cellW * 0.5f,
//           (float)ty * sGrid->cellH + sGrid->posY + sGrid->cellH * 0.5f,
//           -5.f};
//       glm_vec3_copy(center, positions[num]);
//       vec4 colorOccupied = {0.0f, 0.75f, 0.75f, 1.0f};
//       vec4 colorNonoccupied = {0.75f, 0.0f, 0.75f, 1.0f};
//       glm_vec4_copy((tcell_idx != -1) ? colorOccupied : colorNonoccupied,
//                     colors[num]);
//       num++;
//     }
//   }
//   debugDraw_drawQuads(positions, colors, num, 1.f, 1.f);

//   // float gcolor[4] = {0.0f, 1.0f, 0.0f, 1.f};
//   // vec2 posGrid = {boids[0].position[0] - dgrid->posX,
//   //                 boids[0].position[1] - dgrid->posY};
//   // vec3 posA = {boids[0].position[0], boids[0].position[1], -3};
//   // SmallList<int> otherBoids =
//   //     dynamicGrid_query(dgrid, posGrid[0], posGrid[1], 1.0f, 1.0f, 0);
//   // debugDraw_drawQuad(posA, 2.f, 2.f, lcolor);
//   // for (int boidIdx = 0; boidIdx < otherBoids.size(); boidIdx++) {
//   //   int idx = otherBoids[boidIdx];
//   //   vec3 posB = {boids[idx].position[0], boids[idx].position[1], -3};
//   //   debugDraw_drawQuad(posB, 2.f, 2.f, gcolor);
//   // }
// }
