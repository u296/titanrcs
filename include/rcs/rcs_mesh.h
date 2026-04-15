#ifndef RCS_MESH_H
#define RCS_MESH_H

#include "backend/backend.h"

bool make_rcs_mesh(RenderBackend* rb, VkCommandPool cpool, Buffer* vbuf,
                   Buffer* ibuf, u32* n_indices, Buffer* sharp_ibuf, u32* n_sharp_indices, CleanupStack* cs);

#endif