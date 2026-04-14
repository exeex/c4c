cmake_minimum_required(VERSION 3.20)
project(tiny_c2ll LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

option(ENABLE_HEAVY_TENTATIVE_SNAPSHOT
       "Use full parser-state snapshots in TentativeParseGuard speculative parses"
       ON)

find_program(CLANG_EXECUTABLE NAMES clang)
if(NOT CLANG_EXECUTABLE)
  message(WARNING "clang not found: clang execution tests will be skipped")
endif()

find_program(OBJDUMP_EXECUTABLE NAMES llvm-objdump objdump gobjdump)
if(NOT OBJDUMP_EXECUTABLE)
  message(WARNING "objdump not found: backend object contract tests will be skipped")
endif()

set(EXAMPLE_C "${PROJECT_SOURCE_DIR}/tests/c/internal/example/example.c")
set(EXAMPLE_LL "${CMAKE_BINARY_DIR}/example.ll")

add_subdirectory(src/frontend)
add_subdirectory(src/codegen)

set(BACKEND_LOWERING_BOOTSTRAP_SRCS
    "${PROJECT_SOURCE_DIR}/src/backend/target.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/backend.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/bir.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/bir_printer.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/bir_validate.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/lowering/call_decode.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/lowering/lir_to_bir.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/lowering/lir_to_bir_context.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/lowering/lir_to_bir_analysis.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/lowering/lir_to_bir_module.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/prepare/prepare.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/prepare/legalize.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/prepare/stack_layout.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/prepare/liveness.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/prepare/regalloc.cpp"
)

set(C4C_BACKEND_INCLUDE_DIRS
    "${PROJECT_SOURCE_DIR}/src/backend"
    "${PROJECT_SOURCE_DIR}/src/backend/aarch64"
)

add_executable(c4cll
    "${PROJECT_SOURCE_DIR}/src/apps/c4cll.cpp"
    ${BACKEND_LOWERING_BOOTSTRAP_SRCS}
)
target_include_directories(c4cll PRIVATE ${C4C_BACKEND_INCLUDE_DIRS})
target_link_libraries(c4cll PRIVATE c4c_frontend c4c_codegen)
set_target_properties(c4cll PROPERTIES
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

  add_custom_target(eastl_type_traits_simple_workflow
    COMMAND "${CMAKE_COMMAND}"
            -DCOMPILER=$<TARGET_FILE:c4cll>
            -DCLANG=${CLANG_EXECUTABLE}
            -DCXX_COMPILER=${CMAKE_CXX_COMPILER}
            -DSRC=${PROJECT_SOURCE_DIR}/tests/cpp/eastl/eastl_type_traits_simple.cpp
            -DROOT=${PROJECT_SOURCE_DIR}
            -DBINARY_DIR=${CMAKE_BINARY_DIR}
            -P "${PROJECT_SOURCE_DIR}/tests/cpp/eastl/run_eastl_type_traits_simple_workflow.cmake"
    DEPENDS c4cll
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    COMMENT "Run standalone EASTL type_traits workflow"
    VERBATIM
  )

  add_custom_target(eastl_vector_simple_parse_recipe
    COMMAND "${CMAKE_COMMAND}"
            -DCOMPILER=$<TARGET_FILE:c4cll>
            -DROOT=${PROJECT_SOURCE_DIR}
            -DSRC=${PROJECT_SOURCE_DIR}/tests/cpp/eastl/eastl_vector_simple.cpp
            "-DEXPECT_FAIL_LOC:STRING=ref/EASTL/include/EASTL/internal/type_properties.h:35:52"
            "-DEXPECT_ERROR_SUBSTRING:STRING=parse_fn=try_parse_record_typedef_member phase=committed expected=SEMI got='('"
            -P "${PROJECT_SOURCE_DIR}/tests/cpp/eastl/run_eastl_vector_parse_recipe.cmake"
    DEPENDS c4cll
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    COMMENT "Run standalone EASTL vector parse recipe"
    VERBATIM
  )

  add_custom_target(std_vector_simple_parse_baseline
    COMMAND "${CMAKE_COMMAND}"
            -DCOMPILER=$<TARGET_FILE:c4cll>
            -DSRC=${PROJECT_SOURCE_DIR}/tests/cpp/std/std_vector_simple.cpp
            -DEXPECT_TIMEOUT_SEC=60
            -P "${PROJECT_SOURCE_DIR}/tests/cpp/std/run_std_vector_parse_baseline.cmake"
    DEPENDS c4cll
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    COMMENT "Run standalone std::vector parse baseline"
    VERBATIM
  )
endif()

enable_testing()
add_subdirectory(tests)
