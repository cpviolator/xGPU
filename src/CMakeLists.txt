set (XGPU_OBJS
  # cmake-format: sortable
  cpu_util.c
  #cuda_correlator.c
  cuda_xengine.cu
  omp_xengine.c
  #xgpuinfo.c
  )

#set (CUDA_CORRELATOR_OBJS cuda_correlator.c)
#set(XGPUINFO_OBJS xgpuinfo.c)

# generate a cmake object library for all cpp files first
add_library(xgpu ${XGPU_OBJS})

enable_language(CUDA)

# make one library
set(XGPU_LIB xgpu)

# add executables
add_executable(cuda_correlator cuda_correlator.c)
target_link_libraries(cuda_correlator ${XGPU_LIB} ${CUDA_nvml_LIBRARY})

#add_executable(xgpuinfo xgpuinfo.c)
#target_link_libraries(xgpuinfo ${XGPU_LIB} "-lstdc++")
