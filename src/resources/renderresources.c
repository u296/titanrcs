#include "backend/backend.h"

#include "cleanupstack.h"
#include "common.h"
#include "context.h"
#include "descriptors.h"
#include "res.h"
#include "resources/commandpool.h"
#include "resources/renderresources.h"
#include "resources/sync.h"
#include <assert.h>

#include "buffers.h"
#include <stdlib.h>

typedef struct MappingCleanup {
    VmaAllocator allocctx;
    VmaAllocation alloc;
} MappingCleanup;

void destroy_mapping(void* obj) {
    MappingCleanup* m = (MappingCleanup*)obj;

    vmaUnmapMemory(m->allocctx, m->alloc);
}

bool make_uniform_buffers(const u32 n_max_inflight, RenderBackend* rb,
                          Buffer** ubufs, void*** ubuf_mappings,
                          CleanupStack* cs) {

    *ubufs = malloc(sizeof(Buffer) * n_max_inflight);
    *ubuf_mappings = malloc(sizeof(void*) * n_max_inflight);

    for (u32 i = 0; i < n_max_inflight; i++) {

        char bufname[32];
        sprintf(bufname, "Uniform Buffer %u", i);

        make_buffer(rb, sizeof(InterfaceUbo),
                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, TR_MAPPABLE_WRITE,
                    &(*ubufs)[i], cs);
        vmaMapMemory(rb->alloc, (*ubufs)[i].alloc, &(*ubuf_mappings)[i]);
        vmaSetAllocationName(rb->alloc, (*ubufs)[i].alloc, bufname);
        CLEANUP_START_NORES(MappingCleanup){
            .allocctx = rb->alloc,
            .alloc = (*ubufs)[i].alloc,
        };
        CLEANUP_END(mapping)
    }

    CLEANUP_START_NORES(void*)
    *ubufs CLEANUP_END(memfree) CLEANUP_START_NORES(void*) *
        ubuf_mappings CLEANUP_END(memfree)

            return false;
}

#define CHECK assert(f == false);

bool make_renderresources(RenderContext* ctx, CleanupStack* cs) {
    bool f;
    Error e;

    ctx->resources.n_inflight_frames = N_MAX_INFLIGHT;

    f = make_commandpool(ctx->backend.dev, ctx->backend.queues,
                         &ctx->resources.cmd_pool, &e, cs);
    CHECK

    f = make_uniform_buffers(N_MAX_INFLIGHT, &ctx->backend,
                             &ctx->resources.ubufs,
                             &ctx->resources.ubuf_mappings, cs);
    CHECK

    f = make_commandbuffers(ctx->backend.dev, ctx->resources.cmd_pool,
                            N_MAX_INFLIGHT, &ctx->resources.cmd_bufs, &e, cs);
    CHECK

    f = make_sync_objects(
        ctx->backend.dev, N_MAX_INFLIGHT, ctx->swapchain.n_swpch_img,
        &ctx->resources.img_ready_sems, &ctx->resources.render_finished_sems,
        &ctx->resources.inflight_fncs, &e, cs);
    CHECK
    return false;
}