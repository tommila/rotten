#include "all.h"

#define MAX_VERTICES 4096
#define VERTEX_DATA_SIZE MAX_VERTICES * sizeof(widget_vertex)
#define INDEX_DATA_SIZE MAX_VERTICES * 3 * sizeof(u32)
#define TOTAL_DATA_SIZE VERTEX_DATA_SIZE + INDEX_DATA_SIZE + KILOBYTES(128)
#define OFFSETOF(structObj, member) ((uptr)(&((structObj *)0)->member))

static str8_buffer string8Buffer(memory_arena* arena, usize size) {
  str8_buffer buf = {
    (u8*)memArena_alloc(arena, size * sizeof(u8)),
    0,
    size
  };
  return buf;
}

static str8_buffer makeString8Buffer(memory_arena* arena, str8 str) {
  str8_buffer buf = {
    (u8*)memArena_alloc(arena, str.len * sizeof(u8)),
    str.len,
    str.len
  };
  memcpy(buf.buffer, str.buffer, str.len * sizeof(u8));
  return buf;
}

static str8 vs = string8( 
    "#version 330\n"
    "in vec2 position;\n"
    "in vec2 texcoord0;\n"
    "in vec4 color0;\n"
    "out vec4 color;\n"
    "out vec2 uv;\n"
    "uniform vec2 disp_size;\n"
    "void main() {\n"
    "   gl_Position = vec4(((position/disp_size)-0.5)*vec2(2.0,-2.0), 0.5, 1.0);\n"
    "   uv = texcoord0;\n"
    "   color = color0;\n"
    "}\n");

static str8 fs = string8(
    "#version 330\n"
    "#define M_PI 3.1415926535897932384626433832795\n"
    "precision mediump float;\n"
    "uniform sampler2D tex;\n"
  // sdf values could be passed as vertex attributes
    "uniform float sdfRoundingFactor;\n"
    "uniform float sdfBezelFactor;\n"
    "uniform float sdfHollowFactor;\n"
    "in vec2 uv;\n"
    "in vec4 color;\n"
    "out vec4 frag_color;\n"
    "float sdRoundBox(in vec2 p, in vec2 b, in float r)\n"
    "{\n"
    "     vec2 q = abs(p) - b + r;\n"
    "     return min(max(q.x, q.y), 0.0) + length(max(q,0.0001)) - r;\n"
    "}\n"
    "void main(){\n"
    "   vec2 uvCentered = (vec2(uv.x, uv.y) * 2.0) - vec2(1.0, 1.0);\n"
    "   float d = sdRoundBox(uvCentered, vec2(1.0,1.0), sdfRoundingFactor);\n"
    // give some slack on the rounding so that line shapes won't be hidden
    "   float a = round(max(d, 0.0) + 0.499)\n;"
    "   d = max(-d, 0.0);\n"
    "   float bezelActive = round(sdfBezelFactor + 0.499);\n" 
    "   float hollowActive = round(sdfHollowFactor + 0.499);\n" 
    "   float bezel = (1.0 - smoothstep(0.0, sdfBezelFactor, abs(d))) * bezelActive;\n"
    "   vec4 c = mix(color, vec4(0.0), a);\n"
    "   frag_color = c * texture(tex, uv.st);\n"
    "   frag_color.rgb = mix(frag_color.rgb, vec3(0.0), bezel);\n "
    "   frag_color.a *= 1.0 - (1.0 - bezel * sdfHollowFactor) * hollowActive;\n"
    "}\n");

typedef struct text_style {
  rgba8 color;
  i32 fontSize;
  i32 lineSpace;
  i32 charSpace;
  i32 spaceLen;
  str8 font;
} text_style;

typedef struct button_style {
  v2 fillMargin;
  f32 roundingFactor;
  f32 bezelFactor;
  rgba8 frameColor;
  rgba8 frameColorPressed;
  rgba8 focusColorAdd;
  rgba8 textColor;
  rgba8 textColorPressed;
} button_style;

typedef struct checkbox_style {
  v2 size;
  v2 fillMargin;
  f32 roundingFactor;
  f32 bezelFactor;
  i32 alignment;
  rgba8 frameColor;
  rgba8 frameColorSelected;
  rgba8 focusColorAdd;
  rgba8 textColor;
  rgba8 textColorPressed;
} checkbox_style;

typedef struct histograph_style {
  i32 rowNum;
  i32 colNum;
  rgba8 gridColor;
  rgba8 curveColor[4];
} histograph_style;

typedef struct widget_theme {
  text_style text;
  button_style button;
  checkbox_style checkbox;
  histograph_style histograph;
  struct {
    f32 roundingFactor;
    f32 bezelFactor;
    rgba8 color;
  } panel;
  u8 globalAlphaReduction;
} widget_theme;

typedef struct widget_vertex {
  v2 pos;
  v2 uv;
  rgba8 col;
} widget_vertex;

enum widget_type {
  widget_type_invalid,
  widget_type_rectangle,
  widget_type_line,
  widget_type_text
};

enum widget_text_alignment {
  widget_text_alignment_left,
  widget_text_alignment_right,
  widget_text_alignment_center,
};

typedef struct bitmap_font {
  rt_image_handle texture;
  v2i size;
  v2 uv;
  v2 padding;
  i32 rowNum;
  i32 colNum;
  char first;
} bitmap_font;

typedef struct ui_widget {
  widget_type type;
  f32 sdfRoundingFactor;
  f32 sdfBezelFactor;
  f32 sdfHollowFactor;
  i32 baseVertex;
  i32 baseElement;
  i32 elementNum;
} ui_widget;

