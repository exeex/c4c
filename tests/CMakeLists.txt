# ── Cache variables (all declared here for cmake-gui visibility) ──────────────

set(CTEST_CORE_PARALLEL_LEVEL "8" CACHE STRING
    "Parallel jobs for ctest_core target")
set(CTEST_CORE_TEST_TIMEOUT_SEC "30" CACHE STRING
    "Per-test timeout (seconds) for ctest_core target")

set(C_TESTSUITE_ROOT "" CACHE PATH "Path to vendored c-testsuite subset root")
set(C_CLANG_TEST_SUITE_ROOT "" CACHE PATH "Path to vendored external clang C subset root")
set(LLVM_TEST_SUITE_ROOT "" CACHE PATH "Path to vendored gcc torture subset root")
set(CPP_CLANG_TEST_SUITE_ROOT "" CACHE PATH "Path to vendored external clang C++ subset root")

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

# ── Test suites ───────────────────────────────────────────────────────────────

add_subdirectory(frontend)
add_subdirectory(preprocessor)
add_subdirectory(backend)
add_subdirectory(c/internal)
add_subdirectory(cpp/internal)
add_subdirectory(c/external)
add_subdirectory(cpp/external)
