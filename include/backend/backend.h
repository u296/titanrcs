#ifndef BACKEND_H
#define BACKEND_H
#include "cleanupstack.h"
#include "common.h"
#include "res.h"
#include <vk_mem_alloc.h>
#include <vkFFT.h>
#include <GLFW/glfw3.h>

typedef struct Queues {
    VkQueue graphics_queue;
    u32 i_graphics_queue_fam;

    VkQueue present_queue;
    u32 i_present_queue_fam;
} Queues;

typedef struct RenderBackend {
    GLFWwindow* wnd;
    bool fb_resized;
    VkInstance inst;
    VkDebugUtilsMessengerEXT debugmsg;
    VkSurfaceKHR surf;
    VkPhysicalDevice physdev;
    VkDevice dev;
    Queues queues;
    VmaAllocator alloc;
    VkFFTApplication* fft[N_MAX_INFLIGHT];
} RenderBackend;

void init_backend(RenderBackend* out_rb, CleanupStack* cs);

#endif