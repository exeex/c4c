cmake_minimum_required(VERSION 3.20)

foreach(v COMPILER BINARY_DIR REPEAT_COUNT EXPECT_TIMEOUT_SEC)
  if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
    message(FATAL_ERROR "Missing required -D${v}=...")
  endif()
endforeach()

file(MAKE_DIRECTORY "${BINARY_DIR}/tmp")
set(src "${BINARY_DIR}/tmp/qualified_template_call_template_arg_perf.cpp")

file(WRITE "${src}" "// Generated parser perf guard\n")
file(APPEND "${src}" "template <bool B, class T = void>\n")
file(APPEND "${src}" "struct enable_if { using type = T; };\n")
file(APPEND "${src}" "template <class A, class B>\n")
file(APPEND "${src}" "struct is_same { static constexpr bool value = false; };\n")
file(APPEND "${src}" "template <class T1, class T2>\n")
file(APPEND "${src}" "struct pair_like {\n")
file(APPEND "${src}" "  template <bool, class U1, class U2>\n")
file(APPEND "${src}" "  struct chooser {\n")
file(APPEND "${src}" "    template <class X1, class X2>\n")
file(APPEND "${src}" "    static constexpr bool check() { return true; }\n")
file(APPEND "${src}" "  };\n")
file(APPEND "${src}" "  template <class U1, class U2>\n")
file(APPEND "${src}"
  "  using chooser_t = chooser<!is_same<T1, U1>::value || !is_same<T2, U2>::value, U1, U2>;\n")

math(EXPR repeat_last "${REPEAT_COUNT} - 1")
foreach(i RANGE 0 ${repeat_last})
  file(APPEND "${src}"
    "  template <class U1_${i}, class U2_${i}, class = typename enable_if<chooser_t<U1_${i}, U2_${i}>::template check<U1_${i}, U2_${i}>(), bool>::type>\n")
  file(APPEND "${src}" "  static void f${i}();\n")
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
    "[FAIL] qualified template call template arg perf guard failed\n"
    "rc=${rc}\n"
    "stdout:\n${out}\n"
    "stderr:\n${err}")
endif()

if(NOT "${err}" STREQUAL "")
  message(FATAL_ERROR
    "[FAIL] expected no stderr from qualified template call template arg perf guard\n"
    "stderr:\n${err}")
endif()

message(STATUS "[PASS] qualified template call template arg perf guard")
message(STATUS "  source: ${src}")
message(STATUS "  repeats: ${REPEAT_COUNT}")
message(STATUS "  timeout budget: ${EXPECT_TIMEOUT_SEC}s")
