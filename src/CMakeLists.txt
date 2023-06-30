# add_library(enhanced-containers STATIC secure_allocator.cpp)
add_library(enhanced-containers STATIC
  no_swap_allocator.cpp
)

target_include_directories(enhanced-containers PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include;${CMAKE_BINARY_DIR}/include>"
  $<INSTALL_INTERFACE:include>
)
