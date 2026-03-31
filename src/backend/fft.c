#include <assert.h>
#include <volk.h>
#include "backend/fft.h"
#include "backend/backend.h"
#include "vkFFT/vkFFT_AppManagement/vkFFT_DeleteApp.h"
#include "vkFFT/vkFFT_AppManagement/vkFFT_InitializeApp.h"
#include "vkFFT/vkFFT_Structs/vkFFT_Structs.h"
#include "vulkan/vulkan_core.h"
#include <string.h>
#include "cleanupstack.h"

typedef struct FFTCleanup {
    VkFFTApplication* app;
} FFTCleanup;

void destroy_fftapp(void* obj) {
    FFTCleanup* f = (FFTCleanup*)obj;

    deleteVkFFT(f->app);
}

bool make_fftapp(VkInstance inst, VkPhysicalDevice physdev, VkDevice dev, Queues queues, VkFFTApplication** out_fftapp, CleanupStack* cs ) {
    VkFFTConfiguration cfg = {0};

    VkCommandPool tmppool = VK_NULL_HANDLE;

    VkQueue fftqueue = queues.graphics_queue;
    u32 fftqueue_i = queues.i_graphics_queue_fam;

    VkCommandPoolCreateInfo pci = {};
    pci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pci.queueFamilyIndex = fftqueue_i;

    vkCreateCommandPool(dev, &pci, NULL, &tmppool);

    VkFence tmpfen = VK_NULL_HANDLE;
    VkFenceCreateInfo fci = {};
    fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    vkCreateFence(dev, &fci, NULL, &tmpfen);

    cfg.physicalDevice = &physdev;
    cfg.device = &dev;
    cfg.queue = &fftqueue;
    cfg.commandPool = &tmppool;
    cfg.fence = &tmpfen;


    cfg.FFTdim = 2;
    cfg.size[0] = 256;
    cfg.size[1] = 256;
    cfg.size[2] = 1;
    cfg.isInputFormatted = 1;


    VkFFTApplication* fftapp = malloc(sizeof(VkFFTApplication));
    CLEANUP_START_NORES(void*)
    fftapp
    CLEANUP_END(memfree)

    VkFFTResult res = initializeVkFFT(fftapp, cfg);

    assert(res == VKFFT_SUCCESS);

    CLEANUP_START_NORES(FFTCleanup)
    {
        fftapp
    }
    CLEANUP_END(fftapp)

    *out_fftapp = fftapp;

    vkDestroyFence(dev, tmpfen, NULL);
    vkDestroyCommandPool(dev, tmppool, NULL);

    return false;

}