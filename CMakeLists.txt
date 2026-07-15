cmake_minimum_required(VERSION 3.20)
project(tiny_c2ll LANGUAGES CXX)

# All in-tree host tools, libraries, and native tests use the same strict C++20
# dialect. Source-language tests select their own modes independently.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CheckCXXSourceCompiles)

check_cxx_source_compiles(
  "static_assert(__cplusplus >= 202002L); int main() { return 0; }"
  C4C_HAS_CXX20_LANGUAGE_MODE
)
check_cxx_source_compiles(
  "struct Pair { int first; int second; }; int main() { Pair value{.first = 1, .second = 2}; return value.second - 2; }"
  C4C_HAS_CXX20_DESIGNATED_INITIALIZERS
)
check_cxx_source_compiles(
  "consteval int answer() { return 42; } static_assert(answer() == 42); int main() { return 0; }"
  C4C_HAS_CXX20_CONSTEVAL
)
check_cxx_source_compiles(
  "#include <span>\nint main() { int values[] = {1, 2}; std::span<int> view{values}; return view.size() == 2 ? 0 : 1; }"
  C4C_HAS_CXX20_STD_SPAN
)

set(C4C_MISSING_CXX20_CAPABILITIES)
if(NOT C4C_HAS_CXX20_LANGUAGE_MODE)
  list(APPEND C4C_MISSING_CXX20_CAPABILITIES "__cplusplus >= 202002L")
endif()
if(NOT C4C_HAS_CXX20_DESIGNATED_INITIALIZERS)
  list(APPEND C4C_MISSING_CXX20_CAPABILITIES "designated initialization")
endif()
if(NOT C4C_HAS_CXX20_CONSTEVAL)
  list(APPEND C4C_MISSING_CXX20_CAPABILITIES "consteval")
endif()
if(NOT C4C_HAS_CXX20_STD_SPAN)
  list(APPEND C4C_MISSING_CXX20_CAPABILITIES "std::span")
endif()
if(C4C_MISSING_CXX20_CAPABILITIES)
  list(JOIN C4C_MISSING_CXX20_CAPABILITIES ", " C4C_MISSING_CXX20_CAPABILITIES_TEXT)
  message(FATAL_ERROR
    "The selected host C++ compiler does not satisfy c4c's required C++20 contract. "
    "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}. "
    "Missing capabilities: ${C4C_MISSING_CXX20_CAPABILITIES_TEXT}. "
    "Select a compiler and standard library that provide the complete contract."
  )
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

option(ENABLE_HEAVY_TENTATIVE_SNAPSHOT
       "Use full parser-state snapshots in TentativeParseGuard speculative parses"
       ON)
option(ENABLE_C4C_BACKEND
       "Build experimental backend and backend-dependent routes"
       OFF)

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
if(ENABLE_C4C_BACKEND)
  add_subdirectory(src/backend)
endif()

add_library(c4c_target_profile STATIC
    "${PROJECT_SOURCE_DIR}/src/target_profile.cpp"
)
target_include_directories(c4c_target_profile PUBLIC
    "${PROJECT_SOURCE_DIR}/src"
)

add_executable(c4cll
    "${PROJECT_SOURCE_DIR}/src/apps/c4cll.cpp"
)
target_link_libraries(c4cll PRIVATE c4c_frontend c4c_codegen)
if(ENABLE_C4C_BACKEND)
  target_link_libraries(c4cll PRIVATE c4c_backend)
endif()
target_compile_definitions(c4cll PRIVATE C4C_ENABLE_BACKEND=$<BOOL:${ENABLE_C4C_BACKEND}>)

add_executable(c4c-objdump
    "${PROJECT_SOURCE_DIR}/src/apps/c4c-objdump.cpp"
)




enable_testing()
add_subdirectory(tests)
