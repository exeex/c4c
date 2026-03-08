cmake_minimum_required(VERSION 3.20)
project(tiny_c2ll LANGUAGES CXX)

find_program(CLANG_EXECUTABLE NAMES clang)
if(NOT CLANG_EXECUTABLE)
  message(WARNING "clang not found: clang execution tests will be skipped")
endif()

set(CTEST_CORE_PARALLEL_LEVEL "8" CACHE STRING
    "Parallel jobs for ctest_core target")
set(CTEST_CORE_TEST_TIMEOUT_SEC "30" CACHE STRING
    "Per-test timeout (seconds) for ctest_core target")

add_custom_target(ctest_core
  COMMAND "${CMAKE_CTEST_COMMAND}"
          --output-on-failure
          --progress
          -j "${CTEST_CORE_PARALLEL_LEVEL}"
          --timeout "${CTEST_CORE_TEST_TIMEOUT_SEC}"
          -E "llvm_gcc_c_torture"
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  COMMENT "Run core test set (exclude llvm_gcc_c_torture*, with progress/timeout)"
  VERBATIM
)

set(C_TESTSUITE_ROOT "" CACHE PATH "Path to external c-testsuite root")
if(NOT C_TESTSUITE_ROOT AND EXISTS "${PROJECT_SOURCE_DIR}/tests/c-testsuite")
  set(C_TESTSUITE_ROOT "${PROJECT_SOURCE_DIR}/tests/c-testsuite")
endif()

set(LLVM_TEST_SUITE_ROOT "" CACHE PATH "Path to external llvm-test-suite root")
if(NOT LLVM_TEST_SUITE_ROOT AND EXISTS "${PROJECT_SOURCE_DIR}/tests/llvm-test-suite")
  set(LLVM_TEST_SUITE_ROOT "${PROJECT_SOURCE_DIR}/tests/llvm-test-suite")
endif()

set(EXAMPLE_C "${PROJECT_SOURCE_DIR}/tests/tiny_c2ll/example.c")
set(EXAMPLE_LL "${CMAKE_BINARY_DIR}/example.ll")
file(GLOB NEGATIVE_TEST_SRCS CONFIGURE_DEPENDS
     "${PROJECT_SOURCE_DIR}/tests/tiny_c2ll/bad_*.c")
option(NEGATIVE_TESTS_ENFORCE "Require negative_tests bad_*.c cases to fail compilation (non-zero exit)" ON)

set(FRONTEND_CXX_COMMON_SRCS
    "${PROJECT_SOURCE_DIR}/src/frontend/lexer/token.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/lexer/lexer.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/sema/arena.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/sema/sema_driver.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/sema/sema_validate.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/preprocessor/preprocessor.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/parser/parse.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/parser/types.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/parser/expressions.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/parser/statements.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/parser/declarations.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/parser/common.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/sema/ir_builder.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/sema/ast_to_hir.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/sema/hir_printer.cpp"
    "${PROJECT_SOURCE_DIR}/src/codegen/llvm/hir_to_llvm.cpp"
)

set(FRONTEND_CXX_INCLUDE_DIRS
    "${PROJECT_SOURCE_DIR}/src/frontend"
    "${PROJECT_SOURCE_DIR}/src/frontend/lexer"
    "${PROJECT_SOURCE_DIR}/src/frontend/parser"
    "${PROJECT_SOURCE_DIR}/src/frontend/sema"
    "${PROJECT_SOURCE_DIR}/src/frontend/preprocessor"
    "${PROJECT_SOURCE_DIR}/src/codegen/llvm"
)

# Temporarily disabled: keep a single compiler executable target during migration.
# add_executable(frontend_cxx_stage1
#     "${PROJECT_SOURCE_DIR}/src/frontend/driver.cpp"
#     ${FRONTEND_CXX_COMMON_SRCS}
# )
# target_include_directories(frontend_cxx_stage1 PRIVATE
#     ${FRONTEND_CXX_INCLUDE_DIRS}
# )
# set_target_properties(frontend_cxx_stage1 PROPERTIES
#     CXX_STANDARD 17
#     CXX_STANDARD_REQUIRED ON
#     OUTPUT_NAME "tiny-c2ll-stage1"
# )
# target_compile_options(frontend_cxx_stage1 PRIVATE -Wall -Wextra -Wpedantic)

add_executable(frontend_cxx_next
    "${PROJECT_SOURCE_DIR}/src/apps/tiny-c2ll-next.cpp"
    ${FRONTEND_CXX_COMMON_SRCS}
)
target_include_directories(frontend_cxx_next PRIVATE
    ${FRONTEND_CXX_INCLUDE_DIRS}
)
set_target_properties(frontend_cxx_next PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    OUTPUT_NAME "tiny-c2ll-next"
)
target_compile_options(frontend_cxx_next PRIVATE -Wall -Wextra -Wpedantic)

# Temporarily disabled: keep a single compiler executable target during migration.
# add_executable(frontend_cxx_stage2
#     "${PROJECT_SOURCE_DIR}/src/frontend/driver_stage2.cpp"
# )
# set_target_properties(frontend_cxx_stage2 PROPERTIES
#     CXX_STANDARD 17
#     CXX_STANDARD_REQUIRED ON
#     OUTPUT_NAME "tiny-c2ll-stage2"
# )
# target_compile_options(frontend_cxx_stage2 PRIVATE -Wall -Wextra -Wpedantic)

# Temporarily disabled: keep a single compiler executable target during migration.
# add_executable(frontend_cxx_preprocessor_tests
#     "${PROJECT_SOURCE_DIR}/tests/preprocessor/frontend_cxx_preprocessor_tests.cpp"
#     "${PROJECT_SOURCE_DIR}/src/frontend/preprocessor/preprocessor.cpp"
# )
# target_include_directories(frontend_cxx_preprocessor_tests PRIVATE
#     "${PROJECT_SOURCE_DIR}/src/frontend"
#     "${PROJECT_SOURCE_DIR}/src/frontend/lexer"
#     "${PROJECT_SOURCE_DIR}/src/frontend/parser"
#     "${PROJECT_SOURCE_DIR}/src/frontend/sema"
#     "${PROJECT_SOURCE_DIR}/src/frontend/preprocessor"
# )
# set_target_properties(frontend_cxx_preprocessor_tests PROPERTIES
#     CXX_STANDARD 17
#     CXX_STANDARD_REQUIRED ON
# )
# target_compile_options(frontend_cxx_preprocessor_tests PRIVATE -Wall -Wextra -Wpedantic)

add_custom_command(
  OUTPUT "${EXAMPLE_LL}"
  COMMAND $<TARGET_FILE:frontend_cxx_next> "${EXAMPLE_C}" -o "${EXAMPLE_LL}"
  DEPENDS frontend_cxx_next "${EXAMPLE_C}"
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
            -DCOMPILER=$<TARGET_FILE:frontend_cxx_next>
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
            -DCOMPILER=$<TARGET_FILE:frontend_cxx_next>
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
            -DCOMPILER=$<TARGET_FILE:frontend_cxx_next>
            -DSRC=${src}
            -DENFORCE_NEGATIVE=${NEGATIVE_TESTS_ENFORCE}
            -P "${PROJECT_SOURCE_DIR}/tests/cmake/run_tiny_c2ll_negative_case.cmake"
  )
  set_tests_properties("${test_name}" PROPERTIES LABELS "negative_tests")
endforeach()

# Temporarily disabled with frontend_cxx_preprocessor_tests target.
# add_test(
#     NAME frontend_cxx_preprocessor_tests
#     COMMAND frontend_cxx_preprocessor_tests
# )

add_test(
    NAME frontend_cxx_stage1_version
    COMMAND frontend_cxx_next --version
)
set_tests_properties(frontend_cxx_stage1_version PROPERTIES
    PASS_REGULAR_EXPRESSION "tiny-c2ll"
)

if(EXISTS "${EXAMPLE_C}")
  add_test(
      NAME frontend_cxx_stage1_lex
      COMMAND frontend_cxx_next --lex-only "${EXAMPLE_C}"
  )
  set_tests_properties(frontend_cxx_stage1_lex PROPERTIES
      PASS_REGULAR_EXPRESSION "EOF"
  )
endif()

if(EXISTS "${EXAMPLE_C}")
  add_test(
      NAME frontend_cxx_stage1_parse
      COMMAND frontend_cxx_next --parse-only "${EXAMPLE_C}"
  )
  set_tests_properties(frontend_cxx_stage1_parse PROPERTIES
      PASS_REGULAR_EXPRESSION "Program"
  )
endif()

if(EXISTS "${EXAMPLE_C}")
  add_test(
      NAME frontend_cxx_stage1_emit_ir
      COMMAND frontend_cxx_next "${EXAMPLE_C}"
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
              -DCOMPILER=$<TARGET_FILE:frontend_cxx_next>
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
              -DCOMPILER=$<TARGET_FILE:frontend_cxx_next>
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

if(CLANG_EXECUTABLE AND LLVM_TEST_SUITE_ROOT AND EXISTS "${LLVM_TEST_SUITE_ROOT}")
  set(LLVM_GCC_C_TORTURE_ROOT
      "${LLVM_TEST_SUITE_ROOT}/SingleSource/Regression/C/gcc-c-torture/execute")
  set(LLVM_GCC_C_TORTURE_STEP_TIMEOUT_SEC "20" CACHE STRING
      "Per-step timeout (seconds) for llvm gcc-c-torture case script")
  set(LLVM_GCC_C_TORTURE_TEST_TIMEOUT_SEC "90" CACHE STRING
      "CTest timeout (seconds) for a single llvm gcc-c-torture test")
  set(LLVM_GCC_C_TORTURE_RUN_MEM_MB "0" CACHE STRING
      "Runtime virtual memory cap (MB) for test binaries; 0 disables cap")
  set(LLVM_GCC_C_TORTURE_RUN_CPU_SEC "0" CACHE STRING
      "Runtime CPU time cap (seconds) for test binaries; 0 disables cap")

  if(EXISTS "${LLVM_GCC_C_TORTURE_ROOT}" AND
     EXISTS "${PROJECT_SOURCE_DIR}/tests/llvm_gcc_c_torture_allowlist.txt")
    file(STRINGS "${PROJECT_SOURCE_DIR}/tests/llvm_gcc_c_torture_allowlist.txt"
         LLVM_GCC_C_TORTURE_ALLOWLIST_RAW)
    set(LLVM_GCC_C_TORTURE_TEST_MAP
        "${CMAKE_BINARY_DIR}/llvm_gcc_c_torture_test_map.tsv")
    file(WRITE "${LLVM_GCC_C_TORTURE_TEST_MAP}"
         "# test_name\\tallowlist_entry\n")
    foreach(entry IN LISTS LLVM_GCC_C_TORTURE_ALLOWLIST_RAW)
      string(STRIP "${entry}" entry)
      if(entry STREQUAL "" OR entry MATCHES "^#")
        continue()
      endif()

      set(src "${LLVM_GCC_C_TORTURE_ROOT}/${entry}")
      if(NOT EXISTS "${src}")
        message(WARNING "llvm gcc-c-torture allowlist entry not found: ${entry}")
        continue()
      endif()

      string(REGEX REPLACE "[^A-Za-z0-9_]" "_" test_id "${entry}")
      set(test_name "llvm_gcc_c_torture_${test_id}")

      add_test(
        NAME "${test_name}"
        COMMAND "${CMAKE_COMMAND}"
                -DCOMPILER=$<TARGET_FILE:frontend_cxx_next>
                -DCLANG=${CLANG_EXECUTABLE}
                -DSRC=${src}
                -DROOT=${LLVM_GCC_C_TORTURE_ROOT}
                -DOUT_LL=${CMAKE_BINARY_DIR}/llvm_gcc_c_torture/${entry}.ll
                -DOUT_CLANG_BIN=${CMAKE_BINARY_DIR}/llvm_gcc_c_torture/${entry}.clang.bin
                -DOUT_C2LL_BIN=${CMAKE_BINARY_DIR}/llvm_gcc_c_torture/${entry}.c2ll.bin
                -DCASE_TIMEOUT_SEC=${LLVM_GCC_C_TORTURE_STEP_TIMEOUT_SEC}
                -DRUN_MEM_MB=${LLVM_GCC_C_TORTURE_RUN_MEM_MB}
                -DRUN_CPU_SEC=${LLVM_GCC_C_TORTURE_RUN_CPU_SEC}
                -P "${PROJECT_SOURCE_DIR}/tests/cmake/run_llvm_gcc_c_torture_case.cmake"
      )
      set_tests_properties("${test_name}" PROPERTIES
        LABELS "llvm_gcc_c_torture"
        TIMEOUT "${LLVM_GCC_C_TORTURE_TEST_TIMEOUT_SEC}")
      file(APPEND "${LLVM_GCC_C_TORTURE_TEST_MAP}" "${test_name}\t${entry}\n")
    endforeach()

    add_custom_target(prune_llvm_gcc_c_torture_allowlist_to_failed
      COMMAND "${CMAKE_COMMAND}"
              -DTEST_MAP=${LLVM_GCC_C_TORTURE_TEST_MAP}
              -DLAST_FAILED=${CMAKE_BINARY_DIR}/Testing/Temporary/LastTestsFailed.log
              -DALLOWLIST=${PROJECT_SOURCE_DIR}/tests/llvm_gcc_c_torture_allowlist.txt
              -P "${PROJECT_SOURCE_DIR}/tests/cmake/prune_llvm_gcc_c_torture_allowlist_to_failed.cmake"
      COMMENT "Rewrite tests/llvm_gcc_c_torture_allowlist.txt to keep only failed cases from last ctest run"
      VERBATIM
    )
  endif()
endif()

set(DEFAULT_TEST_TIMEOUT_SEC "30" CACHE STRING
    "Default CTest timeout (seconds) applied to all registered tests")
get_property(ALL_REGISTERED_TESTS DIRECTORY PROPERTY TESTS)
if(ALL_REGISTERED_TESTS)
  foreach(test_name IN LISTS ALL_REGISTERED_TESTS)
    set_property(TEST "${test_name}" PROPERTY TIMEOUT "${DEFAULT_TEST_TIMEOUT_SEC}")
  endforeach()
endif()
