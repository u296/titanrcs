
#include "rcs/rcs_ubo.h"
#include "backend/backend.h"
#include "buffers.h"
#include "cleanupdb.h"
#include "cleanupstack.h"
#include "common.h"
#include "res.h"

bool make_rcs_ubo(RenderBackend* rb, Buffer* ubo, CleanupStack* cs) {

    make_buffer(rb, sizeof(RcsUbo), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                TR_MAPPABLE_WRITE, ubo, cs);
    vmaSetAllocationName(rb->alloc, ubo->alloc, "RCS UBO");

    return false;
}

bool make_rcs_fftbuf(RenderBackend* rb, Buffer* rcs_fftbuf, CleanupStack* cs) {

    make_buffer(rb, RCS_RESOLUTION * RCS_RESOLUTION * 2 * sizeof(float),
                VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                TR_MAPPABLE_NONE, rcs_fftbuf, cs);

    return false;
}

bool make_rcs_fftimg(RenderBackend* rb, VkExtent2D ext, Image* rcs_fftimg,
                     CleanupStack* cs) {

    VkImageCreateInfo ici = {};
    ici.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ici.imageType = VK_IMAGE_TYPE_2D;
    ici.extent = (VkExtent3D){ext.width, ext.height, 1};
    ici.format = VK_FORMAT_R32G32_SFLOAT;
    ici.arrayLayers = 1;
    ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ici.samples = VK_SAMPLE_COUNT_1_BIT;
    ici.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ici.tiling = VK_IMAGE_TILING_OPTIMAL;
    ici.mipLevels = 1;
    ici.queueFamilyIndexCount = 1;
    ici.pQueueFamilyIndices = &rb->queues.i_graphics_queue_fam;
    ici.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    VmaAllocationCreateInfo aci = {};
    aci.usage = VMA_MEMORY_USAGE_AUTO;

    VkResult r = vmaCreateImage(rb->alloc, &ici, &aci, &rcs_fftimg->img,
                                &rcs_fftimg->alloc, NULL);

    VkImageViewCreateInfo ivci = {};
    ivci.image = rcs_fftimg->img;
    ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    ivci.format = VK_FORMAT_R32G32_SFLOAT;
    ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    ivci.subresourceRange.baseMipLevel = 0;
    ivci.subresourceRange.levelCount = 1;
    ivci.subresourceRange.baseArrayLayer = 0;
    ivci.subresourceRange.layerCount = 1;

    r = vkCreateImageView(rb->dev, &ivci, NULL, &rcs_fftimg->view);

    CLEANUP_START(ImageCleanup){*rcs_fftimg, rb->dev,
                                rb->alloc} CLEANUP_END(image)

        return false;
}

bool make_rcs_extrbuf(RenderBackend* rb, Buffer* rcs_extrbuf,
                      CleanupStack* cs) {

    make_buffer(rb, sizeof(ExtractionSsbo),
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                TR_MAPPABLE_READ, rcs_extrbuf, cs);

    return false;
}