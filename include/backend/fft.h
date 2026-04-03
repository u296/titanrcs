#ifndef FFT_H
#define FFT_H
#include <vkFFT.h>

typedef struct Queues Queues;
typedef struct CleanupStack CleanupStack;

void fft_populate_persistents(VkDevice dev, VkPhysicalDevice physdev, Queues queues);

bool make_fftapp(VkInstance inst,
                 VkFFTApplication** out_fftapp, CleanupStack* cs);

#endif