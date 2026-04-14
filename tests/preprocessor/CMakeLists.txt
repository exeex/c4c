add_executable(frontend_cxx_preprocessor_tests
    "${CMAKE_CURRENT_SOURCE_DIR}/frontend_cxx_preprocessor_tests.cpp"
)
target_link_libraries(frontend_cxx_preprocessor_tests PRIVATE c4c_frontend)
set_target_properties(frontend_cxx_preprocessor_tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

add_test(NAME frontend_cxx_preprocessor_tests COMMAND frontend_cxx_preprocessor_tests)
set_tests_properties(frontend_cxx_preprocessor_tests PROPERTIES LABELS "internal;preprocessor")

file(GLOB C_PREPROCESSOR_CASE_SRCS CONFIGURE_DEPENDS
     "${PROJECT_SOURCE_DIR}/tests/c/preprocessor/*.c")
foreach(src IN LISTS C_PREPROCESSOR_CASE_SRCS)
  get_filename_component(stem "${src}" NAME_WE)
  set(test_name "preprocessor_${stem}")
  add_test(
      NAME "${test_name}"
      COMMAND "${CMAKE_COMMAND}"
              -DCOMPILER=$<TARGET_FILE:c4cll>
              -DSRC=${src}
              -P "${PROJECT_SOURCE_DIR}/tests/c/internal/cmake/run_preprocessor_case.cmake"
  )
  set_tests_properties("${test_name}" PROPERTIES LABELS "internal;preprocessor")
endforeach()