typedef struct widget_basis { 
  v2 position;
  rgba8 color;
} widget_basis; 

typedef struct rectangle_widget {
  widget_basis basis;
  v2 size;
  f32 roundingFactor;
  f32 bezelFactor;
  b32 isHollow;
} rectangle_widget;

typedef struct circle_widget {
  widget_basis basis;
  f32 radius;
  f32 bezelFactor;
  b32 isHollow;
} circle_widget;

typedef struct label_widget {
  widget_basis basis;
  str8_buffer* textBuffer;
  i32 textBufferNum;
  b32 newLine;
  i32 fontSize;
  i32 alignment;
  i32 lineSpace;
  i32 charSpace;
  i32 spaceLen;
 } label_widget;

typedef struct grid_widget {
  widget_basis basis;
  v2 size;
  i32 rowNum;
  i32 colNum;
} grid_widget;

typedef struct curve_widget {
  grid_widget grid;
  f32 *curve;
  i32 curvePointNum;
  rgba8 curveColor;
} curve_widget;

typedef struct ui_widget_context {
  rt_command_buffer buffer;
  memory_arena widgetMemory;
  rt_vertex_array_handle vertexArrayHandle;
  rt_vertex_buffer_handle vertexBufferHandle;
  rt_index_buffer_handle indexBufferHandle;
  rt_image_handle defTexture;    
  rt_shader_program_handle programHandle;
  bitmap_font defaultFont;
  widget_theme mainTheme;  
  ui_widget widgets[256];
  u32 widgetNum;
  widget_vertex* vertexBuffer;
  u32* indexBuffer;
  i32 headVertex;
  i32 headElement;
  v2i displaySize;
} ui_widget_context;

static ui_widget_context* allocUiWidgets(memory_arena* mem) {
  ui_widget_context* ctx = pushType(mem, ui_widget_context);
  memArena_init(&ctx->buffer.arena, memArena_alloc(mem, KILOBYTES(64)), KILOBYTES(64));
  memArena_init(&ctx->widgetMemory, memArena_alloc(mem, TOTAL_DATA_SIZE), TOTAL_DATA_SIZE);
  
  ctx->vertexBuffer = (widget_vertex*)pushSize(&ctx->widgetMemory, VERTEX_DATA_SIZE);
  ctx->indexBuffer = (u32*)pushSize(&ctx->widgetMemory, INDEX_DATA_SIZE);

  return ctx;
}

static void setupUiWidgets(ui_widget_context* ctx) {

  ctx->mainTheme = (widget_theme){
    .text = {
      .color = {180,180,180,255},
      .fontSize = 9,
      .lineSpace = 9,
      .charSpace = 8,
      .spaceLen = 5,
      .font = string8("assets/dos_8x8_font_white.png"),
    },
    .button = {
      .fillMargin = {2,2},
      .roundingFactor = 0.01f,
      .bezelFactor = 0.0f,
      .frameColor = {80, 80, 80, 255},
      .frameColorPressed = {150,150,150,255},
      .focusColorAdd = {40,40,40,0},
      .textColor = {180,180,180,255},
      .textColorPressed = {40,40,40,255},
    },
    .checkbox = {
      .size = {16, 16},
      .fillMargin = {6,6},
      .roundingFactor = 0.1f,
      .bezelFactor = 0.0f,
      .alignment = widget_text_alignment_left,
      .frameColor = {80, 80, 80, 255},
      .frameColorSelected = {150,150,150,255},
      .focusColorAdd = {40,40,40,0},
      .textColor = {180,180,180,255},
      .textColorPressed = {180,180,180,255},
    },
    .histograph = {
      .rowNum = 7,
      .colNum = 5,
      .gridColor = {100,100,100,255},
      .curveColor = {{100,230,100, 255},
        {230,100,100, 255},
        {100,100,230, 255},
        {230,230,100, 255}}
    },
    .panel = {
      .roundingFactor = 0.01,
      .bezelFactor = 0.01,
      .color = {80,80,90,255}
    },
    .globalAlphaReduction = 60
  };

  if (!ctx->vertexBufferHandle){
    rt_command_create_vertex_buffer* cmd =
        rt_pushRenderCommand(&ctx->buffer, create_vertex_buffer);

    cmd->vertexDataSize = VERTEX_DATA_SIZE;
    cmd->vertexData = 0;
    cmd->indexDataSize = INDEX_DATA_SIZE;
    cmd->indexData = 0;
    cmd->isStreamData = true;
    cmd->vertexArrHandle = &ctx->vertexArrayHandle;
    cmd->vertexBufHandle = &ctx->vertexBufferHandle;
    cmd->indexBufHandle = &ctx->indexBufferHandle;

    cmd->vertexAttributes[0] = (rt_vertex_attributes){
      .count = 2, .offset = (int)OFFSETOF(widget_vertex, pos),
      .stride = sizeof(widget_vertex),.normalized = false, .type = rt_data_type_f32};
    cmd->vertexAttributes[1] = {
      .count = 2, .offset = (int)OFFSETOF(widget_vertex, uv),
      .stride = sizeof(widget_vertex),.normalized = false, .type = rt_data_type_f32};
    cmd->vertexAttributes[2] = {
      .count = 4, .offset = (int)OFFSETOF(widget_vertex, col),
      .stride = sizeof(widget_vertex), .normalized = true, .type = rt_data_type_u8};
  }

  if(!ctx->programHandle){
    rt_command_create_shader_program* cmd =
        rt_pushRenderCommand(&ctx->buffer, create_shader_program);
    cmd->vertexShaderData = vs;
    cmd->fragmentShaderData = fs;

    cmd->shaderProgramHandle = &ctx->programHandle;
  }
  else {
    rt_command_update_shader_program* cmd =
      rt_pushRenderCommand(&ctx->buffer, update_shader_program);
    cmd->vertexShaderData = vs;
    cmd->fragmentShaderData = fs;

    cmd->shaderProgramHandle = &ctx->programHandle;
  }
  if(!ctx->defTexture){
    rt_command_create_texture* cmd =
        rt_pushRenderCommand(&ctx->buffer, create_texture);
#if 0  
    rgba8 grey = {128,128,128,255};
    rgba8 white = {255,255,255,255};
    static rgba8 defpixels[64] = {
      white,grey,white,grey,white,grey,white,grey,
      grey,white,grey,white,grey,white,grey,white,
      white,grey,white,grey,white,grey,white,grey,
      grey,white,grey,white,grey,white,grey,white,
      white,grey,white,grey,white,grey,white,grey,
      grey,white,grey,white,grey,white,grey,white,
      white,grey,white,grey,white,grey,white,grey,
      grey,white,grey,white,grey,white,grey,white,
    };
#else
    static uint32_t defpixels[64];
    memset(defpixels, 0xffff, sizeof(defpixels));
#endif
    cmd->imageHandle = &ctx->defTexture;
    cmd->image[0].components = 4;
    cmd->image[0].depth = 8;
    cmd->image[0].width = 8;
    cmd->image[0].height = 8;
    cmd->image[0].pixels = (void*)defpixels;
  }

  if (!ctx->defaultFont.texture) {
    rt_image_data imageData = platformApi->loadImage(
      TO_C(ctx->mainTheme.text.font), 4);
    rt_command_create_texture* cmd = 
    rt_pushRenderCommand(&ctx->buffer, create_texture);
    cmd->imageHandle = &ctx->defaultFont.texture;
    cmd->image[0] = imageData;
    
    i32 colNum = 16;
    i32 rowNum = 16;
    i32 padX = 0;
    i32 padY = 0;
    ctx->defaultFont.size = {9, 9};
    ctx->defaultFont.first = 0; // space character' ';
    ctx->defaultFont.rowNum = rowNum;
    ctx->defaultFont.colNum = colNum;
    //ctx->defaultFont.uv = {1.f, 1.f};
    ctx->defaultFont.padding = (v2){padX / (f32)imageData.height, padY / (f32)imageData.width};
    ctx->defaultFont.uv = (v2){ctx->defaultFont.size.x / (f32)imageData.width, ctx->defaultFont.size.y / (f32)imageData.height};
  }
}

