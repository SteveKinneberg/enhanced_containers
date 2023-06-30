add_library(mocks STATIC
  mock_allocator.cpp
  mock_c_lib.cpp
  mock_memory.cpp
)
target_link_libraries(mocks PUBLIC gmock dl)
target_include_directories(mocks PRIVATE ${CMAKE_SOURCE_DIR}/include)

function(ec_test unit_test)
  set(target "${unit_test}_test")
  add_executable(${target} "${unit_test}.cpp" "${ARGN}")
  target_compile_definitions(${target} PRIVATE EC_UNIT_TEST_SUPPORT=1)
  target_include_directories(${target} PRIVATE ${CMAKE_SOURCE_DIR}/include)
  target_link_libraries(${target} mocks gtest gmock gmock_main dl fmt)
  gtest_discover_tests(${target})
  add_dependencies(${UNIT_TESTS_TARGET} ${target})
endfunction()

ec_test(zero_on_release_allocator)
ec_test(no_swap_allocator  ${CMAKE_SOURCE_DIR}/src/no_swap_allocator.cpp)
ec_test(secure_allocator   ${CMAKE_SOURCE_DIR}/src/no_swap_allocator.cpp)
ec_test(secure_containers  ${CMAKE_SOURCE_DIR}/src/no_swap_allocator.cpp)
