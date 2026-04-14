cmake_minimum_required(VERSION 3.20)

foreach(v COMPILER TARGET_TRIPLE SRC_A OUT_A REQUIRED_SNIPPETS_A SRC_B OUT_B REQUIRED_SNIPPETS_B)
  if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
    message(FATAL_ERROR "Missing required -D${v}=...")
  endif()
endforeach()

if(NOT DEFINED CASE_TIMEOUT_SEC OR "${CASE_TIMEOUT_SEC}" STREQUAL "")
  set(CASE_TIMEOUT_SEC 30)
endif()

function(run_backend_case src out_text required_snippets)
  if(NOT EXISTS "${src}")
    message(FATAL_ERROR "[FAIL] backend route case not found: ${src}")
  endif()

  get_filename_component(out_dir "${out_text}" DIRECTORY)
  file(MAKE_DIRECTORY "${out_dir}")

  execute_process(
    COMMAND "${COMPILER}" --backend-bir-stage semantic --codegen asm --target "${TARGET_TRIPLE}" "${src}" -o "${out_text}"
    TIMEOUT "${CASE_TIMEOUT_SEC}"
    RESULT_VARIABLE compiler_rc
    OUTPUT_VARIABLE compiler_out
    ERROR_VARIABLE compiler_err
  )
  if(compiler_rc MATCHES "timeout")
    message(FATAL_ERROR "[BACKEND_ROUTE_TIMEOUT] ${src} exceeded ${CASE_TIMEOUT_SEC}s")
  endif()
  if(NOT compiler_rc EQUAL 0)
    message(FATAL_ERROR "[BACKEND_ROUTE_EMIT_FAIL] ${src}\n${compiler_out}${compiler_err}")
  endif()

  file(READ "${out_text}" output_text)
  string(REPLACE "|" ";" snippets "${required_snippets}")
  foreach(snippet IN LISTS snippets)
    if("${snippet}" STREQUAL "")
      continue()
    endif()
    string(FIND "${output_text}" "${snippet}" snippet_pos)
    if(snippet_pos EQUAL -1)
      message(FATAL_ERROR
        "[BACKEND_ROUTE_SNIPPET_MISSING] ${out_text}\nMissing snippet: ${snippet}\n--- output ---\n${output_text}")
    endif()
  endforeach()

  string(FIND "${output_text}" "define i32 @" legacy_pos)
  if(NOT legacy_pos EQUAL -1)
    message(FATAL_ERROR
      "[BACKEND_ROUTE_FORBIDDEN_SNIPPET] ${out_text}\nUnexpected LLVM fallback output\n--- output ---\n${output_text}")
  endif()
endfunction()

run_backend_case("${SRC_A}" "${OUT_A}" "${REQUIRED_SNIPPETS_A}")
run_backend_case("${SRC_B}" "${OUT_B}" "${REQUIRED_SNIPPETS_B}")

message(STATUS "[PASS][backend-route] ${SRC_A} + ${SRC_B}")
