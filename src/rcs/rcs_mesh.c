#include "rcs/rcs_mesh.h"
#include "backend/backend.h"
#include "buffers.h"
#include "common.h"
#include "rcs/rcs_pipeline.h"
#include <stdlib.h>
#include <stlfile.h>
#include <vulkan/vulkan_core.h>

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

void preprocess_verts(float* raw_verts, u32 n_verts, RcsVertex* new_buf) {

    const float hardcode_scaling = 1.0;

    for (u32 i = 0; i < n_verts; i++) {

        // for now, ignore the normals

        new_buf[i] = (RcsVertex){{raw_verts[i * 3] * hardcode_scaling,
                                  raw_verts[i * 3 + 1] * hardcode_scaling,
                                  raw_verts[i * 3 + 2] * hardcode_scaling},
                                 {1.0, 0.0, 0.0}};
    }
}

bool make_rcs_mesh(RenderBackend* rb, VkCommandPool cpool, Buffer* vbuf,
                   Buffer* ibuf, u32* n_indices, CleanupStack* cs) {

    // make_local_buffer_staged(rb, sizeof(rcs_verts), rcs_verts,
    // VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, cpool, vbuf, cs);
    // make_local_buffer_staged(rb, sizeof(rcs_indices), rcs_indices,
    // VK_BUFFER_USAGE_INDEX_BUFFER_BIT, cpool, ibuf, cs);

    // NEEDS TO BE BINARY STL FILE, CAN'T USE RAW EXPORT FROM OPENVSP
    FILE* fp = fopen("rcs2mesh.stl", "r");

    char comment[80];
    float* vertdata;
    triangle_t *triangles, n_triangles;
    vertex_t n_verts;
    u16* triangle_attrs;

    loadstl(fp, comment, &vertdata, &n_verts, &triangles, &triangle_attrs,
            &n_triangles);

    comment[79] = 0;

    printf("comment string: %s\n", comment);

    fclose(fp);
    printf("READ STL FILE: %i verts and %i triangles\n", n_verts, n_triangles);

    RcsVertex* processed_verts = malloc(sizeof(RcsVertex) * n_verts);

    preprocess_verts(vertdata, n_verts, processed_verts);

    make_local_buffer_staged(rb, n_verts * sizeof(RcsVertex), processed_verts,
                             VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, cpool, vbuf,
                             cs);
    make_local_buffer_staged(rb, sizeof(u32) * 3 * n_triangles, triangles,
                             VK_BUFFER_USAGE_INDEX_BUFFER_BIT, cpool, ibuf, cs);

    vmaSetAllocationName(rb->alloc, vbuf->alloc, "RCS vertex buffer");
    vmaSetAllocationName(rb->alloc, ibuf->alloc, "RCS index buffer");

    *n_indices = n_triangles * 3;

    free(processed_verts);

    return false;
}