static void readyUiWidgets(ui_widget_context* ctx, v2i displaySize) {
  ctx->headVertex = 0;
  ctx->headElement = 0;
  ctx->widgetNum = 0;
  ctx->displaySize = displaySize;
}

static void drawBezier6Curve(ui_widget_context* ctx,
                             curve_widget widget,
                             f32 *curvePoints) {

  u32 lineNum = 30; 
  u32 indexNum = (lineNum - 1) * 2;
  u32 vertexNum = lineNum;
 
  widget_vertex *vertexData = ctx->vertexBuffer + ctx->headVertex;
  u32 *indexData = ctx->indexBuffer + ctx->headElement;
   
  for (u32 i = 0; i < lineNum; i++) {
    f32 t1 = (f32)i / ((f32)lineNum - 1);
    f32 x = t1 * widget.grid.size.x; 
    f32 y = widget.grid.size.y - bezier6n(
      curvePoints,
      t1) * widget.grid.size.y;

    vertexData[i].pos = (v2){x, y} + widget.grid.basis.position; 
    vertexData[i].uv = (v2){0.f, 0.f}; 
    vertexData[i].col = widget.curveColor;
    if (i < lineNum - 1) {
      indexData[i * 2] = i;
      indexData[i * 2 + 1] = i + 1;
    }
  }
  ui_widget *w = ctx->widgets + ctx->widgetNum++;

  w->baseVertex = ctx->headVertex;
  w->baseElement = ctx->headElement;
  w->elementNum = indexNum;
  w->type = widget_type_line;
  w->sdfRoundingFactor = 0.0f;

  ctx->headVertex += vertexNum;
  ctx->headElement += indexNum;
}

static void drawCurve(ui_widget_context* ctx,
                      curve_widget widget) {

  u32 lineNum = widget.curvePointNum; 
  u32 indexNum = (lineNum - 1) * 2;
  u32 vertexNum = lineNum;
 
  widget_vertex *vertexData = ctx->vertexBuffer + ctx->headVertex;
  u32 *indexData = ctx->indexBuffer + ctx->headElement;
   
  for (u32 i = 0; i < lineNum; i++) {
    f32 y = widget.grid.size.y  - widget.curve[i]; 
    f32 x = widget.grid.size.x * (f32)i / ((f32)lineNum - 1);
    
    vertexData[i].pos = (v2){x, y} + widget.grid.basis.position; 
    vertexData[i].uv = (v2){0.f, 0.f}; 
    vertexData[i].col = widget.curveColor;
    if (i < lineNum - 1) {
      indexData[i * 2] = i;
      indexData[i * 2 + 1] = i + 1;
    }
  }
  ui_widget *w = ctx->widgets + ctx->widgetNum++;

  w->baseVertex = ctx->headVertex;
  w->baseElement = ctx->headElement;
  w->elementNum = indexNum;
  w->type = widget_type_line;
  w->sdfRoundingFactor = 0.0f;
  w->sdfBezelFactor = 0.0f;
  ctx->headVertex += vertexNum;
  ctx->headElement += indexNum;
}


