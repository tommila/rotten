// Auto mergin free list  //
// todo: memory alignment //
typedef struct free_node {
  usize size;
  struct free_node* next;
} free_node;

typedef struct used_node {
  usize size;
  usize padding;
  struct free_node* next;
} used_node;

#define sizeOfFreeNode sizeof(free_node)
#define sizeOfUsedNode sizeof(used_node)

#define minNodeSize 256

void memFreeList_init(mem_free_list* list, void* ptr, usize size) {
  list->used = 0;
  list->size = size;
  list->buffer = ptr;

  free_node *node = (free_node*)ptr;
  node->next = NULL;
  node->size = size;
  list->firstFree = node;
  list->freeNum = 1;
}

void* memFreeList_malloc(mem_free_list* list, usize size) {
  usize requiredSize = size + sizeOfUsedNode;
  free_node* prev = NULL;
  free_node* next = (free_node*)list->firstFree;
  free_node* bestFit = NULL;
  free_node* bestFitPrev = NULL;
  usize bestRemaining = list->size;
  // Find first big enough node
  while (next) {
    if (next->size >= requiredSize) {
      usize remaining = next->size - requiredSize;
      if (remaining < bestRemaining) {
	bestRemaining = remaining;
	bestFit = next;
	bestFitPrev = prev;
      }
      if (remaining < minNodeSize) {
	break;
      }
    }
    prev = next;
    next = next->next;
  }

  ASSERT(bestFit && "Free list out of space");

  if (bestRemaining > minNodeSize) {

    // Split towards the end. This means that the last link should always have
    // a free node (if enough space)
    free_node* splitted = (free_node*)((char*)bestFit + requiredSize);
    splitted->size = bestRemaining;
    splitted->next = bestFit->next;

    if (bestFitPrev == NULL) {
      list->firstFree = splitted;
    } else {
      bestFitPrev->next = splitted;
    }
    list->freeNum++;
  } else {
    requiredSize = bestFit->size;
    if (bestFitPrev == NULL) {
      list->firstFree = bestFit->next;
    } else {
      bestFitPrev->next = bestFit->next;
    }
  }

  used_node* used = (used_node*)bestFit;
  used->size = requiredSize;
  list->used += requiredSize;
  used->padding = 0;
  ASSERT(list->freeNum);
  list->freeNum--;
  void* ptr = (void*)((char*)used + sizeOfUsedNode);
  memset(ptr, 0, size);
  return ptr;
}

void* memFreeList_realloc(mem_free_list* list, void* ptr, usize size) {
  void* newPtr = memFreeList_malloc(list, size);
  if (ptr) {
    used_node* used = (used_node*)((char*)ptr - sizeOfUsedNode);
    memcpy(newPtr, ptr, used->size - sizeOfUsedNode);
    memFreeList_free(list, ptr);
  }
  return newPtr;
}

static void merge(mem_free_list* list) {
  free_node* next = (free_node*)list->firstFree;

  while (next && next->next) {
    free_node* merged = next->next;
    if ((char*)next + next->size == (char*)merged) {
      next->size += merged->size;
      next->next = merged->next;
      ASSERT(list->freeNum);
      list->freeNum--;
    }
    else {
      next = merged;
    }
  }
}

void memFreeList_free(mem_free_list* list, void* ptr) {
  if (ptr == NULL) {
    return;
  }
  used_node* used = (used_node*)((char*)ptr - sizeOfUsedNode);
  usize requiredSize = used->size;

  free_node* free = (free_node*)used;
  free->size = requiredSize;
  free->next = NULL;

  free_node* next = (free_node*)list->firstFree;
  free_node* prev = NULL;

  // Find first free node that is further than
  // the free node
  while (next && (char*)free > (char*)next) {
    prev = next;
    next = next->next;
  }

  if (next) {
    if (prev) {
      prev->next = free;
      free->next = next;
    }
    // Next is first free
    else {
      free->next = next;
      list->firstFree = free;
    }
  }
  else {
    // Free nodes are splitted always forward, meaning that
    // there should be always free node at the last postion.
    // In this case the free node space was exhausted and there
    // were no splitted node.
    if (prev) {
      prev->next = free;
    }
    // Fully loaded free list
    else {
      list->firstFree = free;
    }
  }
  list->used -= requiredSize;
  list->freeNum++;
  merge(list);
}
