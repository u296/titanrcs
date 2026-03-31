
#include "rcs/rcs_ubo.h"
#include "backend/backend.h"
#include "buffers.h"
#include "cleanupstack.h"
#include "common.h"
#include <vulkan/vulkan_core.h>


bool make_rcs_ubo(RenderBackend* rb, Buffer* ubo,CleanupStack* cs) {



    make_buffer(rb, sizeof(RcsUbo), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, true, ubo, cs);
    vmaSetAllocationName(rb->alloc, ubo->alloc, "RCS UBO");

    return false;
}