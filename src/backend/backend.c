#include "backend/backend.h"
#include "GLFW/glfw3.h"
#include "cleanupstack.h"
#include <assert.h>

#include "backend/allocator.h"
#include "backend/device.h"
#include "backend/instance.h"
#include "backend/fft.h"
#include "context.h"
#include "res.h"

#define CHECK assert(f == false)

constexpr u32 WIDTH = 800;
constexpr u32 HEIGHT = 600;

bool make_window(GLFWwindow** window) {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    *window = glfwCreateWindow(WIDTH, HEIGHT, "TitanRCS", NULL, NULL);

    return false;
}

void destroy_window(void* obj) {
    GLFWwindow** window = (GLFWwindow**)obj;
    glfwDestroyWindow(*window);
    glfwTerminate();
}

void fb_resize_callback(GLFWwindow* wnd, int width, int height) {
    RenderContext* ctx = (RenderContext*)glfwGetWindowUserPointer(wnd);
    ctx->backend.fb_resized = true;
}



void keypress_callback(GLFWwindow* wnd, int key, int scancode, int action, int mods) {
    RenderContext* ctx = (RenderContext*)glfwGetWindowUserPointer(wnd);

    constexpr f32 ZOOMSTEP = 1.1f;

    if (key == GLFW_KEY_UP && (action & (GLFW_PRESS | GLFW_REPEAT))) {
        ctx->config.zoom *= ZOOMSTEP;
    } else if(key == GLFW_KEY_DOWN && (action & (GLFW_PRESS | GLFW_REPEAT))) {
        ctx->config.zoom *= (1.0f) / ZOOMSTEP;
    }
    printf("action: %i\n", action);
}

void init_backend(RenderBackend* rb, CleanupStack* cs) {

    Error e;

    VkResult f;

    make_window(&rb->wnd);
    CLEANUP_START_ONORES(GLFWwindow*)
    rb->wnd CLEANUP_END(window)

        
    glfwSetFramebufferSizeCallback(rb->wnd, fb_resize_callback);
    glfwSetKeyCallback(rb->wnd, keypress_callback);

    f = make_instance(&rb->inst, &e, cs);
    CHECK;

    volkLoadInstance(rb->inst);

    f = make_debugger(rb->inst, &rb->debugmsg, &e, cs);
    CHECK;

    f = make_surface(rb->inst, rb->wnd, &rb->surf, &e, cs);
    CHECK;

    f = make_device(rb->inst, rb->surf, &rb->physdev, &rb->dev, &rb->queues, &e, cs);
    CHECK;

    volkLoadDevice(rb->dev);

    make_allocator(rb, cs);

    fft_populate_persistents(rb->dev, rb->physdev, rb->queues);

    for (u32 i = 0; i < N_MAX_INFLIGHT; i++) {
        make_fftapp(rb->inst, &rb->fft[i], cs);
    }
}