static void drawGrid(ui_widget_context* ctx,
                     grid_widget widget) {
  u32 totalLineNum = widget.colNum * 2 + widget.rowNum * 2;
  u32 vertexNum = totalLineNum;
  u32 indexNum = vertexNum;
  f32 divX = widget.size.x / (widget.colNum - 1);
  f32 divY = widget.size.y / (widget.rowNum - 1);
  widget_vertex *vertexData = ctx->vertexBuffer + ctx->headVertex;
  u32 *indexData = ctx->indexBuffer + ctx->headElement;

  u32 vIdx = 0;
  u32 iIdx = 0;
  for (i32 i = 0; i < widget.colNum; i++) {
    vertexData[vIdx++] = {
      {widget.basis.position.x + i * divX, widget.basis.position.y},
      {0.f,0.f},
      widget.basis.color
    };
    vertexData[vIdx++] = {
      {widget.basis.position.x + i * divX, widget.basis.position.y + widget.size.y},
      {0.f,0.f},
      widget.basis.color
    };
    indexData[iIdx++] = i * 2;
    indexData[iIdx++] = i * 2 + 1;
  }
  for (i32 i = 0; i < widget.rowNum; i++) {
    vertexData[vIdx++] = {
      {widget.basis.position.x, widget.basis.position.y + i * divY},
      {0.f,0.f},
      widget.basis.color
    };
    vertexData[vIdx++] = {
      {widget.basis.position.x + widget.size.x , widget.basis.position.y + i * divY},
      {0.f,0.f},
      widget.basis.color
    };

    indexData[iIdx++] = widget.colNum * 2 + i * 2;
    indexData[iIdx++] = widget.colNum * 2 + i * 2 + 1;
  }
  ui_widget *w = ctx->widgets + ctx->widgetNum++;

  w->baseVertex = ctx->headVertex;
  w->baseElement = ctx->headElement;
  w->elementNum = indexNum;
  w->type = widget_type_line;
  w->sdfRoundingFactor = 0.0f;
  w->sdfBezelFactor = 0.0f,
  ctx->headVertex += vertexNum;
  ctx->headElement += indexNum;
}

static void drawText(ui_widget_context* ctx, label_widget widget) {
  v2 s = (v2){(f32)ctx->defaultFont.size.x * 
    ((f32)widget.fontSize / ctx->defaultFont.size.y),
    (widget.fontSize == 0 ? ctx->defaultFont.size.y : (f32)widget.fontSize)};
  
  f32 ls = (f32)widget.lineSpace;
  f32 cs = (f32)widget.charSpace;  
  f32 sl = (f32)widget.spaceLen;
  
  v2 uv = ctx->defaultFont.uv;
  
  u32 vertexNum = 0;
  u32 indexNum = 0;
  
  for (i32 i = 0; i < widget.textBufferNum; i++) {
    str8_buffer txt = widget.textBuffer[i];
    
    i32 lineLen = 0; 
    i32 cursorPos = 0;
    if (widget.alignment != widget_text_alignment_left) {
      for (u32 d = 0; d < txt.len; d++) {
        char c = txt.buffer[d];
        lineLen += (c == ' ' ? sl : cs);
      }
    }
    for (u32 d = 0; d < txt.len; d++) {
      char c = txt.buffer[d];
      widget_vertex *vertexData = ctx->vertexBuffer + ctx->headVertex + vertexNum;
      u32 *indexData = ctx->indexBuffer + ctx->headElement + indexNum;
      i32 charIdx = c - ctx->defaultFont.first;
      v2 offset = {(f32)widget.basis.position.x, (f32)widget.basis.position.y};

      u32 col = charIdx % ctx->defaultFont.colNum;
      u32 row = charIdx / ctx->defaultFont.colNum;
      f32 alignmentOffset = 0.f;
      if (widget.alignment != widget_text_alignment_left) {
        alignmentOffset = (widget.alignment == widget_text_alignment_center) ?
          lineLen / 2 : lineLen;
      }
      v2 pos = {cursorPos - alignmentOffset +
        (widget.newLine ? 0.0f : i * sl),
        widget.newLine ? i * ls : 0};
      cursorPos += (c == ' ' ? sl : cs);

      vertexData[0].pos = pos + (v2){0.f, 0.f} + offset;
      vertexData[0].uv = {col * uv.x, row * uv.y};
      vertexData[0].col = widget.basis.color;

      vertexData[1].pos = pos + (v2){s.x, 0.f} + offset;
      vertexData[1].uv = {col * uv.x + uv.x, row * uv.y};
      vertexData[1].col = widget.basis.color;

      vertexData[2].pos = pos +  (v2){s.x, s.y} + offset;
      vertexData[2].uv = {col * uv.x + uv.x, row * uv.y + uv.y};
      vertexData[2].col = widget.basis.color; 

      vertexData[3].pos = pos + (v2){0.f, s.y} + offset;
      vertexData[3].uv = {col * uv.x, row * uv.y + uv.y};
      vertexData[3].col = widget.basis.color;

      indexData[0] = 0 + vertexNum;
      indexData[1] = 1 + vertexNum;
      indexData[2] = 2 + vertexNum;

      indexData[3] = 0 + vertexNum;
      indexData[4] = 2 + vertexNum;
      indexData[5] = 3 + vertexNum;

      vertexNum += 4;
      indexNum += 6;
    }
  }
  ui_widget *w = ctx->widgets + ctx->widgetNum++;

  w->baseVertex = ctx->headVertex;
  w->baseElement = ctx->headElement;
  w->elementNum = indexNum;
  w->type = widget_type_text;
  w->sdfRoundingFactor = 0.f;
  w->sdfBezelFactor = 0.f;
  w->sdfHollowFactor = 0.f;

  ctx->headElement += indexNum;
  ctx->headVertex += vertexNum;
}

