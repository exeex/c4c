cmake_minimum_required(VERSION 3.20)
project(tiny_c2ll LANGUAGES CXX)

find_program(CLANG_EXECUTABLE NAMES clang)
if(NOT CLANG_EXECUTABLE)
  message(WARNING "clang not found: clang execution tests will be skipped")
endif()

set(EXAMPLE_C "${PROJECT_SOURCE_DIR}/tests/internal/example/example.c")
set(EXAMPLE_LL "${CMAKE_BINARY_DIR}/example.ll")

set(FRONTEND_CXX_COMMON_SRCS
    "${PROJECT_SOURCE_DIR}/src/frontend/arena.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/lexer/token.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/lexer/lexer.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/sema/type_utils.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/sema/sema.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/sema/validate.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/sema/consteval.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/preprocessor/preprocessor.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/parser/parse.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/parser/types.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/parser/expressions.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/parser/statements.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/parser/declarations.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/parser/common.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/hir/ast_to_hir.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/hir/hir_printer.cpp"
    "${PROJECT_SOURCE_DIR}/src/codegen/llvm/llvm_codegen.cpp"
    "${PROJECT_SOURCE_DIR}/src/codegen/llvm/hir_emitter.cpp"
    "${PROJECT_SOURCE_DIR}/src/codegen/llvm/hir_to_llvm_const_init.cpp"
)

set(FRONTEND_CXX_INCLUDE_DIRS
    "${PROJECT_SOURCE_DIR}/src/frontend"
    "${PROJECT_SOURCE_DIR}/src/frontend/lexer"
    "${PROJECT_SOURCE_DIR}/src/frontend/parser"
    "${PROJECT_SOURCE_DIR}/src/frontend/sema"
    "${PROJECT_SOURCE_DIR}/src/frontend/hir"
    "${PROJECT_SOURCE_DIR}/src/frontend/preprocessor"
    "${PROJECT_SOURCE_DIR}/src/codegen/llvm"
)

add_executable(c4cll
    "${PROJECT_SOURCE_DIR}/src/apps/c4cll.cpp"
    ${FRONTEND_CXX_COMMON_SRCS}
)
target_include_directories(c4cll PRIVATE
    ${FRONTEND_CXX_INCLUDE_DIRS}
)
set_target_properties(c4cll PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

add_executable(frontend_cxx_preprocessor_tests
    "${PROJECT_SOURCE_DIR}/tests/preprocessor/frontend_cxx_preprocessor_tests.cpp"
    "${PROJECT_SOURCE_DIR}/src/frontend/preprocessor/preprocessor.cpp"
)
target_include_directories(frontend_cxx_preprocessor_tests PRIVATE
    "${PROJECT_SOURCE_DIR}/src/frontend/preprocessor"
)
set_target_properties(frontend_cxx_preprocessor_tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

add_custom_command(
  OUTPUT "${EXAMPLE_LL}"
  COMMAND $<TARGET_FILE:c4cll> "${EXAMPLE_C}" -o "${EXAMPLE_LL}"
  DEPENDS c4cll "${EXAMPLE_C}"
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

include("${PROJECT_SOURCE_DIR}/tests/TestEntry.cmake")
