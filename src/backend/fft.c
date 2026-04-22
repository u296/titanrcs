#include "backend/backend.h"
#include "cleanupstack.h"
#include "res.h"
#include <assert.h>
#include <vkFFT/vkFFT_AppManagement/vkFFT_DeleteApp.h>
#include <vkFFT/vkFFT_AppManagement/vkFFT_InitializeApp.h>
#include <vkFFT/vkFFT_Structs/vkFFT_Structs.h>
#include <volk.h>

typedef struct FFTCleanup {
    VkFFTApplication* app;
} FFTCleanup;

void destroy_fftapp(void* obj) {
    FFTCleanup* f = (FFTCleanup*)obj;

    deleteVkFFT(f->app);
}

// this stupid library requires persistent pointers to creation resources that
// are already pointers I'm thinking about just pointing to the backend but that
// makes it impossible to ever move the backend which sucks because the whole
// point of the backend was to be a movable package

#define ZEROPAD_FFTOUTPUT

static VkDevice persist_dev;
static VkPhysicalDevice persist_physdev;
static VkQueue persist_queue;
static u32 persist_queue_i;
u64 fftbuf_size =
    4ull * sizeof(float) * RCS_RESOLUTION * RCS_RESOLUTION; // x2 for complex x2 for x and y channel

void fft_populate_persistents(VkDevice dev, VkPhysicalDevice physdev,
                              Queues queues) {
    persist_dev = dev;
    persist_physdev = physdev;
    persist_queue = queues.graphics_queue;
    persist_queue_i = queues.i_graphics_queue_fam;
}

bool make_fftapp(VkInstance inst, VkFFTApplication** out_fftapp,
                 CleanupStack* cs) {

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
    fftapp CLEANUP_END(memfree)

        * fftapp = (VkFFTApplication) {
            0
        };

    cfg.physicalDevice = &persist_physdev;
    cfg.device = &persist_dev;
    cfg.queue = &persist_queue;
    cfg.commandPool = &fft_cmdpool;
    cfg.fence = &fft_fen;

    // constexpr u64 full = RCS_RESOLUTION;
    // constexpr u64 pruned = 5120;

    // constexpr u64 totpad = (RCS_RESOLUTION-viewsize);

    cfg.FFTdim = 2;
    cfg.size[0] = RCS_RESOLUTION;
    cfg.size[1] = RCS_RESOLUTION;

    cfg.numberBatches = 2; // x and y channels

    cfg.bufferSize = &fftbuf_size;
    cfg.inputBufferSize = &fftbuf_size;
    cfg.outputBufferSize = &fftbuf_size;

    //cfg.maxTempLength = fftbuf_size;
    // cfg.userTempBuffer = 1;
    // cfg.tempBufferSize = &fftbuf_size;

    /*
    I believe that setting these all to 0 with input and output formatting is
    the only correct way to go. Setting [0] to 2 seems to give the same result,
    which indicates that it's automatically calculated to 2. Setting the other
    two strides manually doesn't seem to work, at least I haven't been able to
    find a number that doesn't yield a completely black image. I think it's safe
    to assume that leaving it like this is fine and yields correct results,
    since the channels aren't cross-contaminated and the output image looks
    correct.
    */

    cfg.isInputFormatted = 0;
    cfg.inputBufferStride[0] = 0;
    cfg.inputBufferStride[1] = 0;
    cfg.inputBufferStride[2] = 0;

    cfg.isOutputFormatted = 0;

    cfg.outputBufferStride[0] = 0;
    cfg.outputBufferStride[1] = 0;
    cfg.outputBufferStride[2] = 0;

#ifdef ZEROPAD_FFTOUTPUT

    constexpr u64 l = RCS_RESOLUTION / RCS_CROPFRACTION;

    /*
    getting a headache trying to wrap my head around this, it doesn't really
    like this whole thing, especially not padding out the just the middle and
    leaving the corners. This is overcomable though, because the shifting can be
    done in the prefftshader itself by multiplying with a correct phase factor.
    For now, the best plan is probably to leave everything as is now, make a
    working implementation and then slowly and carefully remove the perhaps
    unnecessary things like separate work and input buffers, and user created
    temp buffer.
    */

    cfg.frequencyZeroPadding = 1;
    cfg.performZeropadding[0] = 1;
    cfg.performZeropadding[1] = 1;
    cfg.fft_zeropad_left[0] = l;
    cfg.fft_zeropad_right[0] = RCS_RESOLUTION;
    cfg.fft_zeropad_left[1] = l;
    cfg.fft_zeropad_right[1] = RCS_RESOLUTION;

    //cfg.disableReorderFourStep = 1;

#endif

    VkFFTResult res = initializeVkFFT(fftapp, cfg);

    assert(res == VKFFT_SUCCESS);

    CLEANUP_START_NORES(FFTCleanup){fftapp} CLEANUP_END(fftapp)

        * out_fftapp = fftapp;

    vkDestroyFence(persist_dev, fft_fen, NULL);
    vkDestroyCommandPool(persist_dev, fft_cmdpool, NULL);

    return false;
}