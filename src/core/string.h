#define string8(s) (str8){(u8*)s, arrayLen(s) - 1}
#define STR string8
#define TO_C(s) (char*)s.buffer

typedef struct str8 {
  u8* buffer;
  usize len;
} str8;

typedef struct str8_buffer {
  u8 *buffer;
  usize len;
  usize size;
} str8_buffer;
