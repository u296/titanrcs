
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

bool make_rcs_fftbufs(RenderBackend* rb, Buffer* out_inputbuf,
                      CleanupStack* cs) {

    make_buffer(rb,
                RCS_RESOLUTION * RCS_RESOLUTION * 2 * 2 *
                    sizeof(float), // 4 for 2 complex numbers
                VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                TR_MAPPABLE_NONE, out_inputbuf, cs);

    return false;
}

bool make_rcs_finalimg_fft(RenderBackend* rb, VkExtent2D ext,
                           Image* out_finalimg, CleanupStack* cs) {

    // there's no need to store the uncropped parts of the fft image, we'll just
    // make it smaller here

    VkImageCreateInfo ici = {};
    ici.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ici.imageType = VK_IMAGE_TYPE_2D;
    ici.extent = (VkExtent3D){ext.width / RCS_CROPFRACTION,
                              ext.height / RCS_CROPFRACTION, 1};
    ici.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    ici.arrayLayers = 1;
    ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ici.samples = VK_SAMPLE_COUNT_1_BIT;
    ici.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ici.tiling = VK_IMAGE_TILING_OPTIMAL;
    ici.mipLevels = 1;
    ici.queueFamilyIndexCount = 1;
    ici.pQueueFamilyIndices = &rb->queues.i_graphics_queue_fam;
    ici.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
                VK_IMAGE_USAGE_STORAGE_BIT;

    VmaAllocationCreateInfo aci = {};
    aci.usage = VMA_MEMORY_USAGE_AUTO;

    VkResult r = vmaCreateImage(rb->alloc, &ici, &aci, &out_finalimg->img,
                                &out_finalimg->alloc, NULL);

    VkImageViewCreateInfo ivci = {};
    ivci.image = out_finalimg->img;
    ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    ivci.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    ivci.subresourceRange.baseMipLevel = 0;
    ivci.subresourceRange.levelCount = 1;
    ivci.subresourceRange.baseArrayLayer = 0;
    ivci.subresourceRange.layerCount = 1;

    r = vkCreateImageView(rb->dev, &ivci, NULL, &out_finalimg->view);

    CLEANUP_START(ImageCleanup){*out_finalimg, rb->dev,
                                rb->alloc} CLEANUP_END(image)

        return false;
}

bool make_rcs_finalimg_sum(RenderBackend* rb, VkExtent2D ext,
                           Image* out_finalimg, CleanupStack* cs) {

    /*
    adapt the rest of the code to this. 32x32 is 1024 which is the typical limit
    for local workgroup size, so we make the final reduction a 32x32 reduction
    which we then fit the rest of the setup to downscale to.
    */
    const u32 size = 32;

    VkImageCreateInfo ici = {};
    ici.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ici.imageType = VK_IMAGE_TYPE_2D;
    ici.extent = (VkExtent3D){size, size, 1};
    ici.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    ici.arrayLayers = 1;
    ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ici.samples = VK_SAMPLE_COUNT_1_BIT;
    ici.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ici.tiling = VK_IMAGE_TILING_OPTIMAL;
    ici.mipLevels = 1;
    ici.queueFamilyIndexCount = 1;
    ici.pQueueFamilyIndices = &rb->queues.i_graphics_queue_fam;
    ici.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
                VK_IMAGE_USAGE_STORAGE_BIT;

    VmaAllocationCreateInfo aci = {};
    aci.usage = VMA_MEMORY_USAGE_AUTO;

    VkResult r = vmaCreateImage(rb->alloc, &ici, &aci, &out_finalimg->img,
                                &out_finalimg->alloc, NULL);

    VkImageViewCreateInfo ivci = {};
    ivci.image = out_finalimg->img;
    ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    ivci.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    ivci.subresourceRange.baseMipLevel = 0;
    ivci.subresourceRange.levelCount = 1;
    ivci.subresourceRange.baseArrayLayer = 0;
    ivci.subresourceRange.layerCount = 1;

    r = vkCreateImageView(rb->dev, &ivci, NULL, &out_finalimg->view);

    CLEANUP_START(ImageCleanup){*out_finalimg, rb->dev,
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

#ifdef TR_CALCMODE_SUM
bool make_rcs_intermediate_sum_img(RenderBackend* rb, VkExtent2D ext,
                                   Image* out_intermediate_sum_img,
                                   CleanupStack* cs) {

    // there's no need to store the uncropped parts of the fft image, we'll just
    // make it smaller here

    const u32 size = RCS_RESOLUTION / TR_FIRST_DOWNSCALESIZE;

    VkImageCreateInfo ici = {};
    ici.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ici.imageType = VK_IMAGE_TYPE_2D;
    ici.extent = (VkExtent3D){size, size, 1};
    ici.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    ici.arrayLayers = 1;
    ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ici.samples = VK_SAMPLE_COUNT_1_BIT;
    ici.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ici.tiling = VK_IMAGE_TILING_OPTIMAL;
    ici.mipLevels = 1;
    ici.queueFamilyIndexCount = 1;
    ici.pQueueFamilyIndices = &rb->queues.i_graphics_queue_fam;
    ici.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    VmaAllocationCreateInfo aci = {};
    aci.usage = VMA_MEMORY_USAGE_AUTO;

    VkResult r =
        vmaCreateImage(rb->alloc, &ici, &aci, &out_intermediate_sum_img->img,
                       &out_intermediate_sum_img->alloc, NULL);

    VkImageViewCreateInfo ivci = {};
    ivci.image = out_intermediate_sum_img->img;
    ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    ivci.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    ivci.subresourceRange.baseMipLevel = 0;
    ivci.subresourceRange.levelCount = 1;
    ivci.subresourceRange.baseArrayLayer = 0;
    ivci.subresourceRange.layerCount = 1;

    r = vkCreateImageView(rb->dev, &ivci, NULL,
                          &out_intermediate_sum_img->view);

    CLEANUP_START(ImageCleanup){*out_intermediate_sum_img, rb->dev,
                                rb->alloc} CLEANUP_END(image);

    return false;
}
#endif