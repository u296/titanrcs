#include "rcs/rcs_mesh.h"
#include "backend/backend.h"
#include "buffers.h"
#include "common.h"
#include "linalg.h"
#include "rcs/rcs_pipeline.h"
#include <assert.h>
#include <math.h>
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

void weld_vertices(u32 n_verts, f32* raw_verts, u32 n_tris, u32* triangles) {

    const f32 len_threshold = 0.000001f;
    u32 n_welded_verts = 0;

    for (u32 i = 0; i < n_verts - 1; i++) {
        for (u32 j = i + 1; j < n_verts; j++) {

            Vec3 diff = {raw_verts[i * 3 + 0] - raw_verts[j * 3 + 0],
                         raw_verts[i * 3 + 1] - raw_verts[j * 3 + 1],
                         raw_verts[i * 3 + 2] - raw_verts[j * 3 + 2]};

            f32 diff_l = len_v3(diff);

            if (diff_l < len_threshold) {
                for (u32 k = 0; k < n_tris * 3; k++) {
                    if (triangles[k] == j) {
                        triangles[k] = i;
                        n_welded_verts++;
                    }
                }
            }
        }
    }
    printf("welded %u vertices \n", n_welded_verts);
}

void build_vertex_normals(u32 n_verts, float* raw_verts, u32 n_tris,
                          u32* triangles, Vec3* out_vert_normals,
                          Vec3* out_triangle_normals) {
    // make one normal for each vertex.
    // loop through all of the triangles and add the triangles normal to all of
    // its verts. Then at the end, normalize all of the normals so

    // start by zeroing all normals

    for (u32 i = 0; i < n_verts; i++) {
        out_vert_normals[i] = (Vec3){0.0f, 0.0f, 0.0f};
    }

    for (u32 tri = 0; tri < n_tris; tri++) {
        Vec3 this_tri[3];

        u32 this_inds[3] = {triangles[3 * tri + 0], triangles[3 * tri + 1],
                            triangles[3 * tri + 2]};

        for (u8 i = 0; i < 3; i++) {
            this_tri[i] = (Vec3){raw_verts[3 * this_inds[i]],
                                 raw_verts[3 * this_inds[i] + 1],
                                 raw_verts[3 * this_inds[i] + 2]};
        }

        Vec3 a = sub_v3(this_tri[1], this_tri[0]);
        Vec3 b = sub_v3(this_tri[2], this_tri[0]);

        a = muls_v3(1.0f / len_v3(a), a);
        b = muls_v3(1.0f / len_v3(b), b);

        Vec3 norm = cross_v3(a, b);

        out_vert_normals[this_inds[0]] =
            add_v3(out_vert_normals[this_inds[0]], norm);
        out_vert_normals[this_inds[1]] =
            add_v3(out_vert_normals[this_inds[1]], norm);
        out_vert_normals[this_inds[2]] =
            add_v3(out_vert_normals[this_inds[2]], norm);

        out_triangle_normals[tri] = normalize_v3(norm);
    }

    for (u32 i = 0; i < n_verts; i++) { // normalize verts
        Vec3 norm = out_vert_normals[i];

        norm = normalize_v3(norm);

        out_vert_normals[i] = norm;
        f32 normlen = len_v3(out_vert_normals[i]);
        if (isnan(out_vert_normals[i].x) || isnan(out_vert_normals[i].y) ||
            isnan(out_vert_normals[i].z)) {
            printf("MADE NAN VERTEX NORMAL\n");
        } else if (normlen < 0.01) {
            printf("MADE SHORT VERTEX NORMAL\n");
        }
    }
}

void sort_indpair(u32* a, u32* b) {
    if (!(*a < *b)) {
        u32 tmp = *a;
        *a = *b;
        *b = tmp;
    }
}

typedef struct EdgeRecord {
    u32 v1, v2, tri_i;
} EdgeRecord;

i32 edge_order_compare(const void* a, const void* b) {
    const EdgeRecord* e1 = a;
    const EdgeRecord* e2 = b;

    if (e1->v1 != e2->v1) {
        return (i32)e1->v1 - (i32)e2->v1;
    } else {
        return (i32)e1->v2 - (i32)e2->v2;
    }
}

u32 build_sharp_edges(const u32 n_tris, const u32* triangles,
                      const f32* raw_verts, const Vec3* triangle_normals,
                      u32** out_inds) {

    constexpr f32 SHARP_ANGLE = 30.0f*DEG_TO_RAD;

    EdgeRecord* edge_records = malloc(sizeof(EdgeRecord) * n_tris * 3);
    memset(edge_records, 0, sizeof(EdgeRecord) * n_tris * 3);
    u32 i_edge_insert = 0;

    for (u32 tri = 0; tri < n_tris; tri++) {

        for (u32 i = 0; i < 3; i++) {
            u32 v1 = triangles[3 * tri + (i)];
            u32 v2 = triangles[3 * tri + (i + 1) % 3];
            sort_indpair(&v1, &v2);

            if (i_edge_insert >= n_tris * 3) {
                printf("mesh load: TOO MANY EDGES\n");
                abort();
            }
            edge_records[i_edge_insert] = (EdgeRecord){v1, v2, tri};
            i_edge_insert++;
        }
    }

    qsort(edge_records, i_edge_insert, sizeof(EdgeRecord), edge_order_compare);

    u32* built_inds = malloc(2 * 3 * n_tris * sizeof(u32));
    memset(built_inds, 0, 2 * 3 * n_tris * sizeof(u32));
    u32 i_inds_insert = 0;

    for (u32 i = 0; i < i_edge_insert - 1; i++) {
        if (edge_records[i].v1 == edge_records[i + 1].v1 &&
            edge_records[i].v2 == edge_records[i + 1].v2) {
            Vec3 norm1 = triangle_normals[edge_records[i].tri_i];
            Vec3 norm2 = triangle_normals[edge_records[i + 1].tri_i];

            f32 d = dot_v3(norm1, norm2);

            if (!(d < 1.1f && d > -1.1f)) {
                printf("weird length\n");
            }

            if (d < cosf(SHARP_ANGLE)) {
                if (i_inds_insert >= 2 * 3 * n_tris) {
                    printf("mesh load: out of space for output inds");
                    abort();
                }
                built_inds[i_inds_insert] = edge_records[i].v1;
                built_inds[i_inds_insert + 1] = edge_records[i].v2;
                i_inds_insert += 2;
            }
        }
    }
    printf("built %u sharp edges out of maximum %u (%.1f%%)\n",
           i_inds_insert / 2, 3 * n_tris,
           100.0f * (f32)i_inds_insert / (f32)(6 * n_tris));

    *out_inds = built_inds;
    return i_inds_insert;
}

void process_verts(float* raw_verts, u32 n_verts, Vec3* normals,
                   RcsVertex* new_buf) {

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
                   Buffer* ibuf, u32* n_indices, Buffer* sharp_ibuf,
                   u32* n_sharp_indices, CleanupStack* cs) {

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

    // weld_vertices(n_verts, vertdata, n_triangles, triangles);

    RcsVertex* processed_verts = malloc(sizeof(RcsVertex) * n_verts);
    Vec3* vertex_normals = malloc(sizeof(Vec3) * n_verts);
    Vec3* triangle_normals = malloc(sizeof(Vec3) * n_triangles);
    u32* sharp_inds;

    build_vertex_normals(n_verts, vertdata, n_triangles, triangles,
                         vertex_normals, triangle_normals);

    u32 n_sharp_inds = build_sharp_edges(n_triangles, triangles, vertdata,
                                         triangle_normals, &sharp_inds);

    process_verts(vertdata, n_verts, vertex_normals, processed_verts);

    make_local_buffer_staged(rb, n_verts * sizeof(RcsVertex), processed_verts,
                             VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, cpool, vbuf,
                             cs);
    make_local_buffer_staged(rb, sizeof(u32) * 3 * n_triangles, triangles,
                             VK_BUFFER_USAGE_INDEX_BUFFER_BIT, cpool, ibuf, cs);

    make_local_buffer_staged(rb, sizeof(u32) * n_sharp_inds, sharp_inds,
                             VK_BUFFER_USAGE_INDEX_BUFFER_BIT, cpool,
                             sharp_ibuf, cs);

    vmaSetAllocationName(rb->alloc, vbuf->alloc, "RCS vertex buffer");
    vmaSetAllocationName(rb->alloc, ibuf->alloc, "RCS index buffer");
    vmaSetAllocationName(rb->alloc, sharp_ibuf->alloc,
                         "RCS sharp index buffer");

    *n_indices = n_triangles * 3;
    *n_sharp_indices = n_sharp_inds;

    free(processed_verts);
    free(vertex_normals);
    free(triangle_normals);
    free(sharp_inds);

    return false;
}
