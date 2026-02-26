cmake_minimum_required(VERSION 3.20)
project(tiny_c2ll LANGUAGES NONE)

find_program(PYTHON314_EXECUTABLE NAMES python3.14)
find_program(PYTHON3_EXECUTABLE NAMES python3)

if(PYTHON314_EXECUTABLE)
  set(PYTHON_EXECUTABLE "${PYTHON314_EXECUTABLE}")
elseif(PYTHON3_EXECUTABLE)
  set(PYTHON_EXECUTABLE "${PYTHON3_EXECUTABLE}")
else()
  message(FATAL_ERROR "Python 3 is required")
endif()

find_program(CLANG_EXECUTABLE NAMES clang)
if(NOT CLANG_EXECUTABLE)
  message(WARNING "clang not found: clang execution tests will be skipped")
endif()

set(C2LL_SCRIPT "${PROJECT_SOURCE_DIR}/src/c2ll.py")
set(EXAMPLE_C "${PROJECT_SOURCE_DIR}/tests/example.c")
set(EXAMPLE_LL "${CMAKE_BINARY_DIR}/example.ll")
set(C_TESTSUITE_ROOT "" CACHE PATH "Path to external c-testsuite root")
if(NOT C_TESTSUITE_ROOT AND EXISTS "${PROJECT_SOURCE_DIR}/tests/c-testsuite")
  set(C_TESTSUITE_ROOT "${PROJECT_SOURCE_DIR}/tests/c-testsuite")
endif()

add_custom_command(
  OUTPUT "${EXAMPLE_LL}"
  COMMAND "${PYTHON_EXECUTABLE}" "${C2LL_SCRIPT}" "${EXAMPLE_C}" -o "${EXAMPLE_LL}"
  DEPENDS "${C2LL_SCRIPT}" "${EXAMPLE_C}"
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
add_test(
  NAME tiny_c2ll_tests
  COMMAND "${PYTHON_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tests/run_tests.py"
          --compiler "${C2LL_SCRIPT}"
          --workdir "${CMAKE_BINARY_DIR}"
          --clang "${CLANG_EXECUTABLE}"
)

if(C_TESTSUITE_ROOT AND EXISTS "${C_TESTSUITE_ROOT}")
  add_test(
    NAME c_testsuite_allowlist
    COMMAND "${PYTHON_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tests/run_c_testsuite.py"
            --compiler "${C2LL_SCRIPT}"
            --testsuite-root "${C_TESTSUITE_ROOT}"
            --workdir "${CMAKE_BINARY_DIR}"
            --allowlist "${PROJECT_SOURCE_DIR}/tests/c_testsuite_allowlist.txt"
  )
endif()
