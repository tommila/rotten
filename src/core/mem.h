#include <string.h>
#ifdef __cplusplus
extern "C" {
#endif

#if  defined(__SANITIZE_ADDRESS__)
void __asan_poison_memory_region(void const volatile *addr, size_t size);
void __asan_unpoison_memory_region(void const volatile *addr, size_t size);
#define ASAN_POISON_MEMORY_REGION(addr, size) \
  __asan_poison_memory_region((addr), (size))
#define ASAN_UNPOISON_MEMORY_REGION(addr, size) \
  __asan_unpoison_memory_region((addr), (size))
#else
#define ASAN_POISON_MEMORY_REGION(addr, size) \
  ((void)(addr), (void)(size))
#define ASAN_UNPOISON_MEMORY_REGION(addr, size) \
  ((void)(addr), (void)(size))
#endif

#ifdef __cplusplus
}
#endif

#ifndef DEFAULT_ALIGNMENT
#define DEFAULT_ALIGNMENT (2 * sizeof(void*))
#endif

static inline uptr alignForward(uptr ptr, usize align) {
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
  usize remaining;
} memory_arena;

void* memArena_alloc(memory_arena* arena, usize size);
void* memArena_calloc(memory_arena* arena, usize membNum, usize size);
void* memArena_realloc(memory_arena* arena, void* ptr, usize size);
void* memArena_allocUnalign(memory_arena* arena, usize size);

void memArena_free(memory_arena* arena, void* p);
void memArena_clear(memory_arena* arena);
void memArena_init(memory_arena* arena, void* ptr, usize size);

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


