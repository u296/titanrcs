#include "instance.h"
#include "cleanupstack.h"
#include "common.h"
#include "vulkan/vulkan_core.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


constexpr usize N_BASE_EXT = 2;
const char* extensions[N_BASE_EXT] = {
	VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
	VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
};

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT message_type,
	const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
	void* user_data
) {
	if ( true  || severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
		printf("[DBG]: %s\n", callback_data->pMessage);
	}

	return VK_FALSE;
}

VkDebugUtilsMessengerCreateInfoEXT make_debugcreateinfo() {
	VkDebugUtilsMessengerCreateInfoEXT mci = {};
	mci.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	mci.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	mci.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	mci.pfnUserCallback = debug_callback;
	mci.pUserData = NULL;
	return mci;
}

void destroy_instance(void* obj) {
	VkInstance* inst = (VkInstance*)obj;
	vkDestroyInstance(*inst,NULL);
}

bool make_instance(VkInstance* instance, struct Error* e_out, CleanupStack* cs) {

    u32 n_instance_ext = 0;
	u32 n_layer = 0;
    VkExtensionProperties* instance_ext_prop = NULL;
	VkLayerProperties* layer_prop = NULL;
    vkEnumerateInstanceExtensionProperties(NULL, &n_instance_ext, NULL);
	vkEnumerateInstanceLayerProperties(&n_layer, NULL);

    instance_ext_prop = malloc(n_instance_ext * sizeof(VkExtensionProperties));
	layer_prop = malloc(n_layer * sizeof(VkLayerProperties));
    vkEnumerateInstanceExtensionProperties(NULL, &n_instance_ext, instance_ext_prop);
	vkEnumerateInstanceLayerProperties(&n_layer, layer_prop);

    printf("available instance extensions:\n");
    for (u32 i = 0; i < n_instance_ext; i++) {
        printf("\t%s\n", instance_ext_prop[i].extensionName);
    }

	printf("available instance layers:\n");
    for (u32 i = 0; i < n_layer; i++) {
        printf("\t%s\n", layer_prop[i].layerName);
    }

	u32 n_glfw_ext = 0;
	const char** glfw_ext = glfwGetRequiredInstanceExtensions(&n_glfw_ext);


	u32 n_tot_ext = n_glfw_ext + N_BASE_EXT;
	const char** all_ext = malloc(n_tot_ext * sizeof(const char*));

	for (u32 i = 0; i < N_BASE_EXT; i++) {
		all_ext[i] = extensions[i];
	}
	for (u32 i = 0; i < n_glfw_ext; i++) {
		all_ext[i + N_BASE_EXT] = glfw_ext[i];
	}

	printf("requesting these extensions:\n");
	for (u32 i = 0; i < n_tot_ext; i++) {
		printf("\t%s\n", all_ext[i]);
	}

	VkDebugUtilsMessengerCreateInfoEXT mci = make_debugcreateinfo();

    VkApplicationInfo appinfo = {};
    appinfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appinfo.apiVersion = VK_API_VERSION_1_4;
    appinfo.engineVersion = VK_MAKE_VERSION(0,0,0);
    appinfo.applicationVersion = VK_MAKE_VERSION(0,0,0);
    appinfo.pEngineName = "none";
    appinfo.pApplicationName = "diddytron";
    appinfo.pNext = NULL;

    VkInstanceCreateInfo ici = {};
    ici.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ici.pApplicationInfo = &appinfo;
    ici.enabledExtensionCount = n_tot_ext;
    ici.ppEnabledExtensionNames = all_ext;
    ici.enabledLayerCount = 0;
    ici.ppEnabledLayerNames = NULL;
    ici.pNext = &mci;

	const char* desired_layers[1] = {"VK_LAYER_KHRONOS_validation"};

	for (u32 i = 0; i < n_layer; i++) {
		if (strcmp(desired_layers[0], layer_prop[i].layerName) == 0) {
			ici.ppEnabledLayerNames = desired_layers;
			ici.enabledLayerCount = 1;
			printf("requesting validation layer\n");
		}
	}

    VkResult r = vkCreateInstance(&ici,NULL,instance);
    printf("tried to create instance\n");

    free(instance_ext_prop);
	free(all_ext);

    if (cs != NULL && r == VK_SUCCESS) {
        cs_push(cs, instance, sizeof(*instance), destroy_instance);
    }

    VERIFY("instance", r)

    return false;
}

typedef struct DebugCleanup {
	VkInstance inst;
	VkDebugUtilsMessengerEXT msg;
} DebugCleanup;

void destroy_messenger(void* obj) {
	struct DebugCleanup* d = (struct DebugCleanup*)obj;
	vkDestroyDebugUtilsMessengerEXT(d->inst, d->msg, NULL);
}

bool make_debugger(VkInstance instance, VkDebugUtilsMessengerEXT* messenger, struct Error* e_out, CleanupStack* cs) {
	VkDebugUtilsMessengerCreateInfoEXT mci = make_debugcreateinfo();

	VkResult r = vkCreateDebugUtilsMessengerEXT(instance, &mci, NULL, messenger);

    if (cs != NULL && r == VK_SUCCESS) {
        DebugCleanup d = {instance, *messenger};
        cs_push(cs, &d, sizeof(d), destroy_messenger);
    }

	VERIFY("debug messenger", r)
	printf("created debug messenger\n");
	return false;
}

typedef struct SurfaceCleanup {
	VkInstance inst;
	VkSurfaceKHR surf;
} SurfaceCleanup;

void destroy_surface(void* obj) {
	struct SurfaceCleanup* s = (struct SurfaceCleanup*)obj;
	vkDestroySurfaceKHR(s->inst,s->surf, NULL);
}


bool make_surface(VkInstance inst, GLFWwindow* wnd, VkSurfaceKHR* surface, struct Error* e_out, CleanupStack* cs) {
	VkResult r = glfwCreateWindowSurface(inst, wnd, NULL, surface);

    if (cs != NULL && r == VK_SUCCESS) {
        SurfaceCleanup s = {inst, *surface};
        cs_push(cs, &s, sizeof(s), destroy_surface);
    }

	VERIFY("create surface", r)
    return false;
}












