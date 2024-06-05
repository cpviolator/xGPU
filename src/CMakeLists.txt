set (XGPU_OBJS
  # cmake-format: sortable
  cpu_util.c
  cuda_xengine.cu
  omp_xengine.c
  )

#set (CUDA_CORRELATOR_OBJS cuda_correlator.c)
#set(XGPUINFO_OBJS xgpuinfo.c)

# generate a cmake object library for all cpp files first
add_library(xgpu ${XGPU_OBJS})

# make one library
set(XGPU_LIB xgpu)

# add executables
add_executable(cuda_correlator cuda_correlator.c)
target_link_libraries(cuda_correlator ${XGPU_LIB} ${CUDA_nvml_LIBRARY})

add_executable(xgpuinfo xgpuinfo.c)
target_link_libraries(xgpuinfo ${XGPU_LIB} ${CUDA_nvml_LIBRARY} "-lstdc++")

# Install library
install(TARGETS xgpu LIBRARY DESTINATION lib)

# Install headers
set(XGPU_HEADERS
  power.h
  xgpu.h
  xgpu_info.h
  )
install(FILES ${XGPU_HEADERS} DESTINATION include)

# Install executables
install(TARGETS cuda_correlator xgpuinfo RUNTIME DESTINATION bin)
