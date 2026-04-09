#include <assert.h>
#include <volk.h>
#include "backend/fft.h"
#include "backend/backend.h"
#include <vkFFT/vkFFT_AppManagement/vkFFT_DeleteApp.h>
#include <vkFFT/vkFFT_AppManagement/vkFFT_InitializeApp.h>
#include <vkFFT/vkFFT_Structs/vkFFT_Structs.h>
#include "cleanupstack.h"
#include "res.h"


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
u64 fftbuf_size = 2 * sizeof(float) * RCS_RESOLUTION * RCS_RESOLUTION; // x2 for complex

void fft_populate_persistents(VkDevice dev, VkPhysicalDevice physdev, Queues queues) {
    persist_dev = dev;
    persist_physdev = physdev;
    persist_queue = queues.graphics_queue;
    persist_queue_i = queues.i_graphics_queue_fam;
}

bool make_fftapp(VkInstance inst, VkFFTApplication** out_fftapp, CleanupStack* cs ) {

    

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
    cfg.size[0] = RCS_RESOLUTION;
    cfg.size[1] = RCS_RESOLUTION;
    cfg.size[2] = 1;
    cfg.isInputFormatted = 0;
    cfg.isOutputFormatted = 0;

    constexpr u32 VIEWSIZE = 1024;

    cfg.frequencyZeroPadding = 1;
    cfg.performZeropadding[0] = 1;
    cfg.performZeropadding[1] = 1;
    cfg.fft_zeropad_left[0] = 512;
    cfg.fft_zeropad_right[0] = 8192-512;
    cfg.fft_zeropad_left[1] = 512;
    cfg.fft_zeropad_right[1] = 8192-512;
    cfg.keepShaderCode = 1;


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