#define drawRectangleWidget(ctx, widget)\
  _drawRectangle(ctx, widget)

#define drawCircleWidget(ctx, widget) {\
  rectangle_widget rect = {.basis = widget.basis}; \
  rect.size = {widget.radius, widget.radius}; \
  rect.isHollow = widget.isHollow; \
  rect.bezelFactor = widget.bezelFactor; \
  rect.roundingFactor = 1.0; \
  _drawRectangle(ctx, rect); }\

static void _drawRectangle(ui_widget_context* ctx,
                           rectangle_widget widget) {

  widget_vertex *vertexData = ctx->vertexBuffer + ctx->headVertex;
  u32 *indexData = ctx->indexBuffer + ctx->headElement;
  u32 vertexNum = 4;
  u32 indexNum = 6;

  v2 s = widget.size;
  if (!widget.isHollow) {
  }
      vertexData[0].pos = widget.basis.position + (v2){0.f, 0.f};
      vertexData[0].uv = {0.0f, 0.0f};
      vertexData[0].col = widget.basis.color;

      vertexData[1].pos = widget.basis.position + (v2){s.x, 0.f};
      vertexData[1].uv = {1.0f, 0.0f};
      vertexData[1].col = widget.basis.color;

      vertexData[2].pos = widget.basis.position +  (v2){s.x, s.y};
      vertexData[2].uv = {1.0f, 1.0f};
      vertexData[2].col = widget.basis.color; 

      vertexData[3].pos = widget.basis.position + (v2){0.f, s.y};
      vertexData[3].uv = {0.0f, 1.0f};
      vertexData[3].col = widget.basis.color;

      indexData[0] = 0;
      indexData[1] = 1;
      indexData[2] = 2;

      indexData[3] = 0;
      indexData[4] = 2;
      indexData[5] = 3;

  ui_widget *w = ctx->widgets + ctx->widgetNum++;

  w->sdfRoundingFactor = widget.roundingFactor;
  w->sdfBezelFactor = widget.bezelFactor;
  w->sdfHollowFactor = widget.isHollow ? 1.0f : 0.0f;
  w->baseVertex = ctx->headVertex;
  w->baseElement = ctx->headElement;
  w->elementNum = indexNum;
  w->type = widget_type_rectangle;
  ctx->headElement += indexNum;
  ctx->headVertex += vertexNum;
}

static void curveWidget(ui_widget_context* ctx,
                             v2 position, v2 size,
                             str8 label,
                             v2 xAxisRange, v2 yAxisRange,
                             str8 xAxisFormatting, str8 yAxisFormatting,
                             f32* curve,
                             i32 statPointNum
                             ) {
  str8_buffer labelBuf = makeString8Buffer(&ctx->widgetMemory, label);
  label_widget labelWidget = {0};
  labelWidget.basis = {
     .position = position + 
     (v2){size.x / 2, -12.0},
     .color = ctx->mainTheme.text.color
  };
  labelWidget.basis.color.a -= ctx->mainTheme.globalAlphaReduction;
  labelWidget.textBuffer = &labelBuf,
  labelWidget.textBufferNum = 1,
  labelWidget.newLine = false,
  labelWidget.fontSize = ctx->mainTheme.text.fontSize,
  labelWidget.charSpace = 7;
  labelWidget.spaceLen = ctx->mainTheme.text.spaceLen;
  labelWidget.alignment = widget_text_alignment_center;

  drawText(ctx, labelWidget);

  str8_buffer yAxisText[ctx->mainTheme.histograph.rowNum];
  str8_buffer xAxisText[ctx->mainTheme.histograph.colNum];
  label_widget yAxisLabel = labelWidget;
  label_widget xAxisLabel = labelWidget;

  yAxisLabel.textBuffer = yAxisText;
  yAxisLabel.textBufferNum = ctx->mainTheme.histograph.rowNum;
  yAxisLabel.newLine = true;
  yAxisLabel.charSpace = ctx->mainTheme.text.charSpace;  
  yAxisLabel.lineSpace = size.y / (yAxisLabel.textBufferNum - 1);
  yAxisLabel.alignment = widget_text_alignment_right;
  yAxisLabel.basis.position = {
    position.x - 2,
    position.y - 4
  };

  xAxisLabel.textBuffer = xAxisText;
  xAxisLabel.textBufferNum = ctx->mainTheme.histograph.colNum;
  xAxisLabel.newLine = false;
  xAxisLabel.charSpace = ctx->mainTheme.text.charSpace;
  xAxisLabel.spaceLen = size.x / (xAxisLabel.textBufferNum - 1);
  xAxisLabel.lineSpace = ctx->mainTheme.text.lineSpace;
  xAxisLabel.alignment = widget_text_alignment_center;
  xAxisLabel.basis.position = {
    position.x,
    position.y + size.y + 8
  };

  for (i32 i = 0; i < ctx->mainTheme.histograph.rowNum; i++) {
    yAxisText[i] = string8Buffer(&ctx->widgetMemory, 10);
    i32 yLen = snprintf((char*)yAxisText[i].buffer, 10, 
                        (char*)yAxisFormatting.buffer,
                        LERP((f32)yAxisRange.y,
                             (f32)yAxisRange.x, 
                             (f32)i / (yAxisLabel.textBufferNum - 1)));
    yAxisText[i].len = yLen;
  }
  for (i32 i = 0; i < ctx->mainTheme.histograph.colNum; i++) {
    xAxisText[i] = string8Buffer(&ctx->widgetMemory, 10);
    i32 xLen = snprintf((char*)xAxisText[i].buffer, 10,
                        (char*)xAxisFormatting.buffer,
                        LERP((f32)xAxisRange.x,
                             (f32)xAxisRange.y, 
                             (f32)i / (xAxisLabel.textBufferNum - 1)));
    xAxisText[i].len = xLen;
  }
  drawText(ctx, yAxisLabel);
  drawText(ctx, xAxisLabel);
  
  grid_widget grid = {
    .basis = {
      .position = position,
      .color = ctx->mainTheme.histograph.gridColor
    },
    .size = size,
    .rowNum = ctx->mainTheme.histograph.rowNum,
    .colNum = ctx->mainTheme.histograph.colNum,
  };
  
  drawGrid(ctx, grid);
  curve_widget curveWidget = {
    .grid = grid,
    .curve = curve + statPointNum,
    .curvePointNum = statPointNum,
    .curveColor = ctx->mainTheme.histograph.curveColor[0]
  };
  drawBezier6Curve(ctx, curveWidget, curve);
}

static void histographWidget(
  ui_widget_context* ctx,
  v2 position, v2 size,
  str8 label,
  v2 xAxisRange, v2 yAxisRange,
  str8 xAxisFormatting, str8 yAxisFormatting,
  f32 *curve, i32 curvePointNum, i32 curvePointEntries) {
  str8_buffer labelBuf = makeString8Buffer(&ctx->widgetMemory, label);
  label_widget labelWidget = {
   .basis = {
      .position = position + (v2){size.x / 2, -12.0},
      .color = ctx->mainTheme.text.color
    },
    .textBuffer = &labelBuf,
    .textBufferNum = 1,
    .newLine = false,
    .fontSize = ctx->mainTheme.text.fontSize,
    .alignment = widget_text_alignment_center,
    .lineSpace = ctx->mainTheme.text.lineSpace,
    .charSpace = 7,
    .spaceLen = ctx->mainTheme.text.spaceLen,
  };
  labelWidget.basis.color.a -= ctx->mainTheme.globalAlphaReduction;
  drawText(ctx, labelWidget);

  str8_buffer yAxisText[ctx->mainTheme.histograph.rowNum];
  str8_buffer xAxisText[ctx->mainTheme.histograph.colNum];
  label_widget yAxisLabel = labelWidget;
  label_widget xAxisLabel = labelWidget;

  yAxisLabel.textBuffer = yAxisText;
  yAxisLabel.textBufferNum = ctx->mainTheme.histograph.rowNum;
  yAxisLabel.newLine = true;
  yAxisLabel.lineSpace = size.y / (yAxisLabel.textBufferNum - 1);
  yAxisLabel.charSpace = ctx->mainTheme.text.charSpace;
  yAxisLabel.alignment = widget_text_alignment_right;
  yAxisLabel.basis.position = {
    position.x - 2,
    position.y - 4
  };

  xAxisLabel.textBuffer = xAxisText;
  xAxisLabel.textBufferNum = ctx->mainTheme.histograph.colNum;
  xAxisLabel.newLine = false;
  xAxisLabel.spaceLen = size.x / (xAxisLabel.textBufferNum - 1);
  xAxisLabel.lineSpace = ctx->mainTheme.text.charSpace;
  xAxisLabel.alignment = widget_text_alignment_center;
  xAxisLabel.basis.position = {
    position.x,
    position.y + size.y + 8
  };


  for (i32 i = 0; i < ctx->mainTheme.histograph.rowNum; i++) {
    yAxisText[i] = string8Buffer(&ctx->widgetMemory, 10);
    i32 yLen = snprintf((char*)yAxisText[i].buffer, 10, 
                        (char*)yAxisFormatting.buffer,
                        LERP((f32)yAxisRange.y,
                             (f32)yAxisRange.x, 
                             (f32)i / (yAxisLabel.textBufferNum - 1)));
    yAxisText[i].len = yLen;
  }
  for (i32 i = 0; i < ctx->mainTheme.histograph.colNum; i++) {
    xAxisText[i] = string8Buffer(&ctx->widgetMemory, 10);
    i32 xLen = snprintf((char*)xAxisText[i].buffer, 10,
                        (char*)xAxisFormatting.buffer,
                        LERP((f32)xAxisRange.x,
                             (f32)xAxisRange.y, 
                             (f32)i / (xAxisLabel.textBufferNum - 1)));
    xAxisText[i].len = xLen;
  }

  grid_widget grid = {
    .basis = {
      .position = position,
      .color = ctx->mainTheme.histograph.gridColor
    },
    .size = size,
    .rowNum = ctx->mainTheme.histograph.rowNum,
    .colNum = ctx->mainTheme.histograph.colNum,
  };

  drawText(ctx, yAxisLabel);
  drawText(ctx, xAxisLabel);
  drawGrid(ctx, grid);
  for (i32 i = 0; i < curvePointEntries; i++) {
    curve_widget curveWidget = {
      .grid = grid,
      .curve = curve + curvePointNum * i,
      .curvePointNum = curvePointNum,
      .curveColor = ctx->mainTheme.histograph.curveColor[i]
    };

    drawCurve(ctx, curveWidget);
  }
}

static void labelWidget(ui_widget_context* ctx, v2 position,
                        str8 label, i32 alignment = widget_text_alignment_left) {
  label_widget labelWidget = {0};
  str8_buffer labelBuf = makeString8Buffer(&ctx->widgetMemory, label);
  labelWidget.basis = {
     .position = position,
     .color = ctx->mainTheme.text.color
  };
  labelWidget.basis.color.a -= ctx->mainTheme.globalAlphaReduction;
  labelWidget.textBuffer = &labelBuf,
  labelWidget.textBufferNum = 1,
  labelWidget.newLine = false,
  labelWidget.fontSize = ctx->mainTheme.text.fontSize,
  labelWidget.charSpace = ctx->mainTheme.text.charSpace;
  labelWidget.spaceLen = ctx->mainTheme.text.spaceLen;
  labelWidget.alignment = alignment;

  drawText(ctx, labelWidget); 
}

static void panelWidget(ui_widget_context* ctx, v2 position,
                        v2 size) {
  rectangle_widget rect = {
    .basis = {
        .position = position,
        .color = ctx->mainTheme.panel.color
      },
      .size = size,
      .roundingFactor = ctx->mainTheme.panel.roundingFactor,
      .bezelFactor = ctx->mainTheme.panel.bezelFactor,
    };
  rect.basis.color.a -= ctx->mainTheme.globalAlphaReduction;
  drawRectangleWidget(ctx, rect);
}

static b32 checkboxWidget(ui_widget_context* ctx, v2 position,
                          b32 pressed, str8 label,
                          mouse_state mouse)
{
  
  v2 tl = position;
  v2 br = position + ctx->mainTheme.checkbox.size;
  b32 focused = !(mouse.position[0] >= br.x || mouse.position[1] >= br.y ||
                  mouse.position[0] < tl.x || mouse.position[1] < tl.y);
  b32 active = (focused && mouse.button[0] == button_state_pressed);
  rectangle_widget rect = {
    .basis = {
      .position = position,
      .color = ctx->mainTheme.checkbox.frameColor,
    },
    .size = ctx->mainTheme.checkbox.size,
    .roundingFactor = ctx->mainTheme.checkbox.roundingFactor,
    .bezelFactor = ctx->mainTheme.checkbox.bezelFactor,
  };

  rect.basis.color.a -= ctx->mainTheme.globalAlphaReduction;
  if (focused) {
    rect.basis.color += ctx->mainTheme.checkbox.focusColorAdd;
  }
  drawRectangleWidget(ctx, rect);
  if (pressed) {
    rectangle_widget center = rect;
    center.size = center.size - ctx->mainTheme.checkbox.fillMargin;
    center.bezelFactor = 0.0;
    center.basis.position = center.basis.position + ctx->mainTheme.checkbox.fillMargin / 2;
    center.basis.color = ctx->mainTheme.checkbox.frameColorSelected;
    drawRectangleWidget(ctx, center);
  }

  str8_buffer labelBuf = makeString8Buffer(&ctx->widgetMemory, label);
  label_widget labelWidget = {
   .basis = {
      .position = (ctx->mainTheme.checkbox.alignment == widget_text_alignment_center) ?
        rect.basis.position + (v2){rect.size.x, rect.size.y - ctx->mainTheme.text.fontSize / 2} / 2:
        rect.basis.position + (v2){5 + rect.size.x, (f32)rect.size.y * 0.5f - 
        (i32)ctx->mainTheme.text.fontSize / 2},
      .color = (pressed) ? ctx->mainTheme.checkbox.textColorPressed : 
      ctx->mainTheme.checkbox.textColor 
    },
    .textBuffer = &labelBuf,
    .textBufferNum = 1,
    .newLine = false,
    .fontSize = ctx->mainTheme.text.fontSize,
    .alignment = ctx->mainTheme.checkbox.alignment,
    .lineSpace = ctx->mainTheme.text.lineSpace,
    .charSpace = ctx->mainTheme.text.charSpace,
    .spaceLen = ctx->mainTheme.text.spaceLen,
  };
  labelWidget.basis.color.a -= ctx->mainTheme.globalAlphaReduction;
  drawText(ctx, labelWidget);

  return active ? !pressed : pressed;
}

