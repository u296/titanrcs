#ifndef RCS_MESH_H
#define RCS_MESH_H

#include "backend/backend.h"

bool make_rcs_mesh(RenderBackend* rb, VkCommandPool cpool, Buffer* vbuf, Buffer* ibuf, CleanupStack* cs);

#endif