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

set(C2LL_SCRIPT "${PROJECT_SOURCE_DIR}/src/frontend/c2ll.py")
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

if(CLANG_EXECUTABLE)
  add_test(
    NAME ccc_review_tests
    COMMAND "${PYTHON_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tests/run_ccc_review_tests.py"
            --compiler "${C2LL_SCRIPT}"
            --clang "${CLANG_EXECUTABLE}"
            --tests-root "${PROJECT_SOURCE_DIR}/tests/ccc-review-tests"
            --workdir "${CMAKE_BINARY_DIR}"
  )
endif()

if(C_TESTSUITE_ROOT AND EXISTS "${C_TESTSUITE_ROOT}")
  add_test(
    NAME c_testsuite_allowlist
    COMMAND "${PYTHON_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tests/run_c_testsuite.py"
            --compiler "${C2LL_SCRIPT}"
            --clang "${CLANG_EXECUTABLE}"
            --testsuite-root "${C_TESTSUITE_ROOT}"
            --workdir "${CMAKE_BINARY_DIR}"
            --allowlist "${PROJECT_SOURCE_DIR}/tests/c_testsuite_allowlist.txt"
  )
endif()

# ============================================================
# C Frontend (stage1) – compiled with system clang/cc
# M0: skeleton + CMake target
# M1/M2: lexer + parser
# ============================================================
enable_language(C)

set(FRONTEND_C_SRCS
    "${PROJECT_SOURCE_DIR}/src/frontend_c/driver.c"
    "${PROJECT_SOURCE_DIR}/src/frontend_c/lexer.c"
    "${PROJECT_SOURCE_DIR}/src/frontend_c/arena.c"
    "${PROJECT_SOURCE_DIR}/src/frontend_c/parser.c"
    "${PROJECT_SOURCE_DIR}/src/frontend_c/ir_builder.c"
)

add_executable(frontend_c_stage1 ${FRONTEND_C_SRCS})
target_include_directories(frontend_c_stage1 PRIVATE
    "${PROJECT_SOURCE_DIR}/src/frontend_c"
)
set_target_properties(frontend_c_stage1 PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
    OUTPUT_NAME "tiny-c2ll-stage1"
)
target_compile_options(frontend_c_stage1 PRIVATE -Wall -Wextra -Wpedantic)

# Smoke test: --version exits 0
add_test(
    NAME frontend_c_stage1_version
    COMMAND frontend_c_stage1 --version
)
set_tests_properties(frontend_c_stage1_version PROPERTIES
    PASS_REGULAR_EXPRESSION "tiny-c2ll"
)

# Smoke test: --lex-only on the example file produces token output
if(EXISTS "${EXAMPLE_C}")
  add_test(
      NAME frontend_c_stage1_lex
      COMMAND frontend_c_stage1 --lex-only "${EXAMPLE_C}"
  )
  set_tests_properties(frontend_c_stage1_lex PROPERTIES
      PASS_REGULAR_EXPRESSION "EOF"
  )
endif()

# Smoke test: --parse-only on the example file produces AST output
if(EXISTS "${EXAMPLE_C}")
  add_test(
      NAME frontend_c_stage1_parse
      COMMAND frontend_c_stage1 --parse-only "${EXAMPLE_C}"
  )
  set_tests_properties(frontend_c_stage1_parse PROPERTIES
      PASS_REGULAR_EXPRESSION "Program"
  )
endif()

# Smoke test: full IR emission on the example file produces valid LLVM IR (stdout)
if(EXISTS "${EXAMPLE_C}")
  add_test(
      NAME frontend_c_stage1_emit_ir
      COMMAND frontend_c_stage1 "${EXAMPLE_C}"
  )
  set_tests_properties(frontend_c_stage1_emit_ir PROPERTIES
      PASS_REGULAR_EXPRESSION "define"
  )
endif()
