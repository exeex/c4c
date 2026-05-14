set(BACKEND_TEST_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(BACKEND_TEST_CMAKE_ROOT "${BACKEND_TEST_ROOT}/cmake")
set(BACKEND_CASE_ROOT "${BACKEND_TEST_ROOT}/case")
find_program(AARCH64_AS_EXECUTABLE NAMES aarch64-linux-gnu-as)

option(C4C_ENABLE_X86_BACKEND_TESTS
       "Enable x86 emitter and x86 handoff boundary backend tests"
       OFF)

function(c4c_collect_backend_labels out_var)
  set(labels internal backend)
  foreach(label IN LISTS ARGN)
    if(NOT "${label}" STREQUAL "")
      list(APPEND labels "${label}")
    endif()
  endforeach()
  list(JOIN labels ";" labels_value)
  set(${out_var} "${labels_value}" PARENT_SCOPE)
endfunction()

function(c4c_set_backend_test_labels test_name)
  c4c_collect_backend_labels(labels ${ARGN})
  set_tests_properties("${test_name}" PROPERTIES LABELS "${labels}")
endfunction()

function(c4c_disable_backend_test_when_x86_tests_off test_name)
  if(NOT C4C_ENABLE_X86_BACKEND_TESTS)
    set_tests_properties("${test_name}" PROPERTIES DISABLED TRUE)
  endif()
endfunction()

function(c4c_add_backend_codegen_route_test test_name)
  set(options)
  set(one_value_args SRC TARGET_TRIPLE OUT_TEXT REQUIRED_SNIPPETS FORBIDDEN_SNIPPETS
                     EXTRA_COMPILER_ARGS)
  set(multi_value_args LABELS)
  cmake_parse_arguments(ARG "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

  if(ARG_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "c4c_add_backend_codegen_route_test(${test_name}) got unexpected arguments: ${ARG_UNPARSED_ARGUMENTS}")
  endif()

  foreach(required_arg IN ITEMS SRC TARGET_TRIPLE OUT_TEXT)
    if(NOT DEFINED ARG_${required_arg} OR "${ARG_${required_arg}}" STREQUAL "")
      message(FATAL_ERROR
        "c4c_add_backend_codegen_route_test(${test_name}) missing ${required_arg}")
    endif()
  endforeach()

  add_test(
    NAME "${test_name}"
    COMMAND "${CMAKE_COMMAND}"
            -DCOMPILER=$<TARGET_FILE:c4cll>
            -DSRC=${ARG_SRC}
            -DTARGET_TRIPLE=${ARG_TARGET_TRIPLE}
            -DOUT_TEXT=${ARG_OUT_TEXT}
            "-DREQUIRED_SNIPPETS=${ARG_REQUIRED_SNIPPETS}"
            "-DFORBIDDEN_SNIPPETS=${ARG_FORBIDDEN_SNIPPETS}"
            "-DEXTRA_COMPILER_ARGS=${ARG_EXTRA_COMPILER_ARGS}"
            -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_codegen_route_case.cmake"
  )
  c4c_set_backend_test_labels("${test_name}" backend_route ${ARG_LABELS})
endfunction()

function(c4c_add_backend_runtime_case test_name)
  set(options)
  set(one_value_args SRC)
  set(multi_value_args LABELS)
  cmake_parse_arguments(ARG "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

  if(ARG_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "c4c_add_backend_runtime_case(${test_name}) got unexpected arguments: ${ARG_UNPARSED_ARGUMENTS}")
  endif()

  if(NOT DEFINED ARG_SRC OR "${ARG_SRC}" STREQUAL "")
    message(FATAL_ERROR
      "c4c_add_backend_runtime_case(${test_name}) missing SRC")
  endif()

  if(NOT CLANG_EXECUTABLE)
    message(FATAL_ERROR
      "c4c_add_backend_runtime_case(${test_name}) requires CLANG_EXECUTABLE")
  endif()

  get_filename_component(case_stem "${ARG_SRC}" NAME_WE)

  add_test(
    NAME "${test_name}"
    COMMAND "${CMAKE_COMMAND}"
            -DCOMPILER=$<TARGET_FILE:c4cll>
            -DCLANG=${CLANG_EXECUTABLE}
            -DSRC=${ARG_SRC}
            -DOUT_LL=${CMAKE_BINARY_DIR}/backend_runtime/${case_stem}.ll
            -DOUT_CLANG_BIN=${CMAKE_BINARY_DIR}/backend_runtime/${case_stem}.clang.bin
            -DOUT_C2LL_BIN=${CMAKE_BINARY_DIR}/backend_runtime/${case_stem}.c2ll.bin
            -P "${PROJECT_SOURCE_DIR}/tests/c/internal/cmake/run_positive_case.cmake"
  )
  c4c_set_backend_test_labels("${test_name}" backend_runtime case ${ARG_LABELS})
endfunction()

add_executable(backend_aarch64_signature_metadata_test
    "${BACKEND_TEST_ROOT}/backend_aarch64_signature_metadata_test.cpp"
)
target_include_directories(backend_aarch64_signature_metadata_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
)
target_compile_definitions(backend_aarch64_signature_metadata_test PRIVATE
    C4C_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
)
set_target_properties(backend_aarch64_signature_metadata_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

add_test(
  NAME backend_aarch64_signature_metadata
  COMMAND $<TARGET_FILE:backend_aarch64_signature_metadata_test>
)
set_tests_properties(backend_aarch64_signature_metadata PROPERTIES
  LABELS "backend;internal;cpp"
)

add_executable(backend_aarch64_prepared_handoff_gate_test
    "${BACKEND_TEST_ROOT}/backend_aarch64_prepared_handoff_gate_test.cpp"
)
target_include_directories(backend_aarch64_prepared_handoff_gate_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
)
target_link_libraries(backend_aarch64_prepared_handoff_gate_test PRIVATE
    c4c_backend
)
set_target_properties(backend_aarch64_prepared_handoff_gate_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

add_test(
  NAME backend_aarch64_prepared_handoff_gate
  COMMAND $<TARGET_FILE:backend_aarch64_prepared_handoff_gate_test>
)
set_tests_properties(backend_aarch64_prepared_handoff_gate PROPERTIES
  LABELS "backend;internal;cpp;aarch64"
)

add_executable(backend_aarch64_register_vocabulary_test
    "${BACKEND_TEST_ROOT}/backend_aarch64_register_vocabulary_test.cpp"
)
target_include_directories(backend_aarch64_register_vocabulary_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
)
target_link_libraries(backend_aarch64_register_vocabulary_test PRIVATE
    c4c_backend
)
set_target_properties(backend_aarch64_register_vocabulary_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

add_test(
  NAME backend_aarch64_register_vocabulary
  COMMAND $<TARGET_FILE:backend_aarch64_register_vocabulary_test>
)
set_tests_properties(backend_aarch64_register_vocabulary PROPERTIES
  LABELS "backend;internal;cpp;aarch64"
)

add_executable(backend_aarch64_prepared_register_conversion_test
    "${BACKEND_TEST_ROOT}/backend_aarch64_prepared_register_conversion_test.cpp"
)
target_include_directories(backend_aarch64_prepared_register_conversion_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
)
target_link_libraries(backend_aarch64_prepared_register_conversion_test PRIVATE
    c4c_backend
)
set_target_properties(backend_aarch64_prepared_register_conversion_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

add_test(
  NAME backend_aarch64_prepared_register_conversion
  COMMAND $<TARGET_FILE:backend_aarch64_prepared_register_conversion_test>
)
set_tests_properties(backend_aarch64_prepared_register_conversion PROPERTIES
  LABELS "backend;internal;cpp;aarch64"
)

add_executable(backend_aarch64_target_operand_records_test
    "${BACKEND_TEST_ROOT}/backend_aarch64_target_operand_records_test.cpp"
)
target_include_directories(backend_aarch64_target_operand_records_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
)
target_link_libraries(backend_aarch64_target_operand_records_test PRIVATE
    c4c_backend
)
set_target_properties(backend_aarch64_target_operand_records_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

add_test(
  NAME backend_aarch64_target_operand_records
  COMMAND $<TARGET_FILE:backend_aarch64_target_operand_records_test>
)
set_tests_properties(backend_aarch64_target_operand_records PROPERTIES
  LABELS "backend;internal;cpp;aarch64"
)

add_executable(backend_aarch64_memory_operand_records_test
    "${BACKEND_TEST_ROOT}/backend_aarch64_memory_operand_records_test.cpp"
)
target_include_directories(backend_aarch64_memory_operand_records_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
)
target_link_libraries(backend_aarch64_memory_operand_records_test PRIVATE
    c4c_backend
)
set_target_properties(backend_aarch64_memory_operand_records_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

add_test(
  NAME backend_aarch64_memory_operand_records
  COMMAND $<TARGET_FILE:backend_aarch64_memory_operand_records_test>
)
set_tests_properties(backend_aarch64_memory_operand_records PROPERTIES
  LABELS "backend;internal;cpp;aarch64"
)

add_executable(backend_aarch64_prepared_memory_operand_records_test
    "${BACKEND_TEST_ROOT}/backend_aarch64_prepared_memory_operand_records_test.cpp"
)
target_include_directories(backend_aarch64_prepared_memory_operand_records_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
)
target_link_libraries(backend_aarch64_prepared_memory_operand_records_test PRIVATE
    c4c_backend
)
set_target_properties(backend_aarch64_prepared_memory_operand_records_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

add_test(
  NAME backend_aarch64_prepared_memory_operand_records
  COMMAND $<TARGET_FILE:backend_aarch64_prepared_memory_operand_records_test>
)
set_tests_properties(backend_aarch64_prepared_memory_operand_records PROPERTIES
  LABELS "backend;internal;cpp;aarch64"
)

add_executable(backend_aarch64_memory_operand_contract_test
    "${BACKEND_TEST_ROOT}/backend_aarch64_memory_operand_contract_test.cpp"
)
target_include_directories(backend_aarch64_memory_operand_contract_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
)
target_link_libraries(backend_aarch64_memory_operand_contract_test PRIVATE
    c4c_backend
)
set_target_properties(backend_aarch64_memory_operand_contract_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

add_test(
  NAME backend_aarch64_memory_operand_contract
  COMMAND $<TARGET_FILE:backend_aarch64_memory_operand_contract_test>
)
set_tests_properties(backend_aarch64_memory_operand_contract PROPERTIES
  LABELS "backend;internal;cpp;aarch64"
)

add_executable(backend_aarch64_target_instruction_records_test
    "${BACKEND_TEST_ROOT}/backend_aarch64_target_instruction_records_test.cpp"
)
target_include_directories(backend_aarch64_target_instruction_records_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
)
target_link_libraries(backend_aarch64_target_instruction_records_test PRIVATE
    c4c_backend
)
set_target_properties(backend_aarch64_target_instruction_records_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

add_test(
  NAME backend_aarch64_target_instruction_records
  COMMAND $<TARGET_FILE:backend_aarch64_target_instruction_records_test>
)
set_tests_properties(backend_aarch64_target_instruction_records PROPERTIES
  LABELS "backend;internal;cpp;aarch64"
)

add_executable(backend_aarch64_machine_printer_test
    "${BACKEND_TEST_ROOT}/backend_aarch64_machine_printer_test.cpp"
)
target_include_directories(backend_aarch64_machine_printer_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
)
target_link_libraries(backend_aarch64_machine_printer_test PRIVATE
    c4c_backend
)
set_target_properties(backend_aarch64_machine_printer_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

add_test(
  NAME backend_aarch64_machine_printer
  COMMAND $<TARGET_FILE:backend_aarch64_machine_printer_test>
)
set_tests_properties(backend_aarch64_machine_printer PROPERTIES
  LABELS "backend;internal;cpp;aarch64"
)

add_executable(backend_aarch64_scalar_alu_records_test
    "${BACKEND_TEST_ROOT}/backend_aarch64_scalar_alu_records_test.cpp"
)
target_include_directories(backend_aarch64_scalar_alu_records_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
)
target_link_libraries(backend_aarch64_scalar_alu_records_test PRIVATE
    c4c_backend
)
set_target_properties(backend_aarch64_scalar_alu_records_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

add_test(
  NAME backend_aarch64_scalar_alu_records
  COMMAND $<TARGET_FILE:backend_aarch64_scalar_alu_records_test>
)
set_tests_properties(backend_aarch64_scalar_alu_records PROPERTIES
  LABELS "backend;internal;cpp;aarch64"
)

add_executable(backend_aarch64_scalar_cast_records_test
    "${BACKEND_TEST_ROOT}/backend_aarch64_scalar_cast_records_test.cpp"
)
target_include_directories(backend_aarch64_scalar_cast_records_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
)
target_link_libraries(backend_aarch64_scalar_cast_records_test PRIVATE
    c4c_backend
)
set_target_properties(backend_aarch64_scalar_cast_records_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

add_test(
  NAME backend_aarch64_scalar_cast_records
  COMMAND $<TARGET_FILE:backend_aarch64_scalar_cast_records_test>
)
set_tests_properties(backend_aarch64_scalar_cast_records PROPERTIES
  LABELS "backend;internal;cpp;aarch64"
)

add_executable(backend_aarch64_prepared_scalar_alu_records_test
    "${BACKEND_TEST_ROOT}/backend_aarch64_prepared_scalar_alu_records_test.cpp"
)
target_include_directories(backend_aarch64_prepared_scalar_alu_records_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
)
target_link_libraries(backend_aarch64_prepared_scalar_alu_records_test PRIVATE
    c4c_backend
)
set_target_properties(backend_aarch64_prepared_scalar_alu_records_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

add_test(
  NAME backend_aarch64_prepared_scalar_alu_records
  COMMAND $<TARGET_FILE:backend_aarch64_prepared_scalar_alu_records_test>
)
set_tests_properties(backend_aarch64_prepared_scalar_alu_records PROPERTIES
  LABELS "backend;internal;cpp;aarch64"
)

add_executable(backend_aarch64_prepared_scalar_cast_records_test
    "${BACKEND_TEST_ROOT}/backend_aarch64_prepared_scalar_cast_records_test.cpp"
)
target_include_directories(backend_aarch64_prepared_scalar_cast_records_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
)
target_link_libraries(backend_aarch64_prepared_scalar_cast_records_test PRIVATE
    c4c_backend
)
set_target_properties(backend_aarch64_prepared_scalar_cast_records_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

add_test(
  NAME backend_aarch64_prepared_scalar_cast_records
  COMMAND $<TARGET_FILE:backend_aarch64_prepared_scalar_cast_records_test>
)
set_tests_properties(backend_aarch64_prepared_scalar_cast_records PROPERTIES
  LABELS "backend;internal;cpp;aarch64"
)

add_executable(backend_aarch64_scalar_record_contract_test
    "${BACKEND_TEST_ROOT}/backend_aarch64_scalar_record_contract_test.cpp"
)
target_include_directories(backend_aarch64_scalar_record_contract_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
)
target_link_libraries(backend_aarch64_scalar_record_contract_test PRIVATE
    c4c_backend
)
set_target_properties(backend_aarch64_scalar_record_contract_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

add_test(
  NAME backend_aarch64_scalar_record_contract
  COMMAND $<TARGET_FILE:backend_aarch64_scalar_record_contract_test>
)
set_tests_properties(backend_aarch64_scalar_record_contract PROPERTIES
  LABELS "backend;internal;cpp;aarch64"
)

add_executable(backend_aarch64_branch_compare_records_test
    "${BACKEND_TEST_ROOT}/backend_aarch64_branch_compare_records_test.cpp"
)
target_include_directories(backend_aarch64_branch_compare_records_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
)
target_link_libraries(backend_aarch64_branch_compare_records_test PRIVATE
    c4c_backend
)
set_target_properties(backend_aarch64_branch_compare_records_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

add_test(
  NAME backend_aarch64_branch_compare_records
  COMMAND $<TARGET_FILE:backend_aarch64_branch_compare_records_test>
)
set_tests_properties(backend_aarch64_branch_compare_records PROPERTIES
  LABELS "backend;internal;cpp;aarch64"
)

add_executable(backend_aarch64_compare_branch_candidate_records_test
    "${BACKEND_TEST_ROOT}/backend_aarch64_compare_branch_candidate_records_test.cpp"
)
target_include_directories(backend_aarch64_compare_branch_candidate_records_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
)
target_link_libraries(backend_aarch64_compare_branch_candidate_records_test PRIVATE
    c4c_backend
)
set_target_properties(backend_aarch64_compare_branch_candidate_records_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

add_test(
  NAME backend_aarch64_compare_branch_candidate_records
  COMMAND $<TARGET_FILE:backend_aarch64_compare_branch_candidate_records_test>
)
set_tests_properties(backend_aarch64_compare_branch_candidate_records PROPERTIES
  LABELS "backend;internal;cpp;aarch64"
)

add_executable(backend_aarch64_branch_compare_contract_test
    "${BACKEND_TEST_ROOT}/backend_aarch64_branch_compare_contract_test.cpp"
)
target_include_directories(backend_aarch64_branch_compare_contract_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
)
target_link_libraries(backend_aarch64_branch_compare_contract_test PRIVATE
    c4c_backend
)
set_target_properties(backend_aarch64_branch_compare_contract_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

add_test(
  NAME backend_aarch64_branch_compare_contract
  COMMAND $<TARGET_FILE:backend_aarch64_branch_compare_contract_test>
)
set_tests_properties(backend_aarch64_branch_compare_contract PROPERTIES
  LABELS "backend;internal;cpp;aarch64"
)

add_executable(backend_aarch64_prepared_branch_records_test
    "${BACKEND_TEST_ROOT}/backend_aarch64_prepared_branch_records_test.cpp"
)
target_include_directories(backend_aarch64_prepared_branch_records_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
)
target_link_libraries(backend_aarch64_prepared_branch_records_test PRIVATE
    c4c_backend
)
set_target_properties(backend_aarch64_prepared_branch_records_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

add_test(
  NAME backend_aarch64_prepared_branch_records
  COMMAND $<TARGET_FILE:backend_aarch64_prepared_branch_records_test>
)
set_tests_properties(backend_aarch64_prepared_branch_records PROPERTIES
  LABELS "backend;internal;cpp;aarch64"
)

add_executable(backend_aarch64_target_record_core_contract_test
    "${BACKEND_TEST_ROOT}/backend_aarch64_target_record_core_contract_test.cpp"
)
target_include_directories(backend_aarch64_target_record_core_contract_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
)
target_link_libraries(backend_aarch64_target_record_core_contract_test PRIVATE
    c4c_backend
)
set_target_properties(backend_aarch64_target_record_core_contract_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

add_test(
  NAME backend_aarch64_target_record_core_contract
  COMMAND $<TARGET_FILE:backend_aarch64_target_record_core_contract_test>
)
set_tests_properties(backend_aarch64_target_record_core_contract PROPERTIES
  LABELS "backend;internal;cpp;aarch64"
)

add_executable(backend_aarch64_prepared_module_identity_test
    "${BACKEND_TEST_ROOT}/backend_aarch64_prepared_module_identity_test.cpp"
)
target_include_directories(backend_aarch64_prepared_module_identity_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
)
target_link_libraries(backend_aarch64_prepared_module_identity_test PRIVATE
    c4c_backend
)
set_target_properties(backend_aarch64_prepared_module_identity_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

add_test(
  NAME backend_aarch64_prepared_module_identity
  COMMAND $<TARGET_FILE:backend_aarch64_prepared_module_identity_test>
)
set_tests_properties(backend_aarch64_prepared_module_identity PROPERTIES
  LABELS "backend;internal;cpp;aarch64"
)

add_executable(backend_aarch64_prepared_operand_identity_test
    "${BACKEND_TEST_ROOT}/backend_aarch64_prepared_operand_identity_test.cpp"
)
target_include_directories(backend_aarch64_prepared_operand_identity_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
)
target_link_libraries(backend_aarch64_prepared_operand_identity_test PRIVATE
    c4c_backend
)
set_target_properties(backend_aarch64_prepared_operand_identity_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

add_test(
  NAME backend_aarch64_prepared_operand_identity
  COMMAND $<TARGET_FILE:backend_aarch64_prepared_operand_identity_test>
)
set_tests_properties(backend_aarch64_prepared_operand_identity PROPERTIES
  LABELS "backend;internal;cpp;aarch64"
)

add_executable(backend_aarch64_prepared_frame_control_test
    "${BACKEND_TEST_ROOT}/backend_aarch64_prepared_frame_control_test.cpp"
)
target_include_directories(backend_aarch64_prepared_frame_control_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
)
target_link_libraries(backend_aarch64_prepared_frame_control_test PRIVATE
    c4c_backend
)
set_target_properties(backend_aarch64_prepared_frame_control_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

add_test(
  NAME backend_aarch64_prepared_frame_control
  COMMAND $<TARGET_FILE:backend_aarch64_prepared_frame_control_test>
)
set_tests_properties(backend_aarch64_prepared_frame_control PROPERTIES
  LABELS "backend;internal;cpp;aarch64"
)

add_executable(backend_aarch64_prepared_data_identity_test
    "${BACKEND_TEST_ROOT}/backend_aarch64_prepared_data_identity_test.cpp"
)
target_include_directories(backend_aarch64_prepared_data_identity_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
)
target_link_libraries(backend_aarch64_prepared_data_identity_test PRIVATE
    c4c_backend
)
set_target_properties(backend_aarch64_prepared_data_identity_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

add_test(
  NAME backend_aarch64_prepared_data_identity
  COMMAND $<TARGET_FILE:backend_aarch64_prepared_data_identity_test>
)
set_tests_properties(backend_aarch64_prepared_data_identity PROPERTIES
  LABELS "backend;internal;cpp;aarch64"
)

add_subdirectory(bir)

add_test(
  NAME backend_cli_dump_mir_is_nonfatal_trace_shell
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DTARGET_TRIPLE=x86_64-unknown-linux-gnu
          -DDUMP_FLAG=--dump-mir
          -DSRC=${PROJECT_SOURCE_DIR}/tests/c/internal/compare_case/smoke_expr_branch_lifecycle.c
          "-DREQUIRED_SNIPPETS=x86 route summary|route owner: x86/debug|module emitter: x86/module|status: contract-first debug surface; structured lane diagnostics still deferred"
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_dump_flag_case.cmake"
)
set_tests_properties(backend_cli_dump_mir_is_nonfatal_trace_shell PROPERTIES
  LABELS "backend;internal;cli"
)
c4c_disable_backend_test_when_x86_tests_off(backend_cli_dump_mir_is_nonfatal_trace_shell)

add_test(
  NAME backend_cli_trace_mir_reports_lane_detail
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DTARGET_TRIPLE=x86_64-unknown-linux-gnu
          -DDUMP_FLAG=--trace-mir
          -DSRC=${PROJECT_SOURCE_DIR}/tests/c/internal/compare_case/smoke_expr_branch_lifecycle.c
          "-DREQUIRED_SNIPPETS=x86 route trace|route owner: x86/debug|module emitter: x86/module|- function run:|status: contract-first debug surface; structured lane diagnostics still deferred"
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_dump_flag_case.cmake"
)
set_tests_properties(backend_cli_trace_mir_reports_lane_detail PROPERTIES
  LABELS "backend;internal;cli"
)
c4c_disable_backend_test_when_x86_tests_off(backend_cli_trace_mir_reports_lane_detail)

add_test(
  NAME backend_cli_aarch64_asm_no_machine_nodes_fails
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DTARGET_TRIPLE=aarch64-linux-gnu
          -DSRC=${BACKEND_CASE_ROOT}/aarch64_no_selected_machine_nodes.c
          -DOUT_TEXT=${CMAKE_CURRENT_BINARY_DIR}/aarch64_no_selected_machine_nodes.s
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
            -DOUT_ASM=${CMAKE_CURRENT_BINARY_DIR}/aarch64_return_zero_smoke.s
            -DOUT_OBJ=${CMAKE_CURRENT_BINARY_DIR}/aarch64_return_zero_smoke.o
            -DOUT_BIN=${CMAKE_CURRENT_BINARY_DIR}/aarch64_return_zero_smoke.bin
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
            -DOUT_ASM=${CMAKE_CURRENT_BINARY_DIR}/aarch64_return_add_smoke.s
            -DOUT_OBJ=${CMAKE_CURRENT_BINARY_DIR}/aarch64_return_add_smoke.o
            -DOUT_BIN=${CMAKE_CURRENT_BINARY_DIR}/aarch64_return_add_smoke.bin
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

add_test(
  NAME backend_cli_dump_mir_focus_function_filters_00204
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DTARGET_TRIPLE=x86_64-unknown-linux-gnu
          -DDUMP_FLAG=--dump-mir
          "-DEXTRA_ARGS=--mir-focus-function;stdarg"
          -DSRC=${PROJECT_SOURCE_DIR}/tests/c/external/c-testsuite/src/00204.c
          "-DREQUIRED_SNIPPETS=x86 route summary|focus function: stdarg|route owner: x86/debug|module emitter: x86/module|status: contract-first debug surface; structured lane diagnostics still deferred"
          "-DFORBIDDEN_SNIPPETS=function myprintf|function addlm123"
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_dump_flag_case.cmake"
)
set_tests_properties(backend_cli_dump_mir_focus_function_filters_00204 PROPERTIES
  LABELS "backend;internal;cli"
)
c4c_disable_backend_test_when_x86_tests_off(backend_cli_dump_mir_focus_function_filters_00204)

add_test(
  NAME backend_cli_trace_mir_focus_function_filters_00204
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DTARGET_TRIPLE=x86_64-unknown-linux-gnu
          -DDUMP_FLAG=--trace-mir
          "-DEXTRA_ARGS=--mir-focus-function;stdarg"
          -DSRC=${PROJECT_SOURCE_DIR}/tests/c/external/c-testsuite/src/00204.c
          "-DREQUIRED_SNIPPETS=x86 route trace|focus function: stdarg|route owner: x86/debug|module emitter: x86/module|- function stdarg: 1 blocks, return type void|status: contract-first debug surface; structured lane diagnostics still deferred"
          "-DFORBIDDEN_SNIPPETS=function myprintf|function addlm123"
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_dump_flag_case.cmake"
)
set_tests_properties(backend_cli_trace_mir_focus_function_filters_00204 PROPERTIES
  LABELS "backend;internal;cli"
)
c4c_disable_backend_test_when_x86_tests_off(backend_cli_trace_mir_focus_function_filters_00204)

add_test(
  NAME backend_cli_dump_mir_focus_block_entry_00204
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DTARGET_TRIPLE=x86_64-unknown-linux-gnu
          -DDUMP_FLAG=--dump-mir
          "-DEXTRA_ARGS=--mir-focus-function;stdarg;--mir-focus-block;entry"
          -DSRC=${PROJECT_SOURCE_DIR}/tests/c/external/c-testsuite/src/00204.c
          "-DREQUIRED_SNIPPETS=x86 route summary|focus function: stdarg|focus block: entry|route owner: x86/debug|module emitter: x86/module|status: contract-first debug surface; structured lane diagnostics still deferred"
          "-DFORBIDDEN_SNIPPETS=function myprintf|function addlm123"
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_dump_flag_case.cmake"
)
set_tests_properties(backend_cli_dump_mir_focus_block_entry_00204 PROPERTIES
  LABELS "backend;internal;cli"
)
c4c_disable_backend_test_when_x86_tests_off(backend_cli_dump_mir_focus_block_entry_00204)

add_test(
  NAME backend_cli_trace_mir_focus_block_entry_00204
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DTARGET_TRIPLE=x86_64-unknown-linux-gnu
          -DDUMP_FLAG=--trace-mir
          "-DEXTRA_ARGS=--mir-focus-function;stdarg;--mir-focus-block;entry"
          -DSRC=${PROJECT_SOURCE_DIR}/tests/c/external/c-testsuite/src/00204.c
          "-DREQUIRED_SNIPPETS=x86 route trace|focus function: stdarg|focus block: entry|route owner: x86/debug|module emitter: x86/module|- function stdarg: 1 blocks, return type void|status: contract-first debug surface; structured lane diagnostics still deferred"
          "-DFORBIDDEN_SNIPPETS=function myprintf|function addlm123"
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_dump_flag_case.cmake"
)
set_tests_properties(backend_cli_trace_mir_focus_block_entry_00204 PROPERTIES
  LABELS "backend;internal;cli"
)
c4c_disable_backend_test_when_x86_tests_off(backend_cli_trace_mir_focus_block_entry_00204)

add_test(
  NAME backend_cli_dump_mir_focus_block_missing_00204
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DTARGET_TRIPLE=x86_64-unknown-linux-gnu
          -DDUMP_FLAG=--dump-mir
          "-DEXTRA_ARGS=--mir-focus-function;stdarg;--mir-focus-block;missing.block"
          -DSRC=${PROJECT_SOURCE_DIR}/tests/c/external/c-testsuite/src/00204.c
          "-DREQUIRED_SNIPPETS=x86 route summary|focus function: stdarg|focus block: missing.block|route owner: x86/debug|module emitter: x86/module|status: contract-first debug surface; structured lane diagnostics still deferred"
          "-DFORBIDDEN_SNIPPETS=function myprintf|function addlm123"
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_dump_flag_case.cmake"
)
set_tests_properties(backend_cli_dump_mir_focus_block_missing_00204 PROPERTIES
  LABELS "backend;internal;cli"
)
c4c_disable_backend_test_when_x86_tests_off(backend_cli_dump_mir_focus_block_missing_00204)

add_test(
  NAME backend_cli_trace_mir_focus_block_missing_00204
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DTARGET_TRIPLE=x86_64-unknown-linux-gnu
          -DDUMP_FLAG=--trace-mir
          "-DEXTRA_ARGS=--mir-focus-function;stdarg;--mir-focus-block;missing.block"
          -DSRC=${PROJECT_SOURCE_DIR}/tests/c/external/c-testsuite/src/00204.c
          "-DREQUIRED_SNIPPETS=x86 route trace|focus function: stdarg|focus block: missing.block|route owner: x86/debug|module emitter: x86/module|- function stdarg: 1 blocks, return type void|status: contract-first debug surface; structured lane diagnostics still deferred"
          "-DFORBIDDEN_SNIPPETS=function myprintf|function addlm123"
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_dump_flag_case.cmake"
)
set_tests_properties(backend_cli_trace_mir_focus_block_missing_00204 PROPERTIES
  LABELS "backend;internal;cli"
)
c4c_disable_backend_test_when_x86_tests_off(backend_cli_trace_mir_focus_block_missing_00204)

add_test(
  NAME backend_cli_dump_mir_focus_value_t1_00204
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DTARGET_TRIPLE=x86_64-unknown-linux-gnu
          -DDUMP_FLAG=--dump-mir
          "-DEXTRA_ARGS=--mir-focus-function;stdarg;--mir-focus-value;t1"
          -DSRC=${PROJECT_SOURCE_DIR}/tests/c/external/c-testsuite/src/00204.c
          "-DREQUIRED_SNIPPETS=x86 route summary|focus function: stdarg|focus value: %t1|route owner: x86/debug|module emitter: x86/module|status: contract-first debug surface; structured lane diagnostics still deferred"
          "-DFORBIDDEN_SNIPPETS=function myprintf|function addlm123|no prepared value in the focused function matched the requested MIR value focus"
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_dump_flag_case.cmake"
)
set_tests_properties(backend_cli_dump_mir_focus_value_t1_00204 PROPERTIES
  LABELS "backend;internal;cli"
)
c4c_disable_backend_test_when_x86_tests_off(backend_cli_dump_mir_focus_value_t1_00204)

add_test(
  NAME backend_cli_dump_mir_focus_value_missing_00204
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DTARGET_TRIPLE=x86_64-unknown-linux-gnu
          -DDUMP_FLAG=--dump-mir
          "-DEXTRA_ARGS=--mir-focus-function;stdarg;--mir-focus-value;missing"
          -DSRC=${PROJECT_SOURCE_DIR}/tests/c/external/c-testsuite/src/00204.c
          "-DREQUIRED_SNIPPETS=x86 route summary|focus function: stdarg|focus value: %missing|route owner: x86/debug|module emitter: x86/module|status: contract-first debug surface; structured lane diagnostics still deferred"
          "-DFORBIDDEN_SNIPPETS=function myprintf|function addlm123"
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_dump_flag_case.cmake"
)
set_tests_properties(backend_cli_dump_mir_focus_value_missing_00204 PROPERTIES
  LABELS "backend;internal;cli"
)
c4c_disable_backend_test_when_x86_tests_off(backend_cli_dump_mir_focus_value_missing_00204)

add_test(
  NAME backend_cli_trace_mir_focus_value_t1_00204
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DTARGET_TRIPLE=x86_64-unknown-linux-gnu
          -DDUMP_FLAG=--trace-mir
          "-DEXTRA_ARGS=--mir-focus-function;stdarg;--mir-focus-value;t1"
          -DSRC=${PROJECT_SOURCE_DIR}/tests/c/external/c-testsuite/src/00204.c
          "-DREQUIRED_SNIPPETS=x86 route trace|focus function: stdarg|focus value: %t1|route owner: x86/debug|module emitter: x86/module|- function stdarg: 1 blocks, return type void|status: contract-first debug surface; structured lane diagnostics still deferred"
          "-DFORBIDDEN_SNIPPETS=function myprintf|function addlm123|no prepared value in the focused function matched the requested MIR value focus"
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_dump_flag_case.cmake"
)
set_tests_properties(backend_cli_trace_mir_focus_value_t1_00204 PROPERTIES
  LABELS "backend;internal;cli"
)
c4c_disable_backend_test_when_x86_tests_off(backend_cli_trace_mir_focus_value_t1_00204)

add_test(
  NAME backend_cli_trace_mir_focus_value_missing_00204
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DTARGET_TRIPLE=x86_64-unknown-linux-gnu
          -DDUMP_FLAG=--trace-mir
          "-DEXTRA_ARGS=--mir-focus-function;stdarg;--mir-focus-value;missing"
          -DSRC=${PROJECT_SOURCE_DIR}/tests/c/external/c-testsuite/src/00204.c
          "-DREQUIRED_SNIPPETS=x86 route trace|focus function: stdarg|focus value: %missing|route owner: x86/debug|module emitter: x86/module|- function stdarg: 1 blocks, return type void|status: contract-first debug surface; structured lane diagnostics still deferred"
          "-DFORBIDDEN_SNIPPETS=function myprintf|function addlm123"
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_dump_flag_case.cmake"
)
set_tests_properties(backend_cli_trace_mir_focus_value_missing_00204 PROPERTIES
  LABELS "backend;internal;cli"
)
c4c_disable_backend_test_when_x86_tests_off(backend_cli_trace_mir_focus_value_missing_00204)
