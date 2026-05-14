# ── Cache variables (all declared here for cmake-gui visibility) ──────────────

set(CTEST_CORE_PARALLEL_LEVEL "8" CACHE STRING
    "Parallel jobs for ctest_core target")
set(CTEST_CORE_TEST_TIMEOUT_SEC "30" CACHE STRING
    "Per-test timeout (seconds) for ctest_core target")

set(C_TESTSUITE_ROOT "" CACHE PATH "Path to vendored c-testsuite subset root")
set(C_CLANG_TEST_SUITE_ROOT "" CACHE PATH "Path to vendored external clang C subset root")
set(LLVM_TEST_SUITE_ROOT "" CACHE PATH "Path to vendored gcc torture subset root")
set(CPP_CLANG_TEST_SUITE_ROOT "" CACHE PATH "Path to vendored external clang C++ subset root")
set(CPP_EASTL_EXTERNAL_TEST_SUITE_ROOT "" CACHE PATH "Path to curated external EASTL C++ subset root")

set(LLVM_GCC_C_TORTURE_STEP_TIMEOUT_SEC "20" CACHE STRING
    "Per-step timeout (seconds) for llvm gcc-c-torture case script")
set(LLVM_GCC_C_TORTURE_TEST_TIMEOUT_SEC "90" CACHE STRING
    "CTest timeout (seconds) for a single llvm gcc-c-torture test")
set(LLVM_GCC_C_TORTURE_RUN_MEM_MB "0" CACHE STRING
    "Runtime virtual memory cap (MB) for test binaries; 0 disables cap")
set(LLVM_GCC_C_TORTURE_RUN_CPU_SEC "0" CACHE STRING
    "Runtime CPU time cap (seconds) for test binaries; 0 disables cap")

option(ENABLE_LLVM_GCC_C_TORTURE_TESTS
    "Register llvm gcc-c-torture execute tests (resource-heavy during migration)"
    ON)
option(ENABLE_C_CLANG_EXTERNAL_TESTS
    "Register curated external clang C subset tests"
    ON)
option(ENABLE_CPP_CLANG_EXTERNAL_TESTS
    "Register curated external clang C++ subset tests"
    ON)
option(ENABLE_CPP_EASTL_EXTERNAL_TESTS
    "Register curated external EASTL C++ subset tests"
    ON)
option(ENABLE_C_TESTSUITE_BACKEND_TESTS
    "Register c-testsuite backend execution tests such as x86_backend/aarch64_backend"
    OFF)
option(ENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN
    "Register explicit c-testsuite AArch64 backend scan tests independent of host backend selection"
    OFF)
set(C_TESTSUITE_AARCH64_BACKEND_RUNNER "" CACHE STRING
    "Optional command used to run AArch64 c-testsuite backend binaries on non-AArch64 hosts")

if(ENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN AND NOT ENABLE_C4C_BACKEND)
  message(FATAL_ERROR
    "ENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN requires ENABLE_C4C_BACKEND=ON "
    "so c4cll supports backend-native --codegen asm")
endif()

# ── Auto-detect vendored roots ────────────────────────────────────────────────

if(NOT C_TESTSUITE_ROOT AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/c/external/c-testsuite")
  set(C_TESTSUITE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/c/external/c-testsuite")
endif()
if(NOT C_CLANG_TEST_SUITE_ROOT AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/c/external/clang")
  set(C_CLANG_TEST_SUITE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/c/external/clang")
endif()
if(NOT LLVM_TEST_SUITE_ROOT AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/c/external/gcc_torture")
  set(LLVM_TEST_SUITE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/c/external/gcc_torture")
endif()
if(NOT CPP_CLANG_TEST_SUITE_ROOT AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cpp/external/clang")
  set(CPP_CLANG_TEST_SUITE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/cpp/external/clang")
endif()
if(NOT CPP_EASTL_EXTERNAL_TEST_SUITE_ROOT AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cpp/external/eastl")
  set(CPP_EASTL_EXTERNAL_TEST_SUITE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/cpp/external/eastl")
endif()

find_program(PYTHON3_EXECUTABLE NAMES python3)

# ── Utility build target ──────────────────────────────────────────────────────

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

if(PYTHON3_EXECUTABLE)
  add_custom_target(string_authority_guard
    COMMAND "${PYTHON3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/scripts/string_authority_guard.py"
    COMMAND "${PYTHON3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/scripts/test_string_authority_guard.py"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    COMMENT "Run the string authority guard and focused self-test"
    VERBATIM
  )

  add_test(
    NAME string_authority_guard
    COMMAND "${PYTHON3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/scripts/string_authority_guard.py"
  )
  set_tests_properties(string_authority_guard PROPERTIES
    LABELS "workflow;string_authority"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    TIMEOUT 30
  )

  add_test(
    NAME string_authority_guard_self_test
    COMMAND "${PYTHON3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/scripts/test_string_authority_guard.py"
  )
  set_tests_properties(string_authority_guard_self_test PROPERTIES
    LABELS "workflow;string_authority"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    TIMEOUT 30
  )
else()
  message(WARNING "python3 not found: skipping string authority guard workflow tests")
endif()

# ── Test suites ───────────────────────────────────────────────────────────────

add_subdirectory(frontend)
add_subdirectory(preprocessor)
if(ENABLE_C4C_BACKEND)
  add_subdirectory(backend)
endif()
add_subdirectory(c/internal)
add_subdirectory(cpp/internal)
add_subdirectory(c/external)
add_subdirectory(cpp/external)
