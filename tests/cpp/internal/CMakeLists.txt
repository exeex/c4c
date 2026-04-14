set(INTERNAL_CPP_TEST_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(INTERNAL_C_TEST_CMAKE_ROOT "${PROJECT_SOURCE_DIR}/tests/c/internal/cmake")

file(GLOB_RECURSE INTERNAL_CPP_POSITIVE_TEST_SRCS CONFIGURE_DEPENDS
     "${INTERNAL_CPP_TEST_ROOT}/postive_case/*.cpp")
file(GLOB_RECURSE INTERNAL_CPP_GENERATED_PARSE_ONLY_TEST_SRCS CONFIGURE_DEPENDS
     "${INTERNAL_CPP_TEST_ROOT}/generated/parser_disambiguation_matrix/parse_only/*.cpp")
file(GLOB_RECURSE INTERNAL_CPP_GENERATED_COMPILE_POSITIVE_TEST_SRCS CONFIGURE_DEPENDS
     "${INTERNAL_CPP_TEST_ROOT}/generated/parser_disambiguation_matrix/compile_positive/*.cpp")

list(APPEND INTERNAL_CPP_POSITIVE_TEST_SRCS ${INTERNAL_CPP_GENERATED_PARSE_ONLY_TEST_SRCS})
list(APPEND INTERNAL_CPP_POSITIVE_TEST_SRCS ${INTERNAL_CPP_GENERATED_COMPILE_POSITIVE_TEST_SRCS})






if(CLANG_EXECUTABLE)
  foreach(src IN LISTS INTERNAL_CPP_POSITIVE_TEST_SRCS)
    file(RELATIVE_PATH rel_src "${INTERNAL_CPP_TEST_ROOT}/postive_case" "${src}")
    string(REGEX REPLACE "^(parse|frontend|runtime)/" "" rel_src "${rel_src}")
    string(REGEX REPLACE "[^A-Za-z0-9_]" "_" test_id "${rel_src}")
    set(test_name "cpp_positive_sema_${test_id}")
    get_filename_component(_src_dir "${src}" DIRECTORY)
    get_filename_component(_mode_hint "${_src_dir}" NAME)
    if(_mode_hint STREQUAL "parse" OR _mode_hint STREQUAL "parse_only")
      set(test_mode parse)
    elseif(_mode_hint STREQUAL "frontend" OR _mode_hint STREQUAL "compile_positive")
      set(test_mode frontend)
    else()
      set(test_mode runtime)
    endif()

    add_test(
      NAME "${test_name}"
      COMMAND "${CMAKE_COMMAND}"
              -DCOMPILER=$<TARGET_FILE:c4cll>
              -DCLANG=${CLANG_EXECUTABLE}
              -DCXX_COMPILER=${CMAKE_CXX_COMPILER}
              -DTEST_MODE=${test_mode}
              -DSRC=${src}
              -DOUT_LL=${CMAKE_BINARY_DIR}/internal_cpp_positive/${rel_src}.ll
              -DOUT_CLANG_BIN=${CMAKE_BINARY_DIR}/internal_cpp_positive/${rel_src}.host.bin
              -DOUT_C2LL_BIN=${CMAKE_BINARY_DIR}/internal_cpp_positive/${rel_src}.c2ll.bin
              -P "${INTERNAL_C_TEST_CMAKE_ROOT}/run_positive_case.cmake"
    )
    set_tests_properties("${test_name}" PROPERTIES LABELS "internal;positive_case;cpp")
  endforeach()
else()
  message(WARNING "clang not found: skipping internal cpp positive_case runtime tests")
endif()

add_subdirectory(postive_case)
add_subdirectory(parse_only_case)
add_subdirectory(negative_case)
add_subdirectory(hir_case)
add_subdirectory(consteval_case)

add_test(
  NAME cpp_eastl_piecewise_construct_parse_recipe
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DROOT=${PROJECT_SOURCE_DIR}
          -DSRC=${PROJECT_SOURCE_DIR}/tests/cpp/eastl/eastl_piecewise_construct_simple.cpp
          -DEXPECT_SUCCESS=ON
          -P "${PROJECT_SOURCE_DIR}/tests/cpp/eastl/run_eastl_parse_recipe.cmake"
)
set_tests_properties(cpp_eastl_piecewise_construct_parse_recipe PROPERTIES
  LABELS "internal;positive_case;cpp;workflow"
  TIMEOUT 30
)

add_test(
  NAME cpp_eastl_tuple_fwd_decls_parse_recipe
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DROOT=${PROJECT_SOURCE_DIR}
          -DSRC=${PROJECT_SOURCE_DIR}/tests/cpp/eastl/eastl_tuple_fwd_decls_simple.cpp
          -DEXPECT_SUCCESS=ON
          -P "${PROJECT_SOURCE_DIR}/tests/cpp/eastl/run_eastl_parse_recipe.cmake"
)
set_tests_properties(cpp_eastl_tuple_fwd_decls_parse_recipe PROPERTIES
  LABELS "internal;positive_case;cpp;workflow"
  TIMEOUT 30
)

add_test(
  NAME cpp_eastl_integer_sequence_parse_recipe
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DROOT=${PROJECT_SOURCE_DIR}
          -DSRC=${PROJECT_SOURCE_DIR}/tests/cpp/eastl/eastl_integer_sequence_simple.cpp
          -DEXPECT_SUCCESS=ON
          -P "${PROJECT_SOURCE_DIR}/tests/cpp/eastl/run_eastl_parse_recipe.cmake"
)
set_tests_properties(cpp_eastl_integer_sequence_parse_recipe PROPERTIES
  LABELS "internal;positive_case;cpp;workflow"
  TIMEOUT 30
)

add_test(
  NAME cpp_eastl_type_traits_parse_recipe
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DROOT=${PROJECT_SOURCE_DIR}
          -DSRC=${PROJECT_SOURCE_DIR}/tests/cpp/eastl/eastl_type_traits_simple.cpp
          -DEXPECT_SUCCESS=ON
          -P "${PROJECT_SOURCE_DIR}/tests/cpp/eastl/run_eastl_parse_recipe.cmake"
)
set_tests_properties(cpp_eastl_type_traits_parse_recipe PROPERTIES
  LABELS "internal;positive_case;cpp;workflow"
  TIMEOUT 30
)

add_test(
  NAME cpp_eastl_utility_parse_recipe
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DROOT=${PROJECT_SOURCE_DIR}
          -DSRC=${PROJECT_SOURCE_DIR}/tests/cpp/eastl/eastl_utility_simple.cpp
          -DEXPECT_SUCCESS=ON
          -P "${PROJECT_SOURCE_DIR}/tests/cpp/eastl/run_eastl_parse_recipe.cmake"
)
set_tests_properties(cpp_eastl_utility_parse_recipe PROPERTIES
  LABELS "internal;positive_case;cpp;workflow"
  TIMEOUT 30
  RUN_SERIAL TRUE
)

add_test(
  NAME cpp_eastl_vector_parse_recipe
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DROOT=${PROJECT_SOURCE_DIR}
          -DSRC=${PROJECT_SOURCE_DIR}/tests/cpp/eastl/eastl_vector_simple.cpp
          -DCASE_TIMEOUT_SEC=25
          -DEXPECT_SUCCESS=ON
          -P "${PROJECT_SOURCE_DIR}/tests/cpp/eastl/run_eastl_parse_recipe.cmake"
)
set_tests_properties(cpp_eastl_vector_parse_recipe PROPERTIES
  LABELS "internal;positive_case;cpp;workflow"
  TIMEOUT 45
  RUN_SERIAL TRUE
)

add_test(
  NAME cpp_eastl_memory_uses_allocator_parse_recipe
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DROOT=${PROJECT_SOURCE_DIR}
          -DSRC=${PROJECT_SOURCE_DIR}/tests/cpp/eastl/eastl_memory_uses_allocator_frontier.cpp
          -DCASE_TIMEOUT_SEC=25
          -DEXPECT_SUCCESS=ON
          -P "${PROJECT_SOURCE_DIR}/tests/cpp/eastl/run_eastl_parse_recipe.cmake"
)
set_tests_properties(cpp_eastl_memory_uses_allocator_parse_recipe PROPERTIES
  LABELS "internal;positive_case;cpp;workflow"
  TIMEOUT 35
  RUN_SERIAL TRUE
)

add_test(
  NAME cpp_std_vector_parse_debug_progress
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DSRC=${PROJECT_SOURCE_DIR}/tests/cpp/std/std_vector_simple.cpp
          -DEXPECT_TIMEOUT_SEC=5
          "-DPARSER_DEBUG_ARGS:STRING=--parser-debug"
          "-DEXPECT_STDERR_SUBSTRING:STRING=[pdebug] progress"
          -P "${PROJECT_SOURCE_DIR}/tests/cpp/std/run_std_vector_parse_baseline.cmake"
)
set_tests_properties(cpp_std_vector_parse_debug_progress PROPERTIES
  LABELS "internal;positive_case;cpp;workflow;diagnostic_format"
  TIMEOUT 15
)

add_test(
  NAME cpp_std_vector_parse_debug_progress_file
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DSRC=${PROJECT_SOURCE_DIR}/tests/cpp/std/std_vector_simple.cpp
          -DEXPECT_TIMEOUT_SEC=5
          "-DPARSER_DEBUG_ARGS:STRING=--parser-debug"
          "-DEXPECT_STDERR_SUBSTRING:STRING=file="
          -P "${PROJECT_SOURCE_DIR}/tests/cpp/std/run_std_vector_parse_baseline.cmake"
)
set_tests_properties(cpp_std_vector_parse_debug_progress_file PROPERTIES
  LABELS "internal;positive_case;cpp;workflow;diagnostic_format"
  TIMEOUT 15
)

set(_perf_guard "${PROJECT_SOURCE_DIR}/tests/cpp/internal/run_perf_guard.cmake")
set(_perf_gen  "${PROJECT_SOURCE_DIR}/tests/cpp/internal/perf_gen.py")

add_test(
  NAME cpp_typedef_owned_functional_cast_perf
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DBINARY_DIR=${PROJECT_BINARY_DIR}
          -DREPEAT_COUNT=5000
          -DEXPECT_TIMEOUT_SEC=5
          -DTEST_NAME=typedef_owned_functional_cast
          -DGENERATOR=${_perf_gen}
          -P "${_perf_guard}"
)
set_tests_properties(cpp_typedef_owned_functional_cast_perf PROPERTIES
  LABELS "internal;positive_case;cpp;workflow;parser_perf"
  TIMEOUT 15
)

add_test(
  NAME cpp_qualified_known_type_arg_perf
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DBINARY_DIR=${PROJECT_BINARY_DIR}
          -DREPEAT_COUNT=5000
          -DEXPECT_TIMEOUT_SEC=5
          -DTEST_NAME=qualified_known_type_arg
          -DGENERATOR=${_perf_gen}
          -P "${_perf_guard}"
)
set_tests_properties(cpp_qualified_known_type_arg_perf PROPERTIES
  LABELS "internal;positive_case;cpp;workflow;parser_perf"
  TIMEOUT 15
)

add_test(
  NAME cpp_qualified_template_call_template_arg_perf
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DBINARY_DIR=${PROJECT_BINARY_DIR}
          -DREPEAT_COUNT=5000
          -DEXPECT_TIMEOUT_SEC=5
          -DTEST_NAME=qualified_template_call_template_arg
          -DGENERATOR=${_perf_gen}
          -P "${_perf_guard}"
)
set_tests_properties(cpp_qualified_template_call_template_arg_perf PROPERTIES
  LABELS "internal;positive_case;cpp;workflow;parser_perf"
  TIMEOUT 15
)
