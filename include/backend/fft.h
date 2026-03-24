#ifndef FFT_H
#define FFT_H
#include <vkFFT.h>

typedef struct Queues Queues;
typedef struct CleanupStack CleanupStack;

bool make_fftapp(VkInstance inst, VkPhysicalDevice physdev, VkDevice dev, Queues queues,
                 VkFFTApplication** out_fftapp, CleanupStack* cs);

#endif