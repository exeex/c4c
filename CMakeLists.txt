cmake_minimum_required(VERSION 3.20)
project(tiny_c2ll LANGUAGES CXX)

find_program(CLANG_EXECUTABLE NAMES clang)
if(NOT CLANG_EXECUTABLE)
  message(WARNING "clang not found: clang execution tests will be skipped")
endif()

set(C_TESTSUITE_ROOT "" CACHE PATH "Path to external c-testsuite root")
if(NOT C_TESTSUITE_ROOT AND EXISTS "${PROJECT_SOURCE_DIR}/tests/c-testsuite")
  set(C_TESTSUITE_ROOT "${PROJECT_SOURCE_DIR}/tests/c-testsuite")
endif()

set(EXAMPLE_C "${PROJECT_SOURCE_DIR}/tests/tiny_c2ll/example.c")
set(EXAMPLE_LL "${CMAKE_BINARY_DIR}/example.ll")
file(GLOB NEGATIVE_TEST_SRCS CONFIGURE_DEPENDS
     "${PROJECT_SOURCE_DIR}/tests/tiny_c2ll/bad_*.c")
option(NEGATIVE_TESTS_ENFORCE "Require negative_tests bad_*.c cases to fail compilation (non-zero exit)" OFF)

set(FRONTEND_CXX_SRCS
    "${PROJECT_SOURCE_DIR}/src/frontend_c/driver.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend_c/token.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend_c/lexer.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend_c/arena.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend_c/preprocessor.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend_c/parser.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend_c/ir_builder.cpp"
)

add_executable(frontend_cxx_stage1 ${FRONTEND_CXX_SRCS})
target_include_directories(frontend_cxx_stage1 PRIVATE
    "${PROJECT_SOURCE_DIR}/src/frontend_c"
)
set_target_properties(frontend_cxx_stage1 PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    OUTPUT_NAME "tiny-c2ll-stage1"
)
target_compile_options(frontend_cxx_stage1 PRIVATE -Wall -Wextra -Wpedantic)

add_executable(frontend_cxx_preprocessor_tests
    "${PROJECT_SOURCE_DIR}/tests/preprocessor/frontend_cxx_preprocessor_tests.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend_c/preprocessor.cpp"
)
target_include_directories(frontend_cxx_preprocessor_tests PRIVATE
    "${PROJECT_SOURCE_DIR}/src/frontend_c"
)
set_target_properties(frontend_cxx_preprocessor_tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
target_compile_options(frontend_cxx_preprocessor_tests PRIVATE -Wall -Wextra -Wpedantic)

add_custom_command(
  OUTPUT "${EXAMPLE_LL}"
  COMMAND $<TARGET_FILE:frontend_cxx_stage1> "${EXAMPLE_C}" -o "${EXAMPLE_LL}"
  DEPENDS frontend_cxx_stage1 "${EXAMPLE_C}"
  COMMENT "Generating example LLVM IR"
  VERBATIM
)

add_custom_target(build_example_ir ALL DEPENDS "${EXAMPLE_LL}")

if(CLANG_EXECUTABLE)
  set(EXAMPLE_BIN "${CMAKE_BINARY_DIR}/example_bin")
  add_custom_command(
    OUTPUT "${EXAMPLE_BIN}"
    COMMAND "${CLANG_EXECUTABLE}" "${EXAMPLE_LL}" -o "${EXAMPLE_BIN}"
    DEPENDS "${EXAMPLE_LL}"
    COMMENT "Building example executable from LLVM IR"
    VERBATIM
  )
  add_custom_target(build_example_bin DEPENDS "${EXAMPLE_BIN}")
endif()

enable_testing()

if(CLANG_EXECUTABLE)
  add_test(
    NAME tiny_c2ll_tests
    COMMAND "${CMAKE_COMMAND}"
            -DCOMPILER=$<TARGET_FILE:frontend_cxx_stage1>
            -DEXAMPLE_C=${EXAMPLE_C}
            -DOUT_LL=${CMAKE_BINARY_DIR}/tiny_c2ll_tests/test_example.ll
            -DCLANG=${CLANG_EXECUTABLE}
            -DOUT_BIN=${CMAKE_BINARY_DIR}/tiny_c2ll_tests/test_example_bin
            -P "${PROJECT_SOURCE_DIR}/tests/cmake/run_tiny_c2ll_tests.cmake"
  )
else()
  add_test(
    NAME tiny_c2ll_tests
    COMMAND "${CMAKE_COMMAND}"
            -DCOMPILER=$<TARGET_FILE:frontend_cxx_stage1>
            -DEXAMPLE_C=${EXAMPLE_C}
            -DOUT_LL=${CMAKE_BINARY_DIR}/tiny_c2ll_tests/test_example.ll
            -P "${PROJECT_SOURCE_DIR}/tests/cmake/run_tiny_c2ll_tests.cmake"
  )
endif()

foreach(src IN LISTS NEGATIVE_TEST_SRCS)
  get_filename_component(stem "${src}" NAME_WE)
  set(test_name "negative_tests_${stem}")
  add_test(
    NAME "${test_name}"
    COMMAND "${CMAKE_COMMAND}"
            -DCOMPILER=$<TARGET_FILE:frontend_cxx_stage1>
            -DSRC=${src}
            -DENFORCE_NEGATIVE=${NEGATIVE_TESTS_ENFORCE}
            -P "${PROJECT_SOURCE_DIR}/tests/cmake/run_tiny_c2ll_negative_case.cmake"
  )
  set_tests_properties("${test_name}" PROPERTIES LABELS "negative_tests")
endforeach()

add_test(
    NAME frontend_cxx_preprocessor_tests
    COMMAND frontend_cxx_preprocessor_tests
)

add_test(
    NAME frontend_cxx_stage1_version
    COMMAND frontend_cxx_stage1 --version
)
set_tests_properties(frontend_cxx_stage1_version PROPERTIES
    PASS_REGULAR_EXPRESSION "tiny-c2ll"
)

if(EXISTS "${EXAMPLE_C}")
  add_test(
      NAME frontend_cxx_stage1_lex
      COMMAND frontend_cxx_stage1 --lex-only "${EXAMPLE_C}"
  )
  set_tests_properties(frontend_cxx_stage1_lex PROPERTIES
      PASS_REGULAR_EXPRESSION "EOF"
  )
endif()

if(EXISTS "${EXAMPLE_C}")
  add_test(
      NAME frontend_cxx_stage1_parse
      COMMAND frontend_cxx_stage1 --parse-only "${EXAMPLE_C}"
  )
  set_tests_properties(frontend_cxx_stage1_parse PROPERTIES
      PASS_REGULAR_EXPRESSION "Program"
  )
endif()

if(EXISTS "${EXAMPLE_C}")
  add_test(
      NAME frontend_cxx_stage1_emit_ir
      COMMAND frontend_cxx_stage1 "${EXAMPLE_C}"
  )
  set_tests_properties(frontend_cxx_stage1_emit_ir PROPERTIES
      PASS_REGULAR_EXPRESSION "define"
  )
endif()

if(CLANG_EXECUTABLE)
  file(GLOB CCC_REVIEW_SRCS CONFIGURE_DEPENDS
       "${PROJECT_SOURCE_DIR}/tests/ccc-review-tests/*.c")
  foreach(src IN LISTS CCC_REVIEW_SRCS)
    get_filename_component(stem "${src}" NAME_WE)
    set(test_name "ccc_review_${stem}")
    add_test(
      NAME "${test_name}"
      COMMAND "${CMAKE_COMMAND}"
              -DCOMPILER=$<TARGET_FILE:frontend_cxx_stage1>
              -DCLANG=${CLANG_EXECUTABLE}
              -DSRC=${src}
              -DOUT_LL=${CMAKE_BINARY_DIR}/ccc_review_tests/${stem}.ll
              -DOUT_CLANG_BIN=${CMAKE_BINARY_DIR}/ccc_review_tests/${stem}.clang.bin
              -DOUT_C2LL_BIN=${CMAKE_BINARY_DIR}/ccc_review_tests/${stem}.c2ll.bin
              -P "${PROJECT_SOURCE_DIR}/tests/cmake/run_ccc_review_case.cmake"
    )
    set_tests_properties("${test_name}" PROPERTIES LABELS "ccc_review")
  endforeach()
endif()

if(CLANG_EXECUTABLE AND C_TESTSUITE_ROOT AND EXISTS "${C_TESTSUITE_ROOT}")
  file(STRINGS "${PROJECT_SOURCE_DIR}/tests/c_testsuite_allowlist.txt" C_TESTSUITE_ALLOWLIST_RAW)
  foreach(entry IN LISTS C_TESTSUITE_ALLOWLIST_RAW)
    string(STRIP "${entry}" entry)
    if(entry STREQUAL "" OR entry MATCHES "^#")
      continue()
    endif()

    set(src "${C_TESTSUITE_ROOT}/${entry}")
    if(NOT EXISTS "${src}")
      message(WARNING "c-testsuite allowlist entry not found: ${entry}")
      continue()
    endif()

    string(REGEX REPLACE "[^A-Za-z0-9_]" "_" test_id "${entry}")
    set(test_name "c_testsuite_${test_id}")

    add_test(
      NAME "${test_name}"
      COMMAND "${CMAKE_COMMAND}"
              -DCOMPILER=$<TARGET_FILE:frontend_cxx_stage1>
              -DCLANG=${CLANG_EXECUTABLE}
              -DSRC=${src}
              -DROOT=${C_TESTSUITE_ROOT}
              -DOUT_LL=${CMAKE_BINARY_DIR}/c_testsuite/${entry}.ll
              -DOUT_BIN=${CMAKE_BINARY_DIR}/c_testsuite/${entry}.bin
              -P "${PROJECT_SOURCE_DIR}/tests/cmake/run_c_testsuite_case.cmake"
    )
    set_tests_properties("${test_name}" PROPERTIES LABELS "c_testsuite")
  endforeach()
endif()
