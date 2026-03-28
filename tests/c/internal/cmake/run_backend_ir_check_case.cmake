cmake_minimum_required(VERSION 3.20)

foreach(v COMPILER SRC TARGET_TRIPLE OUT_LL)
  if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
    message(FATAL_ERROR "Missing required -D${v}=...")
  endif()
endforeach()

if(NOT EXISTS "${SRC}")
  message(FATAL_ERROR "[FAIL] backend case not found: ${SRC}")
endif()

if(NOT DEFINED CASE_TIMEOUT_SEC OR "${CASE_TIMEOUT_SEC}" STREQUAL "")
  set(CASE_TIMEOUT_SEC 30)
endif()

get_filename_component(out_ll_dir "${OUT_LL}" DIRECTORY)
file(MAKE_DIRECTORY "${out_ll_dir}")

execute_process(
  COMMAND "${COMPILER}" --codegen lir --target "${TARGET_TRIPLE}" "${SRC}" -o "${OUT_LL}"
  TIMEOUT "${CASE_TIMEOUT_SEC}"
  RESULT_VARIABLE frontend_rc
  OUTPUT_VARIABLE frontend_out
  ERROR_VARIABLE frontend_err
)
if(frontend_rc MATCHES "timeout")
  message(FATAL_ERROR "[BACKEND_FRONTEND_TIMEOUT] ${SRC} exceeded ${CASE_TIMEOUT_SEC}s")
endif()
if(NOT frontend_rc EQUAL 0)
  message(FATAL_ERROR "[BACKEND_FRONTEND_FAIL] ${SRC}\n${frontend_out}${frontend_err}")
endif()

if(NOT EXISTS "${OUT_LL}")
  message(FATAL_ERROR "[BACKEND_OUTPUT_MISSING] ${SRC}\nexpected LLVM text at ${OUT_LL}")
endif()

file(READ "${OUT_LL}" ir_text)

if(NOT DEFINED REQUIRED_SNIPPETS OR "${REQUIRED_SNIPPETS}" STREQUAL "")
  set(REQUIRED_SNIPPETS
      "vaarg.fp.join.|load double, ptr|getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 4")
endif()
string(REPLACE "|" ";" required_snippets "${REQUIRED_SNIPPETS}")

foreach(needle IN LISTS required_snippets)
  string(FIND "${ir_text}" "${needle}" needle_pos)
  if(needle_pos EQUAL -1)
    message(FATAL_ERROR
      "[BACKEND_IR_MISMATCH] ${SRC}\nmissing required snippet: ${needle}\n"
      "${frontend_out}${frontend_err}")
  endif()
endforeach()

message(STATUS "[PASS][backend-ir] ${SRC}")
