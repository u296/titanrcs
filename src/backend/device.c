#include "backend/device.h"
#include "cleanupstack.h"
#include "vulkan/vulkan_core.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan_beta.h>

#ifdef __APPLE__

#define N_REQ_DEV_EXT (3)
const char* device_extensions[N_REQ_DEV_EXT] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
    VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME};

#else

#define N_REQ_DEV_EXT (2)
const char* device_extensions[N_REQ_DEV_EXT] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME};

#endif

bool physical_device_has_req_extensions(VkPhysicalDevice dev) {
    u32 n_av_ext = 0;
    vkEnumerateDeviceExtensionProperties(dev, NULL, &n_av_ext, NULL);
    VkExtensionProperties* ext_props =
        malloc(sizeof(VkExtensionProperties) * n_av_ext);
    vkEnumerateDeviceExtensionProperties(dev, NULL, &n_av_ext, ext_props);

    for (u32 i = 0; i < N_REQ_DEV_EXT; i++) {
        for (u32 j = 0; j < n_av_ext; j++) {
            if (strcmp(ext_props[j].extensionName, device_extensions[i]) == 0) {
                break;
            }

            if (j == n_av_ext - 1) {
                free(ext_props);
                return false;
            }
        }
    }

    free(ext_props);

    return true;
}

bool physical_device_find_queuefams(VkPhysicalDevice dev, VkSurfaceKHR surf,
                                    Queues* fams) {
    u32 n_qfam = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &n_qfam, NULL);

    VkQueueFamilyProperties* qfams =
        malloc(n_qfam * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &n_qfam, qfams);

    Queues sqf = {};
    Queues set_if_found = {};

    printf("queue families of selected device (%u total):\n", n_qfam);
    for (u32 i = 0; i < n_qfam; i++) {
        printf("\tindex %u flags: %x\n", i, qfams[i].queueFlags);
        if (!set_if_found.i_graphics_queue_fam &&
            qfams[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            sqf.i_graphics_queue_fam = i;
            set_if_found.i_graphics_queue_fam = 1;
        }
        VkBool32 presentsupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, surf, &presentsupport);
        if (!set_if_found.i_present_queue_fam && presentsupport == VK_TRUE) {
            sqf.i_present_queue_fam = i;
            set_if_found.i_present_queue_fam = 1;
        }
    }

    free(qfams);

    if (!set_if_found.i_graphics_queue_fam ||
        !set_if_found.i_present_queue_fam) {
        return false;
    }
    *fams = sqf;
    return true;
}

void destroy_device(void* obj) {
    VkDevice* dev = (VkDevice*)obj;
    vkDestroyDevice(*dev, NULL);
}

bool make_device(VkInstance instance, VkSurfaceKHR surf,
                 VkPhysicalDevice* physdev, VkDevice* device, Queues* queues,
                 struct Error* e_out, CleanupStack* cs) {

    u32 n_dev = 0;
    VkPhysicalDevice* devs = NULL;
    vkEnumeratePhysicalDevices(instance, &n_dev, NULL);

    devs = malloc(sizeof(VkPhysicalDevice) * n_dev);
    VkPhysicalDeviceProperties* props =
        malloc(sizeof(VkPhysicalDeviceProperties) * n_dev);
    VkPhysicalDeviceProperties2* props2 =
        malloc(sizeof(VkPhysicalDeviceProperties2) * n_dev);
    VkPhysicalDeviceVulkan11Properties* vk11props =
        malloc(sizeof(VkPhysicalDeviceVulkan11Properties) * n_dev);
    VkPhysicalDeviceFeatures* feats =
        malloc(sizeof(VkPhysicalDeviceFeatures) * n_dev);
    VkPhysicalDeviceDriverProperties* driver_props =
        malloc(sizeof(VkPhysicalDeviceDriverProperties) * n_dev);

    vkEnumeratePhysicalDevices(instance, &n_dev, devs);

    printf("Physical devices found: %u\n", n_dev);
    for (u32 i = 0; i < n_dev; i++) {
        props2[i] = (VkPhysicalDeviceProperties2){};
        props2[i].sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        props2[i].pNext = &vk11props[i];

        vk11props[i] = (VkPhysicalDeviceVulkan11Properties){};
        vk11props[i].sType =
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES;
        vk11props[i].pNext = &driver_props[i];

        driver_props[i] = (VkPhysicalDeviceDriverProperties){};
        driver_props[i].sType =
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES;
        driver_props[i].pNext = NULL;

        vkGetPhysicalDeviceProperties(devs[i], &props[i]);
        vkGetPhysicalDeviceProperties2(devs[i], &props2[i]);
        vkGetPhysicalDeviceFeatures(devs[i], &feats[i]);
        printf("\t%s\n", props[i].deviceName);

        printf("driver has this name: %s\n", driver_props->driverName);
    }

    if (n_dev == 0) {
        printf("no devices found, aborting");
        exit(1);
    }

    {

        u32 n_ext = 0;
        vkEnumerateDeviceExtensionProperties(devs[0], NULL, &n_ext, NULL);

        VkExtensionProperties* devexts =
            malloc(n_ext * sizeof(VkExtensionProperties));

        vkEnumerateDeviceExtensionProperties(devs[0], NULL, &n_ext, devexts);

        printf("device 0 has these device extensions:\n");

        for (u32 i = 0; i < n_ext; i++) {
            printf("\t%s\n", devexts[i].extensionName);
        }

        free(devexts);
    }

    // just select device 0

    Queues sqf = {};

    u32 i_sel = UINT32_MAX;
    for (u32 i = 0; i < n_dev; i++) {
        if (physical_device_has_req_extensions(devs[i]) &&
            physical_device_find_queuefams(devs[i], surf, &sqf)) {
            i_sel = i;
            break;
        }
    }

    if (i_sel == UINT32_MAX) {
        printf("NO SUITABLE DEVICE\n");
        return true;
    }

    {
        printf("supported subgroup ops: 0x%x\n",
               vk11props[i_sel].subgroupSupportedOperations);
    }

    float qprio = 1.0f;

    VkDeviceQueueCreateInfo queue_infos[2];
    u32 n_queue_infos = 0;

    if (sqf.i_graphics_queue_fam == sqf.i_present_queue_fam) {
        VkDeviceQueueCreateInfo dgqci = {};
        dgqci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        dgqci.queueFamilyIndex = sqf.i_graphics_queue_fam;
        dgqci.queueCount = 1;
        dgqci.pQueuePriorities = &qprio;

        queue_infos[0] = dgqci;
        n_queue_infos = 1;
    } else {
        VkDeviceQueueCreateInfo dgqci = {};
        dgqci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        dgqci.queueFamilyIndex = sqf.i_graphics_queue_fam;
        dgqci.queueCount = 1;
        dgqci.pQueuePriorities = &qprio;

        VkDeviceQueueCreateInfo dpqci = {};
        dpqci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        dpqci.queueFamilyIndex = sqf.i_present_queue_fam;
        dpqci.queueCount = 1;
        dpqci.pQueuePriorities = &qprio;

        queue_infos[0] = dgqci;
        queue_infos[1] = dpqci;
        n_queue_infos = 2;
    }

    VkPhysicalDeviceVulkan14Features vk14features = {};
    vk14features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES;
    vk14features.pushDescriptor = VK_TRUE;
#ifdef TR_USE_ADVANCED_LINES
    vk14features.rectangularLines = VK_TRUE;
#endif
    vk14features.pNext = NULL;

    VkPhysicalDeviceVulkan13Features vk13features = {};
    vk13features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    vk13features.synchronization2 = VK_TRUE;
    vk13features.dynamicRendering = VK_TRUE;
    vk13features.maintenance4 = VK_TRUE;
    vk13features.pNext = &vk14features;

    VkPhysicalDeviceFeatures2 df2 = {};
    df2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    df2.features.shaderSampledImageArrayDynamicIndexing = VK_TRUE;
    df2.features.independentBlend = VK_TRUE;
#ifdef TR_USE_ADVANCED_LINES
    df2.features.wideLines = VK_TRUE;
#endif
    df2.pNext = &vk13features;

    // VkPhysicalDeviceFeatures df = {};

    VkDeviceCreateInfo dci = {};
    dci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    dci.queueCreateInfoCount = n_queue_infos;
    dci.pQueueCreateInfos = queue_infos;
    dci.enabledExtensionCount = N_REQ_DEV_EXT;
    dci.ppEnabledExtensionNames = device_extensions;
    dci.pEnabledFeatures = NULL;
    dci.pNext = &df2;

    VkResult r = vkCreateDevice(devs[i_sel], &dci, NULL, device);
    *physdev = devs[i_sel];

    free(props);
    free(devs);

    if (cs != NULL && r == VK_SUCCESS) {
        cs_push(cs, device, sizeof(*device), destroy_device);
    }

    VERIFY("device creation", r);
    printf("created device\n");

    vkGetDeviceQueue(*device, sqf.i_graphics_queue_fam, 0,
                     &queues->graphics_queue);
    vkGetDeviceQueue(*device, sqf.i_present_queue_fam, 0,
                     &queues->present_queue);
    printf("got graphics queue %p\n", queues->graphics_queue);
    printf("got present queue %p\n", queues->present_queue);
    queues->i_graphics_queue_fam = sqf.i_graphics_queue_fam;
    queues->i_present_queue_fam = sqf.i_present_queue_fam;

    // exit(0);
    return false;
}
