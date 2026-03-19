#ifndef BACKEND_H
#define BACKEND_H
#include "common.h"
#include "cleanupstack.h"
#include<GLFW/glfw3.h>

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

} RenderBackend;

void init_backend(RenderBackend* out_rb, CleanupStack* cs);

#endif