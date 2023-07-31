#ifndef DEFAULT_ALIGNMENT
#define DEFAULT_ALIGNMENT (2 * sizeof(void*))
#endif

static uptr alignForward(uptr ptr, usize align) {
  uptr p, a, modulo;

  p = ptr;
  a = (uptr)align;
  // Same as (p % a) but faster as 'a' is a power of two
  modulo = p & (a - 1);

  if (modulo != 0) {
    // If 'p' address is not aligned, push the address to the
    // next value which is aligned
    p += a - modulo;
  }
  return p;
}

typedef struct memory_arena {
  void* buffer;
  usize head;
  usize size;
} memory_arena;

void* memArena_alloc(memory_arena* arena, usize size);
void* memArena_allocUnalign(memory_arena* arena, usize size);

void memArena_free(memory_arena* arena, void* p);
void memArena_clear(memory_arena* arena);
void memArena_init(memory_arena* arena, void* ptr, usize size);

void mem_init(void* ptr, usize size);
void mem_free(void* ptr);
void* mem_malloc(usize size);
void* mem_calloc(usize num, usize len);
void* mem_realloc(void* ptr, usize new_size);

typedef struct mem_free_list {
  void* buffer;
  usize used;
  usize size;
  usize freeNum;
  void* firstFree;
} mem_free_list;

void memFreeList_init(mem_free_list* list, void* ptr, usize size);
void memFreeList_free(mem_free_list* list, void* ptr);
void* memFreeList_malloc(mem_free_list* list, usize size);
void* memFreeList_realloc(mem_free_list* list, void* ptr, usize size);

#define pushArray(memory, count, type) \
  (type *)arenaAlloc(memory, (count) * sizeof(type))
#define pushSize(memory, size) \
  arenaAlloc(memory, size)
#define pushType(memory, type) (type *)arenaAlloc(memory, sizeof(type))
inline void *arenaAlloc(memory_arena *mem, u32 size) {
  void *it = memArena_alloc(mem, size);
  return it;
}
