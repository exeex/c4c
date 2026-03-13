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

set(C_TESTSUITE_ROOT "" CACHE PATH "Path to vendored c-testsuite subset root")
if(NOT C_TESTSUITE_ROOT AND EXISTS "${PROJECT_SOURCE_DIR}/tests/external/c-testsuite")
  set(C_TESTSUITE_ROOT "${PROJECT_SOURCE_DIR}/tests/external/c-testsuite")
endif()

set(LLVM_TEST_SUITE_ROOT "" CACHE PATH "Path to vendored gcc torture subset root")
if(NOT LLVM_TEST_SUITE_ROOT AND EXISTS "${PROJECT_SOURCE_DIR}/tests/external/gcc_torture")
  set(LLVM_TEST_SUITE_ROOT "${PROJECT_SOURCE_DIR}/tests/external/gcc_torture")
endif()

option(ENABLE_LLVM_GCC_C_TORTURE_TESTS
    "Register llvm gcc-c-torture execute tests (resource-heavy during migration)"
    ON)

enable_testing()

include("${PROJECT_SOURCE_DIR}/tests/internal/InternalTests.cmake")

if(CLANG_EXECUTABLE AND C_TESTSUITE_ROOT AND EXISTS "${C_TESTSUITE_ROOT}")
  set(C_TESTSUITE_ALLOWLIST "${C_TESTSUITE_ROOT}/allowlist.txt")
  file(STRINGS "${C_TESTSUITE_ALLOWLIST}" C_TESTSUITE_ALLOWLIST_RAW)
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
              -DCOMPILER=$<TARGET_FILE:c4cll>
              -DCLANG=${CLANG_EXECUTABLE}
              -DSRC=${src}
              -DROOT=${C_TESTSUITE_ROOT}
              -DOUT_LL=${CMAKE_BINARY_DIR}/c_testsuite/${entry}.ll
              -DOUT_BIN=${CMAKE_BINARY_DIR}/c_testsuite/${entry}.bin
              -P "${PROJECT_SOURCE_DIR}/tests/external/c-testsuite/RunCase.cmake"
    )
    set_tests_properties("${test_name}" PROPERTIES LABELS "c_testsuite")
  endforeach()
endif()

if(ENABLE_LLVM_GCC_C_TORTURE_TESTS AND
   CLANG_EXECUTABLE AND LLVM_TEST_SUITE_ROOT AND EXISTS "${LLVM_TEST_SUITE_ROOT}")
  set(LLVM_GCC_C_TORTURE_ROOT "${LLVM_TEST_SUITE_ROOT}")
  set(LLVM_GCC_C_TORTURE_ALLOWLIST "${LLVM_TEST_SUITE_ROOT}/allowlist.txt")
  set(LLVM_GCC_C_TORTURE_STEP_TIMEOUT_SEC "20" CACHE STRING
      "Per-step timeout (seconds) for llvm gcc-c-torture case script")
  set(LLVM_GCC_C_TORTURE_TEST_TIMEOUT_SEC "90" CACHE STRING
      "CTest timeout (seconds) for a single llvm gcc-c-torture test")
  set(LLVM_GCC_C_TORTURE_RUN_MEM_MB "0" CACHE STRING
      "Runtime virtual memory cap (MB) for test binaries; 0 disables cap")
  set(LLVM_GCC_C_TORTURE_RUN_CPU_SEC "0" CACHE STRING
      "Runtime CPU time cap (seconds) for test binaries; 0 disables cap")

  if(EXISTS "${LLVM_GCC_C_TORTURE_ROOT}" AND
     EXISTS "${LLVM_GCC_C_TORTURE_ALLOWLIST}")
    file(STRINGS "${LLVM_GCC_C_TORTURE_ALLOWLIST}"
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
                -DCOMPILER=$<TARGET_FILE:c4cll>
                -DCLANG=${CLANG_EXECUTABLE}
                -DSRC=${src}
                -DROOT=${LLVM_GCC_C_TORTURE_ROOT}
                -DOUT_LL=${CMAKE_BINARY_DIR}/llvm_gcc_c_torture/${entry}.ll
                -DOUT_CLANG_BIN=${CMAKE_BINARY_DIR}/llvm_gcc_c_torture/${entry}.clang.bin
                -DOUT_C2LL_BIN=${CMAKE_BINARY_DIR}/llvm_gcc_c_torture/${entry}.c2ll.bin
                -DCASE_TIMEOUT_SEC=${LLVM_GCC_C_TORTURE_STEP_TIMEOUT_SEC}
                -DRUN_MEM_MB=${LLVM_GCC_C_TORTURE_RUN_MEM_MB}
                -DRUN_CPU_SEC=${LLVM_GCC_C_TORTURE_RUN_CPU_SEC}
                -P "${PROJECT_SOURCE_DIR}/tests/external/gcc_torture/RunCase.cmake"
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
              -DALLOWLIST=${LLVM_GCC_C_TORTURE_ALLOWLIST}
              -P "${PROJECT_SOURCE_DIR}/tests/external/gcc_torture/PruneAllowlistToFailed.cmake"
      COMMENT "Rewrite tests/external/gcc_torture/allowlist.txt to keep only failed cases from last ctest run"
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
