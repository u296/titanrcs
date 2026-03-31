#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include "backend/backend.h"
#include "cleanupstack.h"
#include "vk_mem_alloc.h"

typedef struct RenderBackend RenderBackend;

bool make_allocator(RenderBackend*rb, CleanupStack* cs);

#endif