static b32 buttonWidget(ui_widget_context* ctx, v2 position, v2 size,
                          b32 pressed, str8 label,
                          mouse_state mouse)
{
  
  v2 tl = position;
  v2 br = position + size;
  b32 focused = !(mouse.position[0] >= br.x || mouse.position[1] >= br.y ||
                  mouse.position[0] < tl.x || mouse.position[1] < tl.y);
  b32 active = (focused && mouse.button[0] == button_state_pressed);
  rectangle_widget rect = {
    .basis = {
      .position = position,
      .color = ctx->mainTheme.button.frameColor,
    },
    .size = size,
    .roundingFactor = ctx->mainTheme.button.roundingFactor,
    .bezelFactor = ctx->mainTheme.button.bezelFactor,
  };

  rect.basis.color.a -= ctx->mainTheme.globalAlphaReduction;
  if (focused) {
    rect.basis.color += ctx->mainTheme.button.focusColorAdd;
  }
  drawRectangleWidget(ctx, rect);
  if (pressed) {
    rectangle_widget center = rect;
    center.size = center.size - ctx->mainTheme.button.fillMargin;
    center.bezelFactor = 0.0;
    center.basis.position = center.basis.position + ctx->mainTheme.button.fillMargin / 2;
    center.basis.color = ctx->mainTheme.button.frameColorPressed;
    drawRectangleWidget(ctx, center);
  }

  str8_buffer labelBuf = makeString8Buffer(&ctx->widgetMemory, label);
  label_widget labelWidget = {
   .basis = {
      .position = rect.basis.position + 
      (v2){rect.size.x, rect.size.y - ctx->mainTheme.text.fontSize / 2} / 2,
      .color = (pressed) ? ctx->mainTheme.button.textColorPressed : 
      ctx->mainTheme.button.textColor 
    },
    .textBuffer = &labelBuf,
    .textBufferNum = 1,
    .newLine = false,
    .fontSize = ctx->mainTheme.text.fontSize,
    .alignment = widget_text_alignment_center,
    .lineSpace = ctx->mainTheme.text.lineSpace,
    .charSpace = ctx->mainTheme.text.charSpace,
    .spaceLen = ctx->mainTheme.text.spaceLen,
  };
  labelWidget.basis.color.a -= ctx->mainTheme.globalAlphaReduction;
  drawText(ctx, labelWidget);

  return active ? !pressed : pressed;
}

static void renderUIWidgets(ui_widget_context* ctx,
                            memory_arena* tempMemArena) {
  if (ctx->widgetNum == 0) return; 
  {
    rt_command_apply_program *cmd = rt_pushRenderCommand(
      &ctx->buffer, apply_program);
    cmd->programHandle = ctx->programHandle;
    cmd->enableBlending = true;
  } 
  {
    rt_command_update_vertex_buffer* cmd =
      rt_pushRenderCommand(&ctx->buffer, update_vertex_buffer);
    cmd->indexBufHandle = ctx->indexBufferHandle;
    cmd->vertexBufHandle = ctx->vertexBufferHandle;

    cmd->vertexData = ctx->vertexBuffer;
    cmd->vertexDataSize = ctx->headVertex * sizeof(widget_vertex);

    cmd->indexData = ctx->indexBuffer;
    cmd->indexDataSize = ctx->headElement * sizeof(u32); 
  }
  

  widget_type prevType = widget_type_invalid;
  for (u32 wIdx = 0; wIdx < ctx->widgetNum; wIdx++) {
    ui_widget* w = ctx->widgets + wIdx;
   {
      rt_command_apply_uniforms* cmd =
        rt_pushRenderCommand(&ctx->buffer, apply_uniforms);

      void* uniformData = (v2*)pushSize(tempMemArena, sizeof(v2) + sizeof(i32) + sizeof(f32));
      v2* dispSize = (v2*)uniformData;
      dispSize->x = (f32)ctx->displaySize.x;
      dispSize->y = (f32)ctx->displaySize.y;
      i32 *textureIdx = (i32*)((char*)uniformData + sizeof(v2));
      *textureIdx = 0;
      cmd->shaderProgram = ctx->programHandle;
      cmd->uniforms[0] = 
        (rt_uniform_data){.type = rt_uniform_type_vec2, .name = STR("disp_size"), .data = dispSize};
      cmd->uniforms[1] =
        (rt_uniform_data){.type = rt_uniform_type_int, .name = STR("tex"), .data = textureIdx};
      cmd->uniforms[2] =
        (rt_uniform_data){.type = rt_uniform_type_f32, .name = STR("sdfRoundingFactor"),
          .data = &w->sdfRoundingFactor};
      cmd->uniforms[3] =
        (rt_uniform_data){.type = rt_uniform_type_f32, .name = STR("sdfBezelFactor"),
          .data = &w->sdfBezelFactor};
      cmd->uniforms[4] =
        (rt_uniform_data){.type = rt_uniform_type_f32, .name = STR("sdfHollowFactor"),
          .data = &w->sdfHollowFactor};
    }
    {
      if(w->type != prevType){
        rt_command_apply_bindings* cmd =
          rt_pushRenderCommand(&ctx->buffer, apply_bindings);
        cmd->vertexBufferHandle = ctx->vertexBufferHandle;
        cmd->indexBufferHandle = ctx->indexBufferHandle;
        cmd->vertexArrayHandle = ctx->vertexArrayHandle;
        cmd->textureBindings[0] = (rt_binding_data){0};
        cmd->textureBindings[0].textureHandle = w->type == widget_type_text ?
          ctx->defaultFont.texture:
          ctx->defTexture;
        cmd->textureBindings[0].samplerHandle = 0;

        prevType = w->type;
      }

      rt_command_draw_elements* cmd =
        rt_pushRenderCommand(&ctx->buffer, draw_elements);
      cmd->baseElement = w->baseElement;
      cmd->baseVertex = w->baseVertex;
      cmd->numElement = w->elementNum;
      switch (w->type) {
        case widget_type_text:
        case widget_type_rectangle:
          cmd->mode = rt_primitive_triangles;
          break;
        case widget_type_line:
          cmd->mode = rt_primitive_lines;
          cmd->lineWidth = 2.f;
          break;
        InvalidDefaultCase;
      }
    }
  }
}


