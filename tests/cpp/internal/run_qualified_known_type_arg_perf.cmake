cmake_minimum_required(VERSION 3.20)

foreach(v COMPILER BINARY_DIR REPEAT_COUNT EXPECT_TIMEOUT_SEC)
  if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
    message(FATAL_ERROR "Missing required -D${v}=...")
  endif()
endforeach()

file(MAKE_DIRECTORY "${BINARY_DIR}/tmp")
set(src "${BINARY_DIR}/tmp/qualified_known_type_arg_perf.cpp")

file(WRITE "${src}" "// Generated parser perf guard\n")
file(APPEND "${src}" "namespace ns {\n")
file(APPEND "${src}" "template <class T> struct id { using type = T; };\n")
file(APPEND "${src}" "template <class T> using id_t = typename id<T>::type;\n")
file(APPEND "${src}" "template <class... Ts> struct pack {};\n")
file(APPEND "${src}" "} // namespace ns\n")
file(APPEND "${src}" "using big_pack = ns::pack<\n")

math(EXPR repeat_last "${REPEAT_COUNT} - 1")
foreach(i RANGE 0 ${repeat_last})
  if(i GREATER 0)
    file(APPEND "${src}" ",\n")
  endif()
  file(APPEND "${src}" "  ns::id_t<int>")
endforeach()

file(APPEND "${src}" "\n>;\n")
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
    "[FAIL] qualified known type-arg perf guard failed\n"
    "rc=${rc}\n"
    "stdout:\n${out}\n"
    "stderr:\n${err}")
endif()

if(NOT "${err}" STREQUAL "")
  message(FATAL_ERROR
    "[FAIL] expected no stderr from qualified known type-arg perf guard\n"
    "stderr:\n${err}")
endif()

message(STATUS "[PASS] qualified known type-arg perf guard")
message(STATUS "  source: ${src}")
message(STATUS "  repeats: ${REPEAT_COUNT}")
message(STATUS "  timeout budget: ${EXPECT_TIMEOUT_SEC}s")
