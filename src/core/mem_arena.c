// Arena allocator
void* memArena_alloc(memory_arena* arena, usize size) {
  uptr ptr = (uptr)arena->buffer + arena->head;
  usize alignedPtr = alignForward(ptr, DEFAULT_ALIGNMENT);
  usize requiredSize = size + (alignedPtr - ptr);
  ASSERT(arena->head + requiredSize < arena->size);
  arena->head += requiredSize;
  return (void*)alignedPtr;
}

void* memArena_realloc(memory_arena* arena, void* ptr, usize size) {
  void* newPtr = memArena_alloc(arena, size);
  if (ptr) {
    memcpy(newPtr, ptr, size);
  }
  return newPtr;
}

void* memArena_allocUnalign(memory_arena* arena, usize size) {
  uptr ptr = (uptr)arena->buffer + arena->head;
  ASSERT(arena->head + size < arena->size);
  arena->head += size;
  return (void*)ptr;
}

void memArena_free(memory_arena* arena, void *p) {
  // Do nothing
}

void memArena_clear(memory_arena* arena) {
  arena->head = 0;
}

void memArena_init(memory_arena* arena, void *ptr, usize size) {
  arena->buffer = ptr;
  arena->size = size;
  memArena_clear(arena);
}
