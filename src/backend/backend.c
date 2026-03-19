#include "backend/backend.h"
#include "cleanupstack.h"
#include <assert.h>

#include "backend/instance.h"
#include "backend/device.h"


#define CHECK assert(f == false)

constexpr u32 WIDTH = 800;
constexpr u32 HEIGHT = 600;

bool make_window(GLFWwindow** window) {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	//glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	*window = glfwCreateWindow(WIDTH, HEIGHT, "TitanRCS", NULL, NULL);

	return false;
}

void destroy_window(void* obj) {
	GLFWwindow** window = (GLFWwindow**)obj;
	glfwDestroyWindow(*window);
	glfwTerminate();
}

void fb_resize_callback(GLFWwindow* wnd, int width, int height) {
	bool* fbresize = (bool*)glfwGetWindowUserPointer(wnd);
	*fbresize = true;
}

void init_backend(RenderBackend* rb, CleanupStack* cs) {

    Error e;

    VkResult f;

    make_window(&rb->wnd);
	CLEANUP_START_ONORES(GLFWwindow*)
	rb->wnd
	CLEANUP_END(window)

	glfwSetWindowUserPointer(rb->wnd, &rb->fb_resized);
	glfwSetFramebufferSizeCallback(rb->wnd, fb_resize_callback);

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
}