#include "buffers.h"
#include "rcs/rcs_mesh.h"
#include "common.h"
#include "rcs/rcs_pipeline.h"

#define S 0.03125

#define LEFT -S
#define RIGHT S
#define TOP S
#define BOT -S

const RcsVertex rcs_verts[4] = {{{LEFT, TOP, 0.0}, {0.0, 0.0, 1.0}},
                         {{RIGHT, TOP, 0.0}, {0.0, 1.0, 0.0}},
                         {{LEFT, BOT, 0.0}, {1.0, 0.0, 0.0}},
                         {{RIGHT, BOT, 0.0}, {1.0, 1.0, 1.0}}};
/*
const RcsVertex rcs_verts[4] = {{{-1.0, -1.0, 0.0}, {0.0, 0.0, 1.0}},
                         {{-1.0, -0.5, 0.0}, {0.0, 1.0, 0.0}},
                         {{-0.5, -1.0, 0.0}, {1.0, 0.0, 0.0}},
                         {{-0.5, -0.7, 0.0}, {1.0, 1.0, 1.0}}};*/


const u16 rcs_indices[6] = {0, 1, 2, 1, 2, 3};


bool make_rcs_mesh(RenderBackend* rb, VkCommandPool cpool, Buffer* vbuf, Buffer* ibuf, CleanupStack* cs) {
    
    make_local_buffer_staged(rb, sizeof(rcs_verts), rcs_verts, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, cpool, vbuf, cs);
    make_local_buffer_staged(rb, sizeof(rcs_indices), rcs_indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, cpool, ibuf, cs);

    vmaSetAllocationName(rb->alloc, vbuf->alloc, "RCS vertex buffer");
    vmaSetAllocationName(rb->alloc, ibuf->alloc, "RCS index buffer");

    return false;
}
