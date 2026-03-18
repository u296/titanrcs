#ifndef INSTANCE_H
#define INSTANCE_H
#include "common.h"
#include <GLFW/glfw3.h>
#include "cleanupstack.h"

bool make_instance(VkInstance* instance, struct Error* e_out, CleanupStack* cs);



bool make_debugger(VkInstance instance, VkDebugUtilsMessengerEXT* messenger, struct Error* e_out, CleanupStack* cs);
void destroy_debugger(void* obj);




bool make_surface(VkInstance inst, GLFWwindow* wnd, VkSurfaceKHR* surface, struct Error* e_out, CleanupStack* cs);
void destroy_surface(void* obj);

#endif