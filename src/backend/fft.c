#include <assert.h>
#include <volk.h>
#include "backend/fft.h"
#include "backend/backend.h"
#include <vkFFT/vkFFT_AppManagement/vkFFT_DeleteApp.h>
#include <vkFFT/vkFFT_AppManagement/vkFFT_InitializeApp.h>
#include <vkFFT/vkFFT_Structs/vkFFT_Structs.h>
#include "cleanupstack.h"

typedef struct FFTCleanup {
    VkFFTApplication* app;
} FFTCleanup;

void destroy_fftapp(void* obj) {
    FFTCleanup* f = (FFTCleanup*)obj;

    deleteVkFFT(f->app);
}

// this stupid library requires persistent pointers to creation resources that are already pointers
// I'm thinking about just pointing to the backend but that makes it impossible to ever move the backend
// which sucks because the whole point of the backend was to be a movable package

static VkDevice persist_dev;
static VkPhysicalDevice persist_physdev;
static VkQueue persist_queue;
static u32 persist_queue_i;
u64 fftbuf_size = 2 * sizeof(float) * 256 * 256; // x2 for complex


bool make_fftapp(VkInstance inst, VkPhysicalDevice physdev, VkDevice dev, Queues queues, VkFFTApplication** out_fftapp, CleanupStack* cs ) {

    persist_dev = dev;
    persist_physdev = physdev;
    persist_queue = queues.graphics_queue;
    persist_queue_i = queues.i_graphics_queue_fam;

    VkFFTConfiguration cfg = {0};

    VkCommandPool fft_cmdpool = VK_NULL_HANDLE;
    VkFence fft_fen = VK_NULL_HANDLE;


    VkCommandPoolCreateInfo pci = {};
    pci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pci.queueFamilyIndex = persist_queue_i;

    vkCreateCommandPool(persist_dev, &pci, NULL, &fft_cmdpool);

    VkFenceCreateInfo fci = {};
    fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    vkCreateFence(persist_dev, &fci, NULL, &fft_fen);

    VkFFTApplication* fftapp = malloc(sizeof(VkFFTApplication));
    
    CLEANUP_START_NORES(void*)
    fftapp
    CLEANUP_END(memfree)
    
    *fftapp = (VkFFTApplication){0};

    cfg.physicalDevice = &persist_physdev;
    cfg.device = &persist_dev;
    cfg.queue = &persist_queue;
    cfg.commandPool = &fft_cmdpool;
    cfg.fence = &fft_fen;


    cfg.FFTdim = 2;
    cfg.size[0] = 256;
    cfg.size[1] = 256;
    cfg.size[2] = 1;
    cfg.isInputFormatted = 0;

    cfg.bufferSize = &fftbuf_size;


    VkFFTResult res = initializeVkFFT(fftapp, cfg);

    assert(res == VKFFT_SUCCESS);

    CLEANUP_START_NORES(FFTCleanup)
    {
        fftapp
    }
    CLEANUP_END(fftapp)

    *out_fftapp = fftapp;

    vkDestroyFence(persist_dev, fft_fen, NULL);
    vkDestroyCommandPool(persist_dev, fft_cmdpool, NULL);

    return false;

}