#include <stdlib.h>

#include "sync.h"
#include "cleanupstack.h"
#include "common.h"
#include "vulkan/vulkan_core.h"


typedef struct SyncObjectCleanup {
    VkDevice dev;
    VkSemaphore sem1;
    VkSemaphore sem2;
    VkFence fen;
} SyncObjectCleanup;

typedef struct SemaphoreCleanup {
    VkDevice dev;
    VkSemaphore sem;
} SemaphoreCleanup;

typedef struct FenceCleanup {
    VkDevice dev;
    VkFence fen;
} FenceCleanup;

void destroy_semaphore(void* obj) {
    SemaphoreCleanup* s = (SemaphoreCleanup*) obj;
    vkDestroySemaphore(s->dev,s->sem, NULL);
}

void destroy_fence(void* obj) {
    FenceCleanup* f = (FenceCleanup*)obj;
    vkDestroyFence(f->dev, f->fen, NULL);
}

void destroy_sync_objects(void* obj) {
    struct SyncObjectCleanup* s = (struct SyncObjectCleanup*)obj;
    vkDestroySemaphore(s->dev, s->sem1, NULL);
    vkDestroySemaphore(s->dev, s->sem2, NULL);
    vkDestroyFence(s->dev, s->fen, NULL);
}

bool make_sync_objects(VkDevice dev, const u32 n_max_inflight, VkSemaphore** sem1, VkSemaphore** sem2, VkFence** fen, struct Error* e_out, CleanupStack*cs) {
    
    *sem1 = malloc(sizeof(VkSemaphore)*n_max_inflight);
    *sem2 = malloc(sizeof(VkSemaphore)*n_max_inflight);
    *fen = malloc(sizeof(VkFence)*n_max_inflight);

    CLEANUP_START_NORES(void*)
    *sem1
    CLEANUP_END(memfree)
    CLEANUP_START_NORES(void*)
    *sem2
    CLEANUP_END(memfree)
    CLEANUP_START_NORES(void*)
    *fen
    CLEANUP_END(memfree)


    
    VkSemaphoreCreateInfo sci = {};
    sci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    VkFenceCreateInfo fci = {};
    fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (u32 i = 0; i < n_max_inflight; i++) {
        VkResult r = VK_ERROR_UNKNOWN;

        r = vkCreateSemaphore(dev, &sci, NULL, &(*sem1)[i]);
        CLEANUP_START(SemaphoreCleanup)
        {dev,(*sem1)[i]}
        CLEANUP_END(semaphore)
        VERIFY("syncobj", r)

        r = vkCreateSemaphore(dev, &sci, NULL, &(*sem2)[i]);
        CLEANUP_START(SemaphoreCleanup)
        {dev,(*sem2)[i]}
        CLEANUP_END(semaphore)
        VERIFY("syncobj", r)

        r = vkCreateFence(dev, &fci, NULL, &(*fen)[i]);
        CLEANUP_START(FenceCleanup)
        {dev,(*fen)[i]}
        CLEANUP_END(fence)
        VERIFY("syncobj", r)
    }

    return false;

}

