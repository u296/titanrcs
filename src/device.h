#ifndef DEVICE_H
#define DEVICE_H
#include "cleanupstack.h"
#include "common.h"
#include "volk.h"
#include "vulkan/vulkan_core.h"

typedef struct Queues {
	VkQueue graphics_queue;
	u32 i_graphics_queue_fam;

	VkQueue present_queue;
	u32 i_present_queue_fam;
} Queues;

bool make_device(VkInstance instance, VkSurfaceKHR surf, VkPhysicalDevice* physdev, VkDevice* device, struct Queues* queues, struct Error* e_out, CleanupStack* cs);



#endif