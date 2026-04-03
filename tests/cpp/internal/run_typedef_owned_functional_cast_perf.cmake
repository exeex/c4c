cmake_minimum_required(VERSION 3.20)

foreach(v COMPILER BINARY_DIR REPEAT_COUNT EXPECT_TIMEOUT_SEC)
  if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
    message(FATAL_ERROR "Missing required -D${v}=...")
  endif()
endforeach()

file(MAKE_DIRECTORY "${BINARY_DIR}/tmp")
set(src "${BINARY_DIR}/tmp/typedef_owned_functional_cast_perf.cpp")

file(WRITE "${src}" "// Generated parser perf guard\n")
file(APPEND "${src}" "template <class T> struct box {\n")
file(APPEND "${src}" "  typedef T value_type;\n")

math(EXPR repeat_last "${REPEAT_COUNT} - 1")
foreach(i RANGE 0 ${repeat_last})
  file(APPEND "${src}"
    "  static constexpr value_type f${i}() { return value_type(); }\n")
  file(APPEND "${src}"
    "  static constexpr value_type g${i}() { return static_cast<value_type>(0); }\n")
endforeach()

file(APPEND "${src}" "};\n")
file(APPEND "${src}" "int main() { return 0; }\n")

execute_process(
  COMMAND "${COMPILER}" --parse-only "${src}"
  TIMEOUT "${EXPECT_TIMEOUT_SEC}"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
)

if(NOT rc EQUAL 0)
  message(FATAL_ERROR
    "[FAIL] typedef-owned functional cast perf guard failed\n"
    "rc=${rc}\n"
    "stdout:\n${out}\n"
    "stderr:\n${err}")
endif()

if(NOT "${err}" STREQUAL "")
  message(FATAL_ERROR
    "[FAIL] expected no stderr from typedef-owned functional cast perf guard\n"
    "stderr:\n${err}")
endif()

message(STATUS "[PASS] typedef-owned functional cast perf guard")
message(STATUS "  source: ${src}")
message(STATUS "  repeats: ${REPEAT_COUNT}")
message(STATUS "  timeout budget: ${EXPECT_TIMEOUT_SEC}s")
