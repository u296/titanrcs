#include "rcs/rcs_imgs.h"

#include "backend/backend.h"
#include "cleanupstack.h"
#include "common.h"
#include <vulkan/vulkan_core.h>

typedef struct ImageCleanup {
    Image img;
    VkDevice dev;
    VmaAllocator allocctx;
} ImageCleanup;

void destroy_image(void* obj) {
    ImageCleanup* ic = (ImageCleanup*)obj;
    if (ic->img.view != VK_NULL_HANDLE) {
        vkDestroyImageView(ic->dev, ic->img.view, NULL);
    }
    vmaDestroyImage(ic->allocctx, ic->img.img, ic->img.alloc);
}

bool make_rcs_depthresources(RenderBackend* rb, VkExtent2D ext, Image* depthimg,
                             CleanupStack* cs) {
    constexpr VkFormat format = VK_FORMAT_D32_SFLOAT;

    VkImageCreateInfo ici = {};
    ici.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ici.imageType = VK_IMAGE_TYPE_2D;
    ici.extent = (VkExtent3D){ext.width, ext.height, 1};
    ici.mipLevels = 1;
    ici.arrayLayers = 1;
    ici.format = format;
    ici.tiling = VK_IMAGE_TILING_OPTIMAL;
    ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ici.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    ici.samples = VK_SAMPLE_COUNT_1_BIT;
    ici.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkImageViewCreateInfo ivci = {};
    ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    ivci.format = format;
    ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    ivci.subresourceRange.baseMipLevel = 0;
    ivci.subresourceRange.levelCount = 1;
    ivci.subresourceRange.baseArrayLayer = 0;
    ivci.subresourceRange.layerCount = 1;

    VmaAllocationCreateInfo aci = {};
    aci.usage = VMA_MEMORY_USAGE_AUTO;

    vmaCreateImage(rb->alloc, &ici, &aci, &depthimg->img, &depthimg->alloc,
                   NULL);
    ivci.image = depthimg->img;

    vkCreateImageView(rb->dev, &ivci, NULL, &depthimg->view);

    CLEANUP_START_NORES(ImageCleanup){*depthimg, rb->dev,
                                      rb->alloc} CLEANUP_END(image)

        return false;
}

bool make_rcs_rendertargets(RenderBackend* rb, VkExtent2D ext,
                            const u32 n_targets, Image* rendtargets,
                            CleanupStack* cs) {

    VkFormat mainformat = VK_FORMAT_R8G8B8A8_SRGB;

    

    VkImageCreateInfo ici = {};
    ici.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ici.imageType = VK_IMAGE_TYPE_2D;
    ici.extent = (VkExtent3D){ext.width, ext.height, 1};
    ici.mipLevels = 1;
    ici.arrayLayers = 1;
    ici.format = mainformat;
    ici.tiling = VK_IMAGE_TILING_OPTIMAL;
    ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ici.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    ici.samples = VK_SAMPLE_COUNT_1_BIT;
    ici.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkImageViewCreateInfo ivci = {};
    ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    ivci.format = mainformat;
    ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    ivci.subresourceRange.baseMipLevel = 0;
    ivci.subresourceRange.levelCount = 1;
    ivci.subresourceRange.baseArrayLayer = 0;
    ivci.subresourceRange.layerCount = 1;

    VmaAllocationCreateInfo aci = {};
    aci.usage = VMA_MEMORY_USAGE_AUTO;

    for (u32 i = 0; i < n_targets; i++) {
        if (i == 0) {
            ici.format = VK_FORMAT_R32G32_SFLOAT;
            ivci.format = ici.format;
        } else {
            ici.format = mainformat;
            ivci.format = mainformat;
        }

        vmaCreateImage(rb->alloc, &ici, &aci, &rendtargets[i].img,
                       &rendtargets[i].alloc, NULL);

        ivci.image = rendtargets[i].img;

        vkCreateImageView(rb->dev, &ivci, NULL, &rendtargets[i].view);

        CLEANUP_START_NORES(ImageCleanup){rendtargets[i], rb->dev,
                                          rb->alloc} CLEANUP_END(image)
    }

    return false;
}

typedef struct SamplerCleanup {
    VkDevice dev;
    VkSampler samp;
} SamplerCleanup;

void destroy_sampler(void* obj) {
    SamplerCleanup* sc = (SamplerCleanup*)obj;
    vkDestroySampler(sc->dev, sc->samp, NULL);
}

bool make_sampler(RenderBackend* rb, VkSampler* out_sampler, CleanupStack* cs) {

    VkSamplerCreateInfo sci = {};
    sci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sci.magFilter = VK_FILTER_LINEAR;
    sci.minFilter = VK_FILTER_NEAREST;
    sci.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sci.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sci.anisotropyEnable = VK_FALSE;
    sci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;

    sci.unnormalizedCoordinates = VK_FALSE;
    sci.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

    VkResult r = vkCreateSampler(rb->dev, &sci, NULL, out_sampler);
    CLEANUP_START(SamplerCleanup)
    {rb->dev, *out_sampler}
    CLEANUP_END(sampler);


    return false;
}