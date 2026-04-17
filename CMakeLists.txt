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


add_subdirectory(src/frontend)
add_subdirectory(src/codegen)
add_subdirectory(src/backend)

add_library(c4c_target_profile STATIC
    "${PROJECT_SOURCE_DIR}/src/target_profile.cpp"
)
target_include_directories(c4c_target_profile PUBLIC
    "${PROJECT_SOURCE_DIR}/src"
)
target_compile_features(c4c_target_profile PUBLIC cxx_std_17)

add_executable(c4cll
    "${PROJECT_SOURCE_DIR}/src/apps/c4cll.cpp"
)
target_link_libraries(c4cll PRIVATE c4c_frontend c4c_codegen c4c_backend)
set_target_properties(c4cll PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)




enable_testing()
add_subdirectory(tests)
