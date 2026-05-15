
glslc rcsvert.vert -o rcsvert.spv -O
glslc po.frag -o po.spv -O
glslc ildc.frag -o ildc.spv -O

glslc fft/imgtobuf.comp -o fft/imgtobuf.spv -O
glslc fft/buftoimg.comp -o fft/buftoimg.spv -O
glslc fft/reduction.comp -o fft/reduction.spv -O 

glslc sum/downscalesum.comp -o sum/downscalesum.spv --target-env=vulkan1.4
glslc sum/reduction.comp -o sum/reduction.spv --target-env=vulkan1.4
