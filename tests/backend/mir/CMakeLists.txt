function(c4c_add_aarch64_cpp_test test_name)
  set(options SOURCE_TEXT_AUDIT)
  cmake_parse_arguments(ARG "${options}" "" "" ${ARGN})

  add_executable(${test_name}_test
      "${BACKEND_TEST_ROOT}/mir/${test_name}_test.cpp"
  )
  target_include_directories(${test_name}_test PRIVATE
      "${PROJECT_SOURCE_DIR}"
  )
  if(ARG_SOURCE_TEXT_AUDIT)
    target_compile_definitions(${test_name}_test PRIVATE
        C4C_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
    )
  else()
    target_link_libraries(${test_name}_test PRIVATE
        c4c_backend
    )
  endif()
  set_target_properties(${test_name}_test PROPERTIES
      CXX_STANDARD 17
      CXX_STANDARD_REQUIRED ON
  )

  add_test(
    NAME ${test_name}
    COMMAND $<TARGET_FILE:${test_name}_test>
  )
endfunction()

c4c_add_aarch64_cpp_test(backend_aarch64_signature_metadata SOURCE_TEXT_AUDIT)
set_tests_properties(backend_aarch64_signature_metadata PROPERTIES
  LABELS "backend;internal;cpp"
)

c4c_add_aarch64_cpp_test(backend_aarch64_prepared_handoff_gate)
set_tests_properties(backend_aarch64_prepared_handoff_gate PROPERTIES
  LABELS "backend;internal;cpp;aarch64"
)

c4c_add_aarch64_cpp_test(backend_aarch64_register_vocabulary)
set_tests_properties(backend_aarch64_register_vocabulary PROPERTIES
  LABELS "backend;internal;cpp;aarch64"
)

c4c_add_aarch64_cpp_test(backend_aarch64_prepared_register_conversion)
set_tests_properties(backend_aarch64_prepared_register_conversion PROPERTIES
  LABELS "backend;internal;cpp;aarch64"
)

c4c_add_aarch64_cpp_test(backend_aarch64_target_operand_records)
set_tests_properties(backend_aarch64_target_operand_records PROPERTIES
  LABELS "backend;internal;cpp;aarch64"
)

c4c_add_aarch64_cpp_test(backend_aarch64_memory_operand_records)
set_tests_properties(backend_aarch64_memory_operand_records PROPERTIES
  LABELS "backend;internal;cpp;aarch64"
)

c4c_add_aarch64_cpp_test(backend_aarch64_prepared_memory_operand_records)
set_tests_properties(backend_aarch64_prepared_memory_operand_records PROPERTIES
  LABELS "backend;internal;cpp;aarch64"
)

c4c_add_aarch64_cpp_test(backend_aarch64_memory_operand_contract)
set_tests_properties(backend_aarch64_memory_operand_contract PROPERTIES
  LABELS "backend;internal;cpp;aarch64"
)

c4c_add_aarch64_cpp_test(backend_aarch64_target_instruction_records)
set_tests_properties(backend_aarch64_target_instruction_records PROPERTIES
  LABELS "backend;internal;cpp;aarch64"
)

c4c_add_aarch64_cpp_test(backend_aarch64_machine_printer)
set_tests_properties(backend_aarch64_machine_printer PROPERTIES
  LABELS "backend;internal;cpp;aarch64"
)

c4c_add_aarch64_cpp_test(backend_aarch64_scalar_alu_records)
set_tests_properties(backend_aarch64_scalar_alu_records PROPERTIES
  LABELS "backend;internal;cpp;aarch64"
)

c4c_add_aarch64_cpp_test(backend_aarch64_scalar_cast_records)
set_tests_properties(backend_aarch64_scalar_cast_records PROPERTIES
  LABELS "backend;internal;cpp;aarch64"
)

c4c_add_aarch64_cpp_test(backend_aarch64_prepared_scalar_alu_records)
set_tests_properties(backend_aarch64_prepared_scalar_alu_records PROPERTIES
  LABELS "backend;internal;cpp;aarch64"
)

c4c_add_aarch64_cpp_test(backend_aarch64_prepared_scalar_cast_records)
set_tests_properties(backend_aarch64_prepared_scalar_cast_records PROPERTIES
  LABELS "backend;internal;cpp;aarch64"
)

c4c_add_aarch64_cpp_test(backend_aarch64_scalar_record_contract)
set_tests_properties(backend_aarch64_scalar_record_contract PROPERTIES
  LABELS "backend;internal;cpp;aarch64"
)

c4c_add_aarch64_cpp_test(backend_aarch64_branch_compare_records)
set_tests_properties(backend_aarch64_branch_compare_records PROPERTIES
  LABELS "backend;internal;cpp;aarch64"
)

c4c_add_aarch64_cpp_test(backend_aarch64_compare_branch_candidate_records)
set_tests_properties(backend_aarch64_compare_branch_candidate_records PROPERTIES
  LABELS "backend;internal;cpp;aarch64"
)

c4c_add_aarch64_cpp_test(backend_aarch64_branch_compare_contract)
set_tests_properties(backend_aarch64_branch_compare_contract PROPERTIES
  LABELS "backend;internal;cpp;aarch64"
)

c4c_add_aarch64_cpp_test(backend_aarch64_prepared_branch_records)
set_tests_properties(backend_aarch64_prepared_branch_records PROPERTIES
  LABELS "backend;internal;cpp;aarch64"
)

c4c_add_aarch64_cpp_test(backend_aarch64_target_record_core_contract)
set_tests_properties(backend_aarch64_target_record_core_contract PROPERTIES
  LABELS "backend;internal;cpp;aarch64"
)

c4c_add_aarch64_cpp_test(backend_aarch64_prepared_module_identity)
set_tests_properties(backend_aarch64_prepared_module_identity PROPERTIES
  LABELS "backend;internal;cpp;aarch64"
)

c4c_add_aarch64_cpp_test(backend_aarch64_prepared_operand_identity)
set_tests_properties(backend_aarch64_prepared_operand_identity PROPERTIES
  LABELS "backend;internal;cpp;aarch64"
)

c4c_add_aarch64_cpp_test(backend_aarch64_prepared_frame_control)
set_tests_properties(backend_aarch64_prepared_frame_control PROPERTIES
  LABELS "backend;internal;cpp;aarch64"
)

c4c_add_aarch64_cpp_test(backend_aarch64_prepared_data_identity)
set_tests_properties(backend_aarch64_prepared_data_identity PROPERTIES
  LABELS "backend;internal;cpp;aarch64"
)

add_test(
  NAME backend_cli_aarch64_asm_no_machine_nodes_fails
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DTARGET_TRIPLE=aarch64-linux-gnu
          -DSRC=${BACKEND_CASE_ROOT}/aarch64_no_selected_machine_nodes.c
          -DOUT_TEXT=${BACKEND_TEST_BINARY_ROOT}/aarch64_no_selected_machine_nodes.s
          "-DEXPECTED_DIAGNOSTICS=AArch64 backend assembly route reached the machine-node printer|no selected printable machine nodes"
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_codegen_failure_case.cmake"
)
set_tests_properties(backend_cli_aarch64_asm_no_machine_nodes_fails PROPERTIES
  LABELS "backend;internal;cli;aarch64"
)

if(CLANG_EXECUTABLE AND AARCH64_AS_EXECUTABLE)
  add_test(
    NAME backend_cli_aarch64_asm_external_return_zero_smoke
    COMMAND "${CMAKE_COMMAND}"
            -DCOMPILER=$<TARGET_FILE:c4cll>
            -DCLANG=${CLANG_EXECUTABLE}
            -DAS=${AARCH64_AS_EXECUTABLE}
            -DTARGET_TRIPLE=aarch64-linux-gnu
            -DSRC=${BACKEND_CASE_ROOT}/aarch64_return_zero_smoke.c
            -DOUT_ASM=${BACKEND_TEST_BINARY_ROOT}/aarch64_return_zero_smoke.s
            -DOUT_OBJ=${BACKEND_TEST_BINARY_ROOT}/aarch64_return_zero_smoke.o
            -DOUT_BIN=${BACKEND_TEST_BINARY_ROOT}/aarch64_return_zero_smoke.bin
            -DHOST_PROCESSOR=${CMAKE_HOST_SYSTEM_PROCESSOR}
            -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_aarch64_external_smoke.cmake"
  )
  set_tests_properties(backend_cli_aarch64_asm_external_return_zero_smoke PROPERTIES
    LABELS "backend;internal;cli;aarch64;external_toolchain"
  )
  add_test(
    NAME backend_cli_aarch64_asm_external_return_add_smoke
    COMMAND "${CMAKE_COMMAND}"
            -DCOMPILER=$<TARGET_FILE:c4cll>
            -DCLANG=${CLANG_EXECUTABLE}
            -DAS=${AARCH64_AS_EXECUTABLE}
            -DTARGET_TRIPLE=aarch64-linux-gnu
            -DSRC=${BACKEND_CASE_ROOT}/return_add.c
            -DOUT_ASM=${BACKEND_TEST_BINARY_ROOT}/aarch64_return_add_smoke.s
            -DOUT_OBJ=${BACKEND_TEST_BINARY_ROOT}/aarch64_return_add_smoke.o
            -DOUT_BIN=${BACKEND_TEST_BINARY_ROOT}/aarch64_return_add_smoke.bin
            -DHOST_PROCESSOR=${CMAKE_HOST_SYSTEM_PROCESSOR}
            "-DREQUIRED_SNIPPETS=.globl main|main:|mov w0, #2|add w0, w0, #3|ret"
            -DEXPECTED_RUN_RC=5
            -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_aarch64_external_smoke.cmake"
  )
  set_tests_properties(backend_cli_aarch64_asm_external_return_add_smoke PROPERTIES
    LABELS "backend;internal;cli;aarch64;external_toolchain"
  )
else()
  message(WARNING
    "clang or AArch64 assembler not found: backend_cli_aarch64_asm_external_return_zero_smoke and backend_cli_aarch64_asm_external_return_add_smoke will be skipped")
endif()
