glslc frag1.frag -o frag.spv -O &&\
glslc vert1.vert -o vert.spv -O &&\
glslc reduction.comp -o reduction.spv -O &&\
glslc ptd_frag.frag -o ptd_frag.spv -O &&\
glslc imgtobuf.comp -o imgtobuf.spv -O &&\
glslc buftoimg.comp -o buftoimg.spv -O
