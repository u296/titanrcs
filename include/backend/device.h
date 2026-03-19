#ifndef DEVICE_H
#define DEVICE_H
#include "cleanupstack.h"
#include "common.h"
#include "volk.h"
#include "vulkan/vulkan_core.h"
#include "backend/backend.h"


bool make_device(VkInstance instance, VkSurfaceKHR surf, VkPhysicalDevice* physdev, VkDevice* device, Queues* queues, struct Error* e_out, CleanupStack* cs);



#endif