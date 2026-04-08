#include "rcs/rcs_mesh.h"
#include "backend/backend.h"
#include "buffers.h"
#include "common.h"
#include "linalg.h"
#include "rcs/rcs_pipeline.h"
#include <assert.h>
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

void build_normals(u32 n_verts, float* raw_verts, u32 n_tris, u32* triangles,
                   Vec3* out_normals) {
    // make one normal for each vertex.
    // loop through all of the triangles and add the triangles normal to all of
    // its verts. Then at the end, normalize all of the normals so

                    // start by zeroing all normals

    for (u32 i = 0; i < n_verts; i++) {
        out_normals[i] = (Vec3) {0.0f, 0.0f, 0.0f};
    }

    for (u32 tri = 0; tri < n_tris; tri++) {
        Vec3 this_tri[3];

        u32 this_inds[3] = {
            triangles[3 * tri + 0],
            triangles[3 * tri + 1],
            triangles[3 * tri + 2]
        };

        for (u8 i = 0; i < 3; i++) {
        this_tri[i] = (Vec3) {
            raw_verts[3*this_inds[i]],
                raw_verts[3*this_inds[i]+1],
                raw_verts[3*this_inds[i]+2]
        };
        }

        Vec3 a = sub_v3(this_tri[1], this_tri[0]);
        Vec3 b = sub_v3(this_tri[2], this_tri[0]);

        Vec3 norm = cross_v3(a, b);

        out_normals[this_inds[0]] = add_v3(out_normals[this_inds[0]], norm);
        out_normals[this_inds[1]] = add_v3(out_normals[this_inds[1]], norm);
        out_normals[this_inds[2]] = add_v3(out_normals[this_inds[2]], norm);
    }

    for (u32 i = 0; i < n_verts; i++) { // normalize verts
        out_normals[i] = muls_v3(1.0f/len_v3(out_normals[i]), out_normals[i]);
    }

}

void process_verts(float* raw_verts, u32 n_verts, Vec3* normals, RcsVertex* new_buf) {

    const float hardcode_scaling = 1.0;

    for (u32 i = 0; i < n_verts; i++) {

        // for now, ignore the normals

        new_buf[i] = (RcsVertex){{raw_verts[i * 3] * hardcode_scaling,
                                  raw_verts[i * 3 + 1] * hardcode_scaling,
                                  raw_verts[i * 3 + 2] * hardcode_scaling},
                                 normals[i]};
    }
}

bool make_rcs_mesh(RenderBackend* rb, VkCommandPool cpool, Buffer* vbuf,
                   Buffer* ibuf, u32* n_indices, CleanupStack* cs) {

    // make_local_buffer_staged(rb, sizeof(rcs_verts), rcs_verts,
    // VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, cpool, vbuf, cs);
    // make_local_buffer_staged(rb, sizeof(rcs_indices), rcs_indices,
    // VK_BUFFER_USAGE_INDEX_BUFFER_BIT, cpool, ibuf, cs);

    // NEEDS TO BE BINARY STL FILE, CAN'T USE RAW EXPORT FROM OPENVSP
    FILE* fp = fopen("models/rcsmesh.stl", "r");

    assert(fp != NULL);

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
    Vec3* normals = malloc(sizeof(Vec3) * n_verts);

    build_normals(n_verts, vertdata, n_triangles, triangles, normals);

    process_verts(vertdata, n_verts, normals, processed_verts);

    make_local_buffer_staged(rb, n_verts * sizeof(RcsVertex), processed_verts,
                             VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, cpool, vbuf,
                             cs);
    make_local_buffer_staged(rb, sizeof(u32) * 3 * n_triangles, triangles,
                             VK_BUFFER_USAGE_INDEX_BUFFER_BIT, cpool, ibuf, cs);

    vmaSetAllocationName(rb->alloc, vbuf->alloc, "RCS vertex buffer");
    vmaSetAllocationName(rb->alloc, ibuf->alloc, "RCS index buffer");

    *n_indices = n_triangles * 3;

    free(processed_verts);
    free(normals);

    return false;
}
