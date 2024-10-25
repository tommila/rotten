#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_UINT_DRAW_INDEX

#include "../ext/nuklear.h"

#define ui_label(context, align, bufSize, text, ...) \
  {                                                  \
    char txtBuf[bufSize];                            \
    snprintf(txtBuf, bufSize, text, __VA_ARGS__);    \
    nk_label(context, txtBuf, align);                \
  }

enum Section { SYSTEM, CAR, RENDERING, _NUM_SECTION };

void ui_draw(nk_context* ctx, game_state* game, f32 delta) {
  rigid_body* chassis = &game->car.body.chassis;

  f32 invH = 1.f / delta;
  static b32 drawCar = true;
  static b32 drawCarColliders = false;
  static b32 drawTerrain = true;
  static b32 drawTerrainGeometry = false;
  static b32 drawTerrainNormals = false;

  static u32 selectedTabs = SYSTEM;

  nk_byte opacity = 200;
  ctx->style.window.fixed_background.data.color.a = opacity;
  if (nk_begin(ctx, "Show", nk_rect(10, 10, 450, 300), NK_WINDOW_BORDER)) {
    nk_layout_row_static(ctx, 24, 430, 1);
    {
      nk_layout_row_dynamic(ctx, 20, _NUM_SECTION);
      {
        if (nk_option_label(ctx, "System", selectedTabs == SYSTEM)) {
          selectedTabs = SYSTEM;
        }
        if (nk_option_label(ctx, "Car", selectedTabs == CAR)) {
          selectedTabs = CAR;
        }
        if (nk_option_label(ctx, "Rendering", selectedTabs == RENDERING)) {
          selectedTabs = RENDERING;
        }
      }

      if (selectedTabs == SYSTEM) {
        nk_layout_row_dynamic(ctx, 20, 1);
        {
          ui_label(ctx, NK_TEXT_LEFT, 64, "Rendering time (ms/frame) %.4f ",
                   game->profiler.rendering);
          ui_label(ctx, NK_TEXT_LEFT, 64, "Simulation time (ms/frame) %.4f ",
                   game->profiler.simulation);
          ui_label(ctx, NK_TEXT_LEFT, 64, "Total time (ms/frame) %.4f ",
                   game->profiler.total);
        }
      } else if (selectedTabs == CAR) {
        nk_layout_row_dynamic(ctx, 20, 1);
        {
          ui_label(ctx, NK_TEXT_LEFT, 64, "Speed (km/h) %.3f ",
                   glms_vec3_dot(chassis->forwardAxis, chassis->velocity) * 3.6f);
          ui_label(ctx, NK_TEXT_LEFT, 64, "Velocity (m/s) %.3f %.3f %.3f ",
                   chassis->velocity.x * invH, chassis->velocity.y * invH,
                   chassis->velocity.z * invH);
        }
      } else if (selectedTabs == RENDERING) {
        nk_layout_row_static(ctx, 20, 100, 1);
        {
          nk_label(ctx, "Car", NK_TEXT_ALIGN_LEFT);
          nk_layout_row_static(ctx, 16, 100, 2);
          {
            nk_checkbox_label(ctx, "Body", &drawCar);
            game->deve.drawState =
                setBit(game->deve.drawState, DRAW_CAR, drawCar);
            nk_checkbox_label(ctx, "Colliders", &drawCarColliders);
            game->deve.drawState = setBit(game->deve.drawState,
                                          DRAW_CAR_COLLIDERS, drawCarColliders);
          }
          nk_label(ctx, "Terrain", NK_TEXT_ALIGN_LEFT);
          nk_layout_row_static(ctx, 16, 100, 3);
          {
            nk_checkbox_label(ctx, "Mesh", &drawTerrain);
            game->deve.drawState =
                setBit(game->deve.drawState, DRAW_TERRAIN, drawTerrain);
            nk_checkbox_label(ctx, "Geometry", &drawTerrainGeometry);
            game->deve.drawState =
                setBit(game->deve.drawState, DRAW_TERRAIN_GEOMETRY,
                       drawTerrainGeometry);
            nk_checkbox_label(ctx, "Normals", &drawTerrainNormals);
            game->deve.drawState =
                setBit(game->deve.drawState, DRAW_TERRAIN_GEOMETRY_NORMALS,
                       drawTerrainNormals);
          }
        }
      }
    }
  }
  nk_end(ctx);
}
