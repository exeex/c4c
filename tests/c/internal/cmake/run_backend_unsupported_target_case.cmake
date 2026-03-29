cmake_minimum_required(VERSION 3.20)

foreach(v COMPILER SRC)
  if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
    message(FATAL_ERROR "Missing required -D${v}=...")
  endif()
endforeach()

execute_process(
  COMMAND "${COMPILER}" --codegen lir --target wasm32-unknown-unknown "${SRC}"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
  TIMEOUT 30
)

if(rc EQUAL 0)
  message(FATAL_ERROR "[BACKEND_UNSUPPORTED_TARGET] expected non-zero exit\n${out}${err}")
endif()

if(NOT err MATCHES "unsupported backend target triple")
  message(FATAL_ERROR "[BACKEND_UNSUPPORTED_TARGET] missing diagnostic\n${out}${err}")
endif()

message(STATUS "[PASS][backend] unsupported target rejected")
