#include "all.h"
#define makeLabel(context, position, align, bufSize, text, ...)\
  {                                                            \
    char txtBuf[bufSize];                                      \
    snprintf(txtBuf, bufSize, text, __VA_ARGS__);              \
    str8 str = STR(txtBuf);                                \
    str.len = strlen(txtBuf);                                  \
    labelWidget(context, position, str, align);                \
  }

enum ui_section { section_car, section_system, section_visual, _section_num };

#define checkboxFlag(ctx, position, bitField,                 \
                     bitMask, labelStr, mouse)                \
  (u16)setBit(bitField, bitMask,                              \
    checkboxWidget(ctx, position,                             \
      isBitSet(bitField, bitMask), labelStr, mouse));         \

#define HISTORY_SIZE 10

static f32 updateTimer = 0;
static f32 introTimer = 0;
void drawUI(ui_widget_context* widgetContext,
             car_game_state* game,
             v2i displaySize,
             f32 delta, memory_arena* tempArena) {
  car_state* car = &game->car;
  rigid_body* chassis = &car->body.chassis;
  mouse_state* mouse = &game->input.mouse;
  static f32 rpmHistory[HISTORY_SIZE] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  static f32 slipRatioHistory[HISTORY_SIZE * 4] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  };
  static f32 slipAngleHistory[HISTORY_SIZE * 4] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  };
  static f32 frictAdjHistory[HISTORY_SIZE * 4] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  };
  static u32 selectedTabs = section_car;
  static b32 drawHistographs = false;
  static b32 drawCurves = false;

  updateTimer += delta;
  introTimer += delta;
  v2 debugPanelPos = {10, 10};
  v2 debugPanelSize = {450, 300};

  v2 curvePanelSize = {200.f, 180.f};
  v2 curvePadding = {60.f, 55.f};

  if (updateTimer > 0.1f) {
    for (u32 i = 1; i < HISTORY_SIZE; i++) {
      rpmHistory[i - 1] = rpmHistory[i];
      slipRatioHistory[i - 1] = slipRatioHistory[i];
      slipRatioHistory[i - 1 + HISTORY_SIZE] = slipRatioHistory[i + HISTORY_SIZE];
      slipRatioHistory[i - 1 + HISTORY_SIZE * 2] = slipRatioHistory[i + HISTORY_SIZE * 2];
      slipRatioHistory[i - 1 + HISTORY_SIZE * 3] = slipRatioHistory[i + HISTORY_SIZE * 3];
      
      slipAngleHistory[i - 1] = slipAngleHistory[i];
      slipAngleHistory[i - 1 + HISTORY_SIZE] = slipAngleHistory[i + HISTORY_SIZE];
      slipAngleHistory[i - 1 + HISTORY_SIZE * 2] = slipAngleHistory[i + HISTORY_SIZE * 2];
      slipAngleHistory[i - 1 + HISTORY_SIZE * 3] = slipAngleHistory[i + HISTORY_SIZE * 3];
      
      frictAdjHistory[i - 1] = frictAdjHistory[i];
      frictAdjHistory[i - 1 + HISTORY_SIZE] = frictAdjHistory[i + HISTORY_SIZE];
      frictAdjHistory[i - 1 + HISTORY_SIZE * 2] = frictAdjHistory[i + HISTORY_SIZE * 2];
      frictAdjHistory[i - 1 + HISTORY_SIZE * 3] = frictAdjHistory[i + HISTORY_SIZE * 3];
    }
    rpmHistory[HISTORY_SIZE - 1] = (f32)fabs(car->stats.rpm) / 8000.f * curvePanelSize.y;    

    slipRatioHistory[HISTORY_SIZE - 1] = 
      (f32)CLAMP(car->stats.slipRatio[0] / 20.f,0.0f, 1.0f) * curvePanelSize.y;    
    slipRatioHistory[HISTORY_SIZE * 2 - 1] = 
      (f32)CLAMP(car->stats.slipRatio[1] / 20.f,0.0f, 1.0f) * curvePanelSize.y;    
    slipRatioHistory[HISTORY_SIZE * 3 - 1] = 
      (f32)CLAMP(car->stats.slipRatio[2] / 20.f,0.0f, 1.0f) * curvePanelSize.y;    
    slipRatioHistory[HISTORY_SIZE * 4 - 1] = 
      (f32)CLAMP(car->stats.slipRatio[3] / 20.f,0.0f, 1.0f) * curvePanelSize.y;    
    
    slipAngleHistory[HISTORY_SIZE - 1] = 
      (f32)CLAMP(car->stats.slipAngle[0] / 2.f + 0.5f, 0.0f, 1.0f) * curvePanelSize.y;    
    slipAngleHistory[HISTORY_SIZE * 2 - 1] = 
      (f32)CLAMP(car->stats.slipAngle[1] / 2.f + 0.5f, 0.0f, 1.0f) * curvePanelSize.y;    
    slipAngleHistory[HISTORY_SIZE * 3 - 1] = 
      (f32)CLAMP(car->stats.slipAngle[2] / 2.f + 0.5f, 0.0f, 1.0f) * curvePanelSize.y;    
    slipAngleHistory[HISTORY_SIZE * 4 - 1] = 
      (f32)CLAMP(car->stats.slipAngle[3] / 2.f + 0.5f, 0.0f, 1.0f) * curvePanelSize.y;    
    
    frictAdjHistory[HISTORY_SIZE - 1] = 
      (f32)car->stats.frictionAdjustment[0] * curvePanelSize.y;    
    frictAdjHistory[HISTORY_SIZE * 2 - 1] = 
      (f32)car->stats.frictionAdjustment[1] * curvePanelSize.y;    
    frictAdjHistory[HISTORY_SIZE * 3 - 1] = 
      (f32)car->stats.frictionAdjustment[2] * curvePanelSize.y;    
    frictAdjHistory[HISTORY_SIZE * 4 - 1] = 
      (f32)car->stats.frictionAdjustment[3] * curvePanelSize.y;    
    updateTimer -= 0.1f;
  }

  labelWidget(widgetContext,(v2){(f32)displaySize.x - 10.f, 10.f},
            STR(VERSION), widget_text_alignment_right);

  text_style orgStyle = widgetContext->mainTheme.text;
  u32 orgAGlobal = widgetContext->mainTheme.globalAlphaReduction;
  if (game->state == game_state_intro && MAX(sinf(introTimer * 5) + 0.9f,0)) {
    widgetContext->mainTheme.globalAlphaReduction = 0;
    widgetContext->mainTheme.text.color = {50,50,50,255};
    widgetContext->mainTheme.text.fontSize = 32;
    widgetContext->mainTheme.text.charSpace = 28;
    widgetContext->mainTheme.text.spaceLen = 48;
    labelWidget(widgetContext, 
                (v2){(f32)displaySize.x / 2 + 2, (f32)displaySize.y / 2 + 2}, 
                STR("PRESS SPACE TO DRIVE!"),widget_text_alignment_center); 
    widgetContext->mainTheme.text.color = {200,200,200,255};
    labelWidget(widgetContext, 
                (v2){(f32)displaySize.x / 2, (f32)displaySize.y / 2}, 
                STR("PRESS SPACE TO DRIVE!"),widget_text_alignment_center); 

  }
  widgetContext->mainTheme.globalAlphaReduction = orgAGlobal;
  widgetContext->mainTheme.text = orgStyle;

  if (!game->debug.drawDebugPanel) {
    
    if (buttonWidget(widgetContext, (v2){10,10}, {100,20},
                     game->debug.drawDebugPanel,
                     STR("Debug"), *mouse)) {
      game->debug.drawDebugPanel = !game->debug.drawDebugPanel;
    }
    return;
  }

  panelWidget(widgetContext, debugPanelPos, debugPanelSize);
  {
    v2 selectionButtonSize = {(debugPanelSize.x - 20) / 3, 20};
    if (buttonWidget(widgetContext,
                       debugPanelPos + (v2){ 
                       selectionButtonSize.x * section_car,
                       0}, selectionButtonSize, selectedTabs == section_car, STR("Car stats"), *mouse)) {
      selectedTabs = section_car;
    }
    if (buttonWidget(widgetContext,
                       debugPanelPos + (v2){
                       selectionButtonSize.x * section_system,
                       0}, selectionButtonSize, selectedTabs == section_system, STR("System"), *mouse)) {
      selectedTabs = section_system;
    }
    if (buttonWidget(widgetContext,
                       debugPanelPos + (v2){
                       selectionButtonSize.x * section_visual,
                       0}, selectionButtonSize, selectedTabs == section_visual, STR("Visual"), *mouse)) {
      selectedTabs = section_visual;
    }
    if (buttonWidget(widgetContext,
                       debugPanelPos + (v2){
                       selectionButtonSize.x * _section_num,
                       0}, {20,20}, !game->debug.drawDebugPanel, STR("X"), *mouse)) {
      game->debug.drawDebugPanel = false;
      return;
    }
    f32 margin = 27;
    f32 lineHeight = 16;
    v2 cursor = debugPanelPos + (v2){margin, selectionButtonSize.y + lineHeight + 20};
    if (selectedTabs == section_system) {
      makeLabel(widgetContext, cursor, widget_text_alignment_left, 64,
                "Rendering time (ms/frame):  %.4f", 
                game->profiler.average[profiler_counter_entry_rendering]);
      cursor.y += lineHeight;
      makeLabel(widgetContext, cursor, widget_text_alignment_left, 64, "UI time (ms/frame) %.4f ",
                game->profiler.average[profiler_counter_entry_ui]);
      cursor.y += lineHeight;
      makeLabel(widgetContext, cursor, widget_text_alignment_left, 64,
                "Simulation time (ms/frame):  %.4f", 
                game->profiler.average[profiler_counter_entry_simulation]);
      cursor.y += lineHeight;
      makeLabel(widgetContext, cursor, widget_text_alignment_left, 64, 
                "Audio processing time (ms/frame):  %.4f", 
                game->profiler.average[profiler_counter_entry_audio]);
      cursor.y += lineHeight;
      makeLabel(widgetContext, cursor, widget_text_alignment_left, 64, 
                "Total time (ms/frame):  %.4f", 
                game->profiler.average[profiler_counter_entry_total]);
    } else if (selectedTabs == section_car) {
      makeLabel(widgetContext, cursor, widget_text_alignment_left, 64,
                "Speed (km/h):  %.3f",
                v3_dot(chassis->forwardAxis, chassis->velocity) * 3.6f);
      cursor.y += lineHeight;
      makeLabel(widgetContext, cursor, widget_text_alignment_left, 64,
                "RPM (m/s):  %d",(i32)car->stats.rpm);
      makeLabel(widgetContext, cursor + ((v2){debugPanelSize.x * 0.4f,0}),
                widget_text_alignment_left, 64,
                "Gear %d", car->stats.gear + 1);
      cursor.y += lineHeight;
      makeLabel(widgetContext, cursor, widget_text_alignment_left, 64,
                "Velocity (m/s):  %.3f %.3f %.3f ",
                chassis->velocity.x * delta, chassis->velocity.y * delta,
                chassis->velocity.z * delta);
      cursor.y += lineHeight;
      makeLabel(widgetContext, cursor, widget_text_alignment_left, 64,
                "Position:  %.3f %.3f %.3f ",
                chassis->position.x, chassis->position.y,
                chassis->position.z);
      cursor.y += lineHeight;
      makeLabel(widgetContext, cursor, widget_text_alignment_left, 64,
                "Tire extra friction:  %.3f %.3f %.3f %.3f",
                game->car.stats.frictionAdjustment[0], game->car.stats.frictionAdjustment[1],
                game->car.stats.frictionAdjustment[2], game->car.stats.frictionAdjustment[3]);
      cursor.y += lineHeight;
      makeLabel(widgetContext, cursor, widget_text_alignment_left, 64,
                "Tire slip ratio:  %.3f %.3f %.3f %.3f",
                game->car.stats.slipRatio[0], game->car.stats.slipRatio[1],
                game->car.stats.slipRatio[2], game->car.stats.slipRatio[3]);
      cursor.y += lineHeight;
      makeLabel(widgetContext, cursor, widget_text_alignment_left, 64,
                "Tire slip angle:  %.3f %.3f %.3f %.3f",
                game->car.stats.slipAngle[0], game->car.stats.slipAngle[1],
                game->car.stats.slipAngle[2], game->car.stats.slipAngle[3]);
      cursor.y += lineHeight;
      drawHistographs = 
        checkboxWidget(widgetContext, cursor,  
                     drawHistographs, STR("History"), *mouse);
      cursor.y += lineHeight + 2;
      drawCurves = 
        checkboxWidget(widgetContext, cursor,  
                     drawCurves, STR("Property curves"), *mouse);
    } else if (selectedTabs == section_visual) {
      v2 renderingCursor = cursor;
      labelWidget(widgetContext, cursor, STR("Car"));
      renderingCursor.y += lineHeight;
      f32 checkboxSpace = (debugPanelSize.x - 40) / 4;
      game->debug.visibilityState = 
        checkboxFlag(widgetContext, renderingCursor, game->debug.visibilityState,
                     visibility_state_car, STR("Body"), *mouse);
      renderingCursor.x += checkboxSpace;
      game->debug.visibilityState = 
        checkboxFlag(widgetContext, renderingCursor, game->debug.visibilityState,
                     visibility_state_car_colliders, STR("Colliders"), *mouse);
      renderingCursor.x += checkboxSpace;
      game->debug.visibilityState = 
        checkboxFlag(widgetContext, renderingCursor, game->debug.visibilityState,
                     visibility_state_slip_angle, STR("Slip angle"), *mouse);
      renderingCursor.y += lineHeight * 2; renderingCursor.x = cursor.x;
      labelWidget(widgetContext, renderingCursor, STR("Terrain"));
      renderingCursor.y += lineHeight;
      game->debug.visibilityState = 
        checkboxFlag(widgetContext, renderingCursor, game->debug.visibilityState,
                     visibility_state_terrain, STR("Mesh"), *mouse);
      renderingCursor.x += checkboxSpace;
      game->debug.visibilityState = 
        checkboxFlag(widgetContext, renderingCursor, game->debug.visibilityState,
                     visibility_state_terrain_geometry, STR("Geometry"), *mouse);
      renderingCursor.x += checkboxSpace;
      game->debug.visibilityState = 
        checkboxFlag(widgetContext, renderingCursor, game->debug.visibilityState,
                     visibility_state_geometry_normals, STR("Normals"), *mouse);
    }
  }

  v2 curveWindowPos = {displaySize.x - 650.f, 10.f};
  v2 curveWindowDim = {600,560};
  if(drawHistographs){
    panelWidget(widgetContext, curveWindowPos, curveWindowDim);
    v2* frictionValues = pushArray(tempArena, 4, v2);
    v2* slipValues = pushArray(tempArena, 4, v2);

    for (u32 i = 0; i < 4; i++){
      f32 slipRatio = car->stats.slipRatio[i];
      f32 slipAngle = car->stats.slipAngle[i];
      {
        f32 t = CLAMP(slipRatio / 2.f, 0.0f, 1.0f);
        frictionValues[i] = {(t * curvePanelSize.x), curvePanelSize.y - (bezier6n(
          car->properties.slipRatioForceCurve, t) * curvePanelSize.y)};
      }
      {
        f32 t = MIN(fabsf(slipAngle) / 2.f,1.f);
        slipValues[i] = {(t * curvePanelSize.x), curvePanelSize.y - (bezier6n(
          car->properties.slipAngleForceCurve,
          t) * curvePanelSize.y)};
      }
    }
    v2 slipAngleGraphPos = curveWindowPos + curvePadding;
    v2 slipRatioGraphPos = curveWindowPos + 
      (v2){curveWindowDim.x * 0.5f + curvePadding.x, curvePadding.y};

    v2 rpmGraphPos = curveWindowPos + 
      (v2){curvePadding.x, curveWindowDim.y * 0.5f + curvePadding.y};
    v2 frictAdjGraphPos = curveWindowPos + 
      (v2){curveWindowDim.x * 0.5f + curvePadding.x,
        curveWindowDim.y * 0.5f + curvePadding.y};

    histographWidget(widgetContext, slipAngleGraphPos, curvePanelSize,
                          STR("Slip angle"),
                          {1.0f, 0.0f}, {-20.f,20.f},
                          STR("%.1f"), STR("%.0f"),
                          slipAngleHistory, HISTORY_SIZE, 4);
    histographWidget(widgetContext, slipRatioGraphPos, curvePanelSize,
                          STR("Slip ratio"),
                          {1.0f, 0.0f}, {0.0f, 20.f},
                          STR("%.1f"), STR("%.0f"),
                          slipRatioHistory, HISTORY_SIZE, 4);
    histographWidget(widgetContext, frictAdjGraphPos, curvePanelSize,
                          STR("Friction AD"),
                          {1.0f, 0.0f}, {0.f, 1.f},
                          STR("%.1f"), STR("%.1f"),
                          frictAdjHistory, HISTORY_SIZE, 4);
    histographWidget(widgetContext, rpmGraphPos, curvePanelSize,
                          STR("RPM"),
                          {1.0f, 0.0f}, {0.0f, 7000.f}, 
                          STR("%.1f"), STR("%.0f"),
                          rpmHistory, HISTORY_SIZE, 1);
    curveWindowPos.x -= curveWindowDim.x + 10;
  }
  if(drawCurves) {
    v2 slipAnglePanlPos = curveWindowPos + curvePadding;
    v2 slipRationPanelPos = curveWindowPos + 
      (v2){curveWindowDim.x * 0.5f + curvePadding.x, curvePadding.y};
    v2 torqueCurvePanelPos = curveWindowPos + 
      (v2){curvePadding.x, curveWindowDim.y * 0.5f + curvePadding.y};
    panelWidget(widgetContext,
                curveWindowPos,
                curveWindowDim);
    curveWidget(widgetContext, slipAnglePanlPos, curvePanelSize,
                STR("Slip angle friction AD curve"),
                {0.f,20.f}, {0.0f, 1.0f},
                STR("%.0f"), STR("%.1f"),
                car->properties.slipAngleForceCurve,
                arrayLen(car->properties.slipAngleForceCurve));
    curveWidget(widgetContext, slipRationPanelPos, curvePanelSize,
                STR("Slip ratio friction AD curve"),
                {0.f,20.f}, {0.0f, 1.0f},
                STR("%.0f"), STR("%.1f"),
                car->properties.slipRatioForceCurve,
                arrayLen(car->properties.slipRatioForceCurve));
    curveWidget(widgetContext, torqueCurvePanelPos, curvePanelSize,
                STR("Torque curve"),
                {0.0f, 7000.f}, {0.f,car->properties.motorTorque},
                STR("%.0f"), STR("%.0f"),
                car->properties.motorTorqueCurve,
                arrayLen(car->properties.motorTorqueCurve));
  }
} 
