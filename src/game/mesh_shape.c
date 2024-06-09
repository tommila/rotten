#include "car_game.h"

inline mesh_data mesh_GridShape(memory_arena *arena,
				f32 gridSizeX,
				f32 gridSizeY,
				u16 cellsX,
				u16 cellsY,
				b32 hasTexture,
				b32 hasColor,
				primitive_type type) {
  mesh_data data = {0};

  u32 vertexNum = cellsX * cellsY;
  u32 posNum = vertexNum * 3;
  u32 uvNum = vertexNum * 2 * (hasTexture);
  u32 colNum = vertexNum * 4 * (hasColor);

  f32 cellWidth = gridSizeX / (f32)cellsX;
  f32 cellHeight = gridSizeY / (f32)cellsY;

  data.vertexData = pushArray(
		arena,
		posNum + uvNum + colNum, f32);

  u32 indiceNumPerCell = 0;
  switch (type) {
    case PRIMITIVE_LINES:
      indiceNumPerCell = 10;
      break;
    case PRIMITIVE_TRIANGLES:
      indiceNumPerCell = 6;
      break;
      InvalidDefaultCase;
  };
  u32 indexNum = (cellsX * cellsY) * indiceNumPerCell;
  data.indices = pushArray(arena, indexNum, u32);

  u32 iIt = 0;
  u32 vIt = 0;
  u32 uIt = posNum;
  u32 cIt = posNum + uvNum;

  f32 invUvX = 1.0f / (f32)cellsX;
  f32 invUvY = 1.0f / (f32)cellsY;

  f32 offsetX = gridSizeX * 0.5f;
  f32 offsetY = gridSizeY * 0.5f;

  for (u32 y = 0; y < cellsY; y++){
    for (u32 x = 0; x < cellsX; x++){

      data.vertexData[vIt++] = (f32)x * cellWidth - offsetX;
      data.vertexData[vIt++] = (f32)y * cellHeight - offsetY;
      data.vertexData[vIt++] = 0.0f;

      if (hasTexture) {
        data.vertexData[uIt++] = invUvX * (f32)x;
        data.vertexData[uIt++] = invUvY * (f32)y;
      }

      if (hasColor) {
        data.vertexData[cIt++] = 0.f;
        data.vertexData[cIt++] = 1.f;
        data.vertexData[cIt++] = 0.f;
        data.vertexData[cIt++] = 1.f;
      }
    }
  }

  if (type == PRIMITIVE_TRIANGLES) {
    for (u16 y = 0; y < cellsY - 1; y++) {
      for (u16 x = 0; x < cellsX - 1; x++) {
        data.indices[iIt++] = y * cellsX + x;
        data.indices[iIt++] = y * cellsX + (x + 1);
        data.indices[iIt++] = (y + 1) * cellsX + (x + 1);

        data.indices[iIt++] = y * cellsX + x;
        data.indices[iIt++] = (y + 1) * cellsX + (x + 1);
        data.indices[iIt++] = (y + 1) * cellsX + x;
      }
    }
  }
  else if(type == PRIMITIVE_LINES) {
    for (u16 y = 0; y < cellsY - 1; y++) {
      for (u16 x = 0; x < cellsX - 1; x++) {
	data.indices[iIt++] = y * (cellsX) + (x + 1);
	data.indices[iIt++] = (y + 1) * (cellsX) + (x + 1);

	data.indices[iIt++] = (y + 1) * (cellsX) + (x + 1);
        data.indices[iIt++] = y * (cellsX) + x;

        data.indices[iIt++] = y * (cellsX) + x;
        data.indices[iIt++] = y * (cellsX) + (x + 1);

        data.indices[iIt++] = (y + 1) * (cellsX) + (x + 1);
        data.indices[iIt++] = (y + 1) * (cellsX) + x;

	data.indices[iIt++] = (y + 1) * (cellsX) + x;
	data.indices[iIt++] = y * (cellsX) + x;
      }
    }
  }

  data.vertexNum = posNum;
  data.colorNum = colNum;
  data.texCoordNum = uvNum;
  data.indexNum = indexNum;

  return data;
}
