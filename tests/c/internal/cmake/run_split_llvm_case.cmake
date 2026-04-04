cmake_minimum_required(VERSION 3.20)

foreach(v COMPILER SRC DEVICE_TARGET_TRIPLE)
  if(NOT DEFINED ${v} OR "${${v}}" STREQUAL "")
    message(FATAL_ERROR "Missing required -D${v}=...")
  endif()
endforeach()

if(NOT EXISTS "${SRC}")
  message(FATAL_ERROR "[FAIL] split llvm case not found: ${SRC}")
endif()

if(NOT DEFINED CASE_TIMEOUT_SEC OR "${CASE_TIMEOUT_SEC}" STREQUAL "")
  set(CASE_TIMEOUT_SEC 30)
endif()

if(NOT DEFINED TARGET_TRIPLE OR "${TARGET_TRIPLE}" STREQUAL "")
  set(TARGET_TRIPLE x86_64-unknown-linux-gnu)
endif()

set(test_dir "${CMAKE_BINARY_DIR}/internal_split_llvm")
file(MAKE_DIRECTORY "${test_dir}")
set(input_copy "${test_dir}/split_exec_case.x")
file(COPY_FILE "${SRC}" "${input_copy}" ONLY_IF_DIFFERENT)

set(expected_host "${test_dir}/split_exec_case.ll")
set(expected_device "${test_dir}/split_exec_case.device.ll")

execute_process(
  COMMAND "${COMPILER}"
          --emit-split-llvm
          --target "${TARGET_TRIPLE}"
          --device-target "${DEVICE_TARGET_TRIPLE}"
          "${input_copy}"
  TIMEOUT "${CASE_TIMEOUT_SEC}"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
)
if(rc MATCHES "timeout")
  message(FATAL_ERROR "[SPLIT_LLVM_TIMEOUT] ${SRC} exceeded ${CASE_TIMEOUT_SEC}s")
endif()
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "[SPLIT_LLVM_FAIL] ${SRC}\n${out}${err}")
endif()

foreach(path IN ITEMS "${expected_host}" "${expected_device}")
  if(NOT EXISTS "${path}")
    message(FATAL_ERROR "[SPLIT_LLVM_OUTPUT_MISSING] missing output: ${path}")
  endif()
endforeach()

file(READ "${expected_host}" host_text)
file(READ "${expected_device}" device_text)

foreach(snippet IN ITEMS
        "target triple = \"${TARGET_TRIPLE}\""
        "define i32 @host_fn()"
        "define i32 @main()")
  string(FIND "${host_text}" "${snippet}" pos)
  if(pos EQUAL -1)
    message(FATAL_ERROR "[SPLIT_LLVM_HOST_SNIPPET_MISSING] ${expected_host}\nMissing snippet: ${snippet}\n--- output ---\n${host_text}")
  endif()
endforeach()

foreach(snippet IN ITEMS
        "define i32 @device_fn()")
  string(FIND "${host_text}" "${snippet}" pos)
  if(NOT pos EQUAL -1)
    message(FATAL_ERROR "[SPLIT_LLVM_HOST_UNEXPECTED_SNIPPET] ${expected_host}\nUnexpected snippet: ${snippet}\n--- output ---\n${host_text}")
  endif()
endforeach()

foreach(snippet IN ITEMS
        "target triple = \"${DEVICE_TARGET_TRIPLE}\""
        "define i32 @device_fn()")
  string(FIND "${device_text}" "${snippet}" pos)
  if(pos EQUAL -1)
    message(FATAL_ERROR "[SPLIT_LLVM_DEVICE_SNIPPET_MISSING] ${expected_device}\nMissing snippet: ${snippet}\n--- output ---\n${device_text}")
  endif()
endforeach()

foreach(snippet IN ITEMS
        "define i32 @host_fn()"
        "define i32 @main()")
  string(FIND "${device_text}" "${snippet}" pos)
  if(NOT pos EQUAL -1)
    message(FATAL_ERROR "[SPLIT_LLVM_DEVICE_UNEXPECTED_SNIPPET] ${expected_device}\nUnexpected snippet: ${snippet}\n--- output ---\n${device_text}")
  endif()
endforeach()

message(STATUS "[PASS][split-llvm] ${SRC}")
