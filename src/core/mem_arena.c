#define pushArray(memory, count, type) \
  (type *)arenaAlloc(memory, (count) * sizeof(type))
#define pushArrayZeros(memory, count, type) \
  (type *)arenaAllocZeros(memory, (count) * sizeof(type))
#define pushSize(memory, size) \
  arenaAlloc(memory, size)
#define pushSizeZeros(memory, size) \
  arenaAllocZeros(memory, size)
#define pushType(memory, type) (type *)arenaAlloc(memory, sizeof(type))

static void *arenaAlloc(memory_arena *mem, usize size) {
  void *it = memArena_alloc(mem, size);
  return it;
}
static void *arenaAllocZeros(memory_arena *mem, usize size) {
  void *it = memArena_alloc(mem, size);
  memset(it, 0, size); 
  return it;
}

void* memArena_alloc(memory_arena* arena, usize size) {
  uptr ptr = (uptr)arena->buffer + arena->head;
  usize alignedPtr = alignForward(ptr, DEFAULT_ALIGNMENT);
  usize requiredSize = size + (alignedPtr - ptr);
  ASAN_UNPOISON_MEMORY_REGION((void*)alignedPtr, requiredSize);
  ASSERT(requiredSize <= arena->remaining);
  arena->head += requiredSize;
  arena->remaining -= requiredSize;
  return (void*)alignedPtr;
}

void* memArena_calloc(memory_arena* arena, usize membNum, usize size) {
  return arenaAllocZeros(arena, membNum * size);
}
void* memArena_realloc(memory_arena* arena, void* ptr, usize size) {
  void* newPtr = memArena_alloc(arena, size);
  u32 diff = (uptr)newPtr - (uptr)ptr;
  if (ptr) {
    // Clamp copy size if it's overlapping with
    // the copy address.
    memcpy(newPtr, ptr, (diff < size) ? diff : size);
  }
  return newPtr;
}

void* memArena_allocUnalign(memory_arena* arena, usize size) {
  void* ptr = (void*)((uptr)arena->buffer + arena->head);
  ASAN_UNPOISON_MEMORY_REGION(ptr, size);
  ASSERT(size < arena->remaining);
  arena->head += size;
  return ptr;
}

void memArena_free(memory_arena* arena, void *p) {
  // Do nothing
}

void memArena_clear(memory_arena* arena) {
  arena->head = 0;
  arena->remaining = arena->size;
  ASAN_POISON_MEMORY_REGION(arena->buffer, arena->size);
}

void memArena_init(memory_arena* arena, void *ptr, usize size) {
  arena->buffer = ptr;
  arena->size = size;
  memArena_clear(arena);
}
