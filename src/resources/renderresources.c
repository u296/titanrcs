#include "backend/backend.h"

#include "cleanupstack.h"
#include "context.h"
#include "descriptors.h"
#include "resources/commandpool.h"
#include "resources/renderresources.h"
#include "resources/sync.h"
#include <assert.h>

#include "buffers.h"
#include <stdlib.h>

bool make_uniform_buffers(const u32 n_max_inflight, RenderBackend* rb,
                          Buffer** ubufs, void*** ubuf_mappings, CleanupStack* cs) {

    *ubufs = malloc(sizeof(Buffer) * n_max_inflight);
    *ubuf_mappings = malloc(sizeof(void*) * n_max_inflight);

    for (u32 i = 0; i < n_max_inflight; i++) {


        make_buffer(rb, sizeof(UniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, true, &(*ubufs)[i], cs);
        vmaMapMemory(rb->alloc, (*ubufs)[i].alloc, &(*ubuf_mappings)[i]);
    }

    CLEANUP_START_NORES(void*)
    *ubufs CLEANUP_END(memfree) CLEANUP_START_NORES(void*) * ubuf_mappings CLEANUP_END(memfree)

                                                                 return false;
}

#define CHECK assert(f == false);

bool make_renderresources(RenderContext* ctx, CleanupStack* cs) {
    bool f;
    Error e;

    constexpr u32 n_max_inflight = 2;
    ctx->resources.n_inflight_frames = n_max_inflight;

    f = make_commandpool(ctx->backend.dev, ctx->backend.queues, &ctx->resources.cmd_pool, &e, cs);
    CHECK

    f = make_uniform_buffers(n_max_inflight, &ctx->backend, &ctx->resources.ubufs, &ctx->resources.ubuf_mappings, cs);
    CHECK

    f = make_commandbuffers(ctx->backend.dev, ctx->resources.cmd_pool, n_max_inflight,
                            &ctx->resources.cmd_bufs, &e, cs);
    CHECK

    f = make_sync_objects(ctx->backend.dev, n_max_inflight, &ctx->resources.img_ready_sems,
                          &ctx->resources.render_finished_sems, &ctx->resources.inflight_fncs, &e,
                          cs);
    CHECK
    return false;
}