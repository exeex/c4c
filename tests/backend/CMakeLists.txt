set(BACKEND_TEST_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(BACKEND_TEST_BINARY_ROOT "${CMAKE_CURRENT_BINARY_DIR}")
set(BACKEND_TEST_CMAKE_ROOT "${BACKEND_TEST_ROOT}/cmake")
set(BACKEND_CASE_ROOT "${BACKEND_TEST_ROOT}/case")
find_program(AARCH64_AS_EXECUTABLE NAMES aarch64-linux-gnu-as)
find_program(QEMU_RISCV64_EXECUTABLE NAMES qemu-riscv64)

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

function(c4c_add_backend_codegen_route_test test_name)
  set(options)
  set(one_value_args SRC TARGET_TRIPLE OUT_TEXT REQUIRED_SNIPPETS FORBIDDEN_SNIPPETS
                     EXTRA_COMPILER_ARGS CASE_TIMEOUT_SEC)
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
            "-DCASE_TIMEOUT_SEC=${ARG_CASE_TIMEOUT_SEC}"
            -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_codegen_route_case.cmake"
  )
  c4c_set_backend_test_labels("${test_name}" backend_route ${ARG_LABELS})
endfunction()

function(c4c_add_backend_codegen_failure_test test_name)
  set(options)
  set(one_value_args SRC TARGET_TRIPLE OUT_TEXT EXPECTED_DIAGNOSTICS CASE_TIMEOUT_SEC)
  set(multi_value_args LABELS)
  cmake_parse_arguments(ARG "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

  if(ARG_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "c4c_add_backend_codegen_failure_test(${test_name}) got unexpected arguments: ${ARG_UNPARSED_ARGUMENTS}")
  endif()

  foreach(required_arg IN ITEMS SRC TARGET_TRIPLE OUT_TEXT EXPECTED_DIAGNOSTICS)
    if(NOT DEFINED ARG_${required_arg} OR "${ARG_${required_arg}}" STREQUAL "")
      message(FATAL_ERROR
        "c4c_add_backend_codegen_failure_test(${test_name}) missing ${required_arg}")
    endif()
  endforeach()

  add_test(
    NAME "${test_name}"
    COMMAND "${CMAKE_COMMAND}"
            -DCOMPILER=$<TARGET_FILE:c4cll>
            -DSRC=${ARG_SRC}
            -DTARGET_TRIPLE=${ARG_TARGET_TRIPLE}
            -DOUT_TEXT=${ARG_OUT_TEXT}
            "-DEXPECTED_DIAGNOSTICS=${ARG_EXPECTED_DIAGNOSTICS}"
            "-DCASE_TIMEOUT_SEC=${ARG_CASE_TIMEOUT_SEC}"
            -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_codegen_failure_case.cmake"
  )
  c4c_set_backend_test_labels("${test_name}" backend_route expected_failure ${ARG_LABELS})
endfunction()

function(c4c_add_backend_codegen_object_test test_name)
  set(options)
  set(one_value_args SRC TARGET_TRIPLE OUT_OBJECT EXPECTED_MACHINE_LE EXPECTED_HEX_CONTAINS
                     CASE_TIMEOUT_SEC)
  set(multi_value_args LABELS)
  cmake_parse_arguments(ARG "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

  if(ARG_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "c4c_add_backend_codegen_object_test(${test_name}) got unexpected arguments: ${ARG_UNPARSED_ARGUMENTS}")
  endif()

  foreach(required_arg IN ITEMS SRC TARGET_TRIPLE OUT_OBJECT EXPECTED_MACHINE_LE)
    if(NOT DEFINED ARG_${required_arg} OR "${ARG_${required_arg}}" STREQUAL "")
      message(FATAL_ERROR
        "c4c_add_backend_codegen_object_test(${test_name}) missing ${required_arg}")
    endif()
  endforeach()

  add_test(
    NAME "${test_name}"
    COMMAND "${CMAKE_COMMAND}"
            -DCOMPILER=$<TARGET_FILE:c4cll>
            -DSRC=${ARG_SRC}
            -DTARGET_TRIPLE=${ARG_TARGET_TRIPLE}
            -DOUT_OBJECT=${ARG_OUT_OBJECT}
            -DEXPECTED_MACHINE_LE=${ARG_EXPECTED_MACHINE_LE}
            "-DEXPECTED_HEX_CONTAINS=${ARG_EXPECTED_HEX_CONTAINS}"
            "-DCASE_TIMEOUT_SEC=${ARG_CASE_TIMEOUT_SEC}"
            -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_codegen_object_case.cmake"
  )
  c4c_set_backend_test_labels("${test_name}" backend_route object ${ARG_LABELS})
endfunction()

function(c4c_add_backend_codegen_object_failure_test test_name)
  set(options NO_OUTPUT_ARG)
  set(one_value_args SRC TARGET_TRIPLE OUT_OBJECT EXPECTED_DIAGNOSTICS CASE_TIMEOUT_SEC)
  set(multi_value_args LABELS)
  cmake_parse_arguments(ARG "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

  if(ARG_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "c4c_add_backend_codegen_object_failure_test(${test_name}) got unexpected arguments: ${ARG_UNPARSED_ARGUMENTS}")
  endif()

  foreach(required_arg IN ITEMS SRC TARGET_TRIPLE EXPECTED_DIAGNOSTICS)
    if(NOT DEFINED ARG_${required_arg} OR "${ARG_${required_arg}}" STREQUAL "")
      message(FATAL_ERROR
        "c4c_add_backend_codegen_object_failure_test(${test_name}) missing ${required_arg}")
    endif()
  endforeach()

  if(NOT ARG_NO_OUTPUT_ARG AND (NOT DEFINED ARG_OUT_OBJECT OR "${ARG_OUT_OBJECT}" STREQUAL ""))
    message(FATAL_ERROR
      "c4c_add_backend_codegen_object_failure_test(${test_name}) missing OUT_OBJECT")
  endif()

  set(output_object_arg)
  if(NOT ARG_NO_OUTPUT_ARG)
    set(output_object_arg -DOUT_OBJECT=${ARG_OUT_OBJECT})
  endif()

  add_test(
    NAME "${test_name}"
    COMMAND "${CMAKE_COMMAND}"
            -DCOMPILER=$<TARGET_FILE:c4cll>
            -DSRC=${ARG_SRC}
            -DTARGET_TRIPLE=${ARG_TARGET_TRIPLE}
            ${output_object_arg}
            "-DEXPECTED_DIAGNOSTICS=${ARG_EXPECTED_DIAGNOSTICS}"
            "-DCASE_TIMEOUT_SEC=${ARG_CASE_TIMEOUT_SEC}"
            -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_codegen_object_failure_case.cmake"
  )
  c4c_set_backend_test_labels("${test_name}" backend_route object expected_failure ${ARG_LABELS})
endfunction()

function(c4c_add_backend_dump_flag_test test_name)
  set(options)
  set(one_value_args SRC TARGET_TRIPLE DUMP_FLAG REQUIRED_SNIPPETS FORBIDDEN_SNIPPETS
                     EXTRA_ARGS CASE_TIMEOUT_SEC)
  set(multi_value_args LABELS)
  cmake_parse_arguments(ARG "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

  if(ARG_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "c4c_add_backend_dump_flag_test(${test_name}) got unexpected arguments: ${ARG_UNPARSED_ARGUMENTS}")
  endif()

  foreach(required_arg IN ITEMS SRC TARGET_TRIPLE DUMP_FLAG)
    if(NOT DEFINED ARG_${required_arg} OR "${ARG_${required_arg}}" STREQUAL "")
      message(FATAL_ERROR
        "c4c_add_backend_dump_flag_test(${test_name}) missing ${required_arg}")
    endif()
  endforeach()

  add_test(
    NAME "${test_name}"
    COMMAND "${CMAKE_COMMAND}"
            -DCOMPILER=$<TARGET_FILE:c4cll>
            -DSRC=${ARG_SRC}
            -DTARGET_TRIPLE=${ARG_TARGET_TRIPLE}
            -DDUMP_FLAG=${ARG_DUMP_FLAG}
            "-DREQUIRED_SNIPPETS=${ARG_REQUIRED_SNIPPETS}"
            "-DFORBIDDEN_SNIPPETS=${ARG_FORBIDDEN_SNIPPETS}"
            "-DEXTRA_ARGS=${ARG_EXTRA_ARGS}"
            "-DCASE_TIMEOUT_SEC=${ARG_CASE_TIMEOUT_SEC}"
            -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_dump_flag_case.cmake"
  )
  c4c_set_backend_test_labels("${test_name}" backend_dump ${ARG_LABELS})
endfunction()

c4c_add_backend_codegen_route_test(
  backend_codegen_route_aarch64_prepared_call_boundary_scalability
  SRC "${BACKEND_CASE_ROOT}/aarch64_prepared_call_boundary_scalability.c"
  TARGET_TRIPLE aarch64-linux-gnu
  OUT_TEXT "${BACKEND_TEST_BINARY_ROOT}/aarch64_prepared_call_boundary_scalability.s"
  REQUIRED_SNIPPETS ".text"
  CASE_TIMEOUT_SEC 5
  LABELS aarch64 scalability prepared_call_boundary
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_aarch64_alu_unpublished_load_local_after_call
  SRC "${BACKEND_CASE_ROOT}/aarch64_alu_unpublished_load_local_after_call.c"
  TARGET_TRIPLE aarch64-linux-gnu
  OUT_TEXT "${BACKEND_TEST_BINARY_ROOT}/aarch64_alu_unpublished_load_local_after_call.s"
  REQUIRED_SNIPPETS ".text"
  CASE_TIMEOUT_SEC 5
  LABELS aarch64 alu prepared_authority probe
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_aarch64_alu_unpublished_load_local_call_boundary
  SRC "${BACKEND_CASE_ROOT}/aarch64_alu_unpublished_load_local_call_boundary.c"
  TARGET_TRIPLE aarch64-linux-gnu
  OUT_TEXT "${BACKEND_TEST_BINARY_ROOT}/aarch64_alu_unpublished_load_local_call_boundary.s"
  REQUIRED_SNIPPETS ".text"
  CASE_TIMEOUT_SEC 5
  LABELS aarch64 alu prepared_authority probe
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_aarch64_pointer_select_aggregate_byte_copy
  SRC "${BACKEND_CASE_ROOT}/aarch64_pointer_select_aggregate_byte_copy.c"
  TARGET_TRIPLE aarch64-linux-gnu
  OUT_TEXT "${BACKEND_TEST_BINARY_ROOT}/aarch64_pointer_select_aggregate_byte_copy.s"
  REQUIRED_SNIPPETS ".text"
  CASE_TIMEOUT_SEC 5
  LABELS aarch64 dispatch_value_materialization aggregate_copy probe
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_aarch64_byval_global_payload_call_boundary
  SRC "${BACKEND_CASE_ROOT}/aarch64_byval_global_payload_call_boundary.c"
  TARGET_TRIPLE aarch64-linux-gnu
  OUT_TEXT "${BACKEND_TEST_BINARY_ROOT}/aarch64_byval_global_payload_call_boundary.s"
  REQUIRED_SNIPPETS "call_pair:|global_pair|adrp x9, global_pair|add x9, x9, :lo12:global_pair|ldrb w0, [x9]|orr x0, x0, x9, lsl #8|bl take_pair"
  FORBIDDEN_SNIPPETS "ldrb w0, [sp|ldr w0, [sp|ldr x0, [sp"
  CASE_TIMEOUT_SEC 5
  LABELS aarch64 byval prepared_call_boundary probe
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_aarch64_byval_global_payload_address_call_boundary
  SRC "${BACKEND_CASE_ROOT}/aarch64_byval_global_payload_address_call_boundary.c"
  TARGET_TRIPLE aarch64-linux-gnu
  OUT_TEXT "${BACKEND_TEST_BINARY_ROOT}/aarch64_byval_global_payload_address_call_boundary.s"
  REQUIRED_SNIPPETS "arg:|bl fa_s16|adrp x10, s17|ldrb w9, [x10]|strb w9, [sp, #136]|adrp x10, s17+16|add x10, x10, :lo12:s17+16|ldrb w9, [x10]|strb w9, [sp, #152]|add x0, sp, #136|bl fa_s17"
  FORBIDDEN_SNIPPETS "mov x0, x20"
  CASE_TIMEOUT_SEC 5
  LABELS aarch64 byval prepared_call_boundary aggregate_address probe
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_aarch64_byval_payload_8_to_13_stack_overflow
  SRC "${BACKEND_CASE_ROOT}/byval_helper_payload_8_to_13.c"
  TARGET_TRIPLE aarch64-linux-gnu
  OUT_TEXT "${BACKEND_TEST_BINARY_ROOT}/aarch64_byval_payload_8_to_13_stack_overflow.s"
  REQUIRED_SNIPPETS "main:|sub sp, sp, #32|mov x16, sp|ldrb w9, [sp, #70]|strb w9, [x16]|ldrb w9, [sp, #82]|strb w9, [x16, #16]|bl fa1|add sp, sp, #32"
  CASE_TIMEOUT_SEC 5
  LABELS aarch64 byval prepared_call_boundary aggregate_stack probe
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_aarch64_byval_payload_9_to_14_stack_overflow
  SRC "${BACKEND_CASE_ROOT}/byval_helper_payload_9_to_14.c"
  TARGET_TRIPLE aarch64-linux-gnu
  OUT_TEXT "${BACKEND_TEST_BINARY_ROOT}/aarch64_byval_payload_9_to_14_stack_overflow.s"
  REQUIRED_SNIPPETS "main:|sub sp, sp, #32|mov x16, sp|ldrb w9, [sp, #74]|strb w9, [x16]|ldrb w9, [sp, #87]|strb w9, [x16, #16]|bl fa2|add sp, sp, #32"
  CASE_TIMEOUT_SEC 5
  LABELS aarch64 byval prepared_call_boundary aggregate_stack probe
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_aarch64_hfa_global_payload_call_boundary
  SRC "${BACKEND_CASE_ROOT}/aarch64_hfa_global_payload_call_boundary.c"
  TARGET_TRIPLE aarch64-linux-gnu
  OUT_TEXT "${BACKEND_TEST_BINARY_ROOT}/aarch64_hfa_global_payload_call_boundary.s"
  REQUIRED_SNIPPETS "call_hfa_globals:|adrp x9, global_float2|ldr s0, [x9]|adrp x9, global_float2+4|add x9, x9, :lo12:global_float2+4|ldr s1, [x9]|bl take_float2|adrp x9, global_double2|ldr d0, [x9]|adrp x9, global_double2+8|add x9, x9, :lo12:global_double2+8|ldr d1, [x9]|bl take_double2"
  FORBIDDEN_SNIPPETS "fmov s0, s13|fmov s1, s|fmov d0, d13|fmov d1, d"
  CASE_TIMEOUT_SEC 5
  LABELS aarch64 hfa prepared_call_boundary scalar_fp probe
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_aarch64_hfa_global_payload_return
  SRC "${BACKEND_CASE_ROOT}/aarch64_hfa_global_payload_return.c"
  TARGET_TRIPLE aarch64-linux-gnu
  OUT_TEXT "${BACKEND_TEST_BINARY_ROOT}/aarch64_hfa_global_payload_return.s"
  REQUIRED_SNIPPETS "return_float2_global:|adrp x9, global_return_float2|add x9, x9, :lo12:global_return_float2|ldr s13, [x9]|adrp x9, global_return_float2+4|add x9, x9, :lo12:global_return_float2+4|ldr s13, [x9]|ldr s20, [sp, #4]|fmov s0, s13|fmov s1, s20|return_double2_global:|adrp x9, global_return_double2|add x9, x9, :lo12:global_return_double2|ldr d13, [x9]|adrp x9, global_return_double2+8|add x9, x9, :lo12:global_return_double2+8|ldr d13, [x9]|ldr d20, [sp, #8]|fmov d0, d13|fmov d1, d20"
  CASE_TIMEOUT_SEC 5
  LABELS aarch64 hfa aggregate_return scalar_fp probe
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_aarch64_f128_hfa_global_payload_call_boundary
  SRC "${BACKEND_CASE_ROOT}/aarch64_f128_hfa_global_payload_call_boundary.c"
  TARGET_TRIPLE aarch64-linux-gnu
  OUT_TEXT "${BACKEND_TEST_BINARY_ROOT}/aarch64_f128_hfa_global_payload_call_boundary.s"
  REQUIRED_SNIPPETS "call_f128_hfa_globals:|adrp x9, global_f128_2|ldr q0, [x9]|adrp x9, global_f128_2+16|add x9, x9, :lo12:global_f128_2+16|ldr q1, [x9]|bl take_f128_2"
  FORBIDDEN_SNIPPETS "mov q0, q13|mov q1, q|mov v0.16b, v13.16b|mov v1.16b, v"
  CASE_TIMEOUT_SEC 5
  LABELS aarch64 hfa prepared_call_boundary f128 probe
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_aarch64_variadic_aggregate_overflow_byte_copy
  SRC "${BACKEND_CASE_ROOT}/aarch64_variadic_aggregate_overflow_byte_copy.c"
  TARGET_TRIPLE aarch64-linux-gnu
  OUT_TEXT "${BACKEND_TEST_BINARY_ROOT}/aarch64_variadic_aggregate_overflow_byte_copy.s"
  REQUIRED_SNIPPETS ".text"
  CASE_TIMEOUT_SEC 5
  LABELS aarch64 dispatch_value_materialization variadic aggregate_copy probe
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_aarch64_got_load_global_prepared_memory
  SRC "${BACKEND_CASE_ROOT}/aarch64_got_load_global_prepared_memory.c"
  TARGET_TRIPLE aarch64-linux-gnu
  OUT_TEXT "${BACKEND_TEST_BINARY_ROOT}/aarch64_got_load_global_prepared_memory.s"
  REQUIRED_SNIPPETS ":got:external_data_symbol|:got_lo12:external_data_symbol|ldr w"
  FORBIDDEN_SNIPPETS ":lo12:external_data_symbol"
  CASE_TIMEOUT_SEC 5
  LABELS aarch64 dispatch_value_materialization load_global got probe
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_aarch64_store_global_stack_publication
  SRC "${BACKEND_CASE_ROOT}/aarch64_store_global_stack_publication.c"
  TARGET_TRIPLE aarch64-linux-gnu
  OUT_TEXT "${BACKEND_TEST_BINARY_ROOT}/aarch64_store_global_stack_publication.s"
  REQUIRED_SNIPPETS ".text|published_global|adrp x|:lo12:published_global|str w|ldr w|sp"
  CASE_TIMEOUT_SEC 5
  LABELS aarch64 store_global stack_publication probe
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_aarch64_sret_global_scalar_source_publication
  SRC "${BACKEND_CASE_ROOT}/aarch64_sret_global_scalar_source_publication.c"
  TARGET_TRIPLE aarch64-linux-gnu
  OUT_TEXT "${BACKEND_TEST_BINARY_ROOT}/aarch64_sret_global_scalar_source_publication.s"
  REQUIRED_SNIPPETS "return_global_one_byte:|adrp x9, global_one_byte|add x9, x9, :lo12:global_one_byte|ldrb w13, [x9]|strb w13, [sp, #1]|strb w13, [x8]"
  CASE_TIMEOUT_SEC 5
  LABELS aarch64 sret dispatch_value_materialization aggregate_return probe
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_aarch64_large_stack_global_byte_publication
  SRC "${BACKEND_CASE_ROOT}/aarch64_large_stack_global_byte_publication.c"
  TARGET_TRIPLE aarch64-linux-gnu
  OUT_TEXT "${BACKEND_TEST_BINARY_ROOT}/aarch64_large_stack_global_byte_publication.s"
  REQUIRED_SNIPPETS "large_stack_global_byte_publication:|adrp x9, large_stack_publication_source|add x9, x9, :lo12:large_stack_publication_source|ldrb w13, [x9]|movz x9, #8112|add x9, sp, x9|strb w13, [x9]|bl consume_large_stack_publication"
  FORBIDDEN_SNIPPETS "strb w9, [sp, #8112]|strb w13, [sp, #8112]"
  CASE_TIMEOUT_SEC 5
  LABELS aarch64 dispatch_value_materialization stack_publication probe
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_riscv64_external_no_storage_main_emits_return_path
  SRC "${PROJECT_SOURCE_DIR}/tests/c/external/c-testsuite/src/00094.c"
  TARGET_TRIPLE riscv64-linux-gnu
  OUT_TEXT "${BACKEND_TEST_BINARY_ROOT}/riscv64_external_00094_no_storage_main.s"
  REQUIRED_SNIPPETS ".globl main|main:|li a0, 0|ret"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 extern_global no_storage output_contract
)

c4c_add_backend_codegen_object_test(
  backend_cli_riscv64_return_zero_writes_elf_obj
  SRC "${BACKEND_CASE_ROOT}/aarch64_return_zero_smoke.c"
  TARGET_TRIPLE riscv64-linux-gnu
  OUT_OBJECT "${BACKEND_TEST_BINARY_ROOT}/riscv64_return_zero_smoke.o"
  EXPECTED_MACHINE_LE f300
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 object output_contract dual-route cli
)

c4c_add_backend_codegen_object_test(
  backend_cli_riscv64_vrm_insn_d_source_obj
  SRC "${BACKEND_CASE_ROOT}/riscv64_vrm_insn_d_source.c"
  TARGET_TRIPLE riscv64-linux-gnu
  OUT_OBJECT "${BACKEND_TEST_BINARY_ROOT}/riscv64_vrm_insn_d_source.o"
  EXPECTED_MACHINE_LE f300
  EXPECTED_HEX_CONTAINS "3f0320140b0403001305000067800000"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 object inline_asm vrm insn_d cli
)

add_test(
  NAME backend_c4c_as_parse_suite
  COMMAND "${CMAKE_COMMAND}"
          -DC4C_AS=$<TARGET_FILE:c4c-as>
          -DC4CLL=$<TARGET_FILE:c4cll>
          -DSOURCE_CASE=${BACKEND_CASE_ROOT}/riscv64_vrm_insn_d_source.c
          -DWORK_DIR=${BACKEND_TEST_BINARY_ROOT}/c4c_as_suite
          -DCASE_TIMEOUT_SEC=10
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_c4c_as_parse_suite.cmake"
)
c4c_set_backend_test_labels(
  backend_c4c_as_parse_suite
  riscv64 asm cli c4c_as parser
)

add_test(
  NAME backend_c4c_objdump_extract_suite
  COMMAND "${CMAKE_COMMAND}"
          -DC4C_OBJDUMP=$<TARGET_FILE:c4c-objdump>
          -DC4C_AS=$<TARGET_FILE:c4c-as>
          -DC4CLL=$<TARGET_FILE:c4cll>
          -DSOURCE_CASE=${BACKEND_CASE_ROOT}/riscv64_vrm_insn_d_source.c
          -DAARCH64_SOURCE_CASE=${BACKEND_CASE_ROOT}/aarch64_return_zero_smoke.c
          -DWORK_DIR=${BACKEND_TEST_BINARY_ROOT}/c4c_objdump_suite
          -DCASE_TIMEOUT_SEC=10
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_c4c_objdump_extract_suite.cmake"
)
c4c_set_backend_test_labels(
  backend_c4c_objdump_extract_suite
  riscv64 objdump cli c4c_objdump extractor
)

add_test(
  NAME backend_rv64_roundtrip_contract
  COMMAND "${CMAKE_COMMAND}"
          -DC4C_AS=$<TARGET_FILE:c4c-as>
          -DC4C_OBJDUMP=$<TARGET_FILE:c4c-objdump>
          -DCORPUS=${BACKEND_TEST_ROOT}/rv64/rv64i_ev64_roundtrip_corpus.s
          -DCONTRACT=${BACKEND_TEST_ROOT}/rv64/rv64i_ev64_roundtrip_contract.md
          -DWORK_DIR=${BACKEND_TEST_BINARY_ROOT}/rv64_roundtrip_contract
          -DCASE_TIMEOUT_SEC=10
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_rv64_roundtrip_contract.cmake"
)
c4c_set_backend_test_labels(
  backend_rv64_roundtrip_contract
  riscv64 rv64 asm objdump cli c4c_as c4c_objdump roundtrip contract
)

add_custom_target(rv64_c_testsuite_asm_roundtrip_scan
  COMMAND "${CMAKE_COMMAND}"
          -DC4CLL=$<TARGET_FILE:c4cll>
          -DC4C_AS=$<TARGET_FILE:c4c-as>
          -DC4C_OBJDUMP=$<TARGET_FILE:c4c-objdump>
          -DROOT=${PROJECT_SOURCE_DIR}
          -DC_TESTSUITE_ROOT=${PROJECT_SOURCE_DIR}/tests/c/external/c-testsuite
          -DALLOWLIST=${BACKEND_TEST_ROOT}/rv64/rv64_c_testsuite_asm_roundtrip_allowlist.txt
          -DWORK_DIR=${BACKEND_TEST_BINARY_ROOT}/rv64_c_testsuite_asm_roundtrip_scan
          -DCASE_TIMEOUT_SEC=10
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_rv64_c_testsuite_asm_roundtrip_scan.cmake"
  DEPENDS c4cll c4c-as c4c-objdump
  VERBATIM
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_riscv64_vrm_insn_d_source_asm
  SRC "${BACKEND_CASE_ROOT}/riscv64_vrm_insn_d_source.c"
  TARGET_TRIPLE riscv64-linux-gnu
  OUT_TEXT "${BACKEND_TEST_BINARY_ROOT}/riscv64_vrm_insn_d_source.s"
  REQUIRED_SNIPPETS ".globl main|main:|.insn.d 10, 11, v6, v0, v2, v4, 3|li a0, 0|ret"
  FORBIDDEN_SNIPPETS "llvm.inline_asm|unsupported_external_call"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 asm inline_asm vrm insn_d cli
)

c4c_add_backend_codegen_object_test(
  backend_cli_aarch64_return_zero_writes_elf_obj
  SRC "${BACKEND_CASE_ROOT}/aarch64_return_zero_smoke.c"
  TARGET_TRIPLE aarch64-linux-gnu
  OUT_OBJECT "${BACKEND_TEST_BINARY_ROOT}/aarch64_return_zero_smoke.o"
  EXPECTED_MACHINE_LE b700
  CASE_TIMEOUT_SEC 10
  LABELS aarch64 object output_contract dual-route cli
)

c4c_add_backend_codegen_object_test(
  backend_cli_aarch64_return_add_writes_elf_obj
  SRC "${BACKEND_CASE_ROOT}/return_add.c"
  TARGET_TRIPLE aarch64-linux-gnu
  OUT_OBJECT "${BACKEND_TEST_BINARY_ROOT}/aarch64_return_add.o"
  EXPECTED_MACHINE_LE b700
  CASE_TIMEOUT_SEC 10
  LABELS aarch64 object output_contract dual-route cli scalar scan
)

c4c_add_backend_codegen_object_test(
  backend_cli_aarch64_two_arg_helper_writes_elf_obj
  SRC "${BACKEND_CASE_ROOT}/two_arg_helper.c"
  TARGET_TRIPLE aarch64-linux-gnu
  OUT_OBJECT "${BACKEND_TEST_BINARY_ROOT}/aarch64_two_arg_helper.o"
  EXPECTED_MACHINE_LE b700
  CASE_TIMEOUT_SEC 10
  LABELS aarch64 object output_contract dual-route cli scalar scan
)

c4c_add_backend_codegen_object_test(
  backend_cli_aarch64_return_add_sub_chain_writes_elf_obj
  SRC "${BACKEND_CASE_ROOT}/return_add_sub_chain.c"
  TARGET_TRIPLE aarch64-linux-gnu
  OUT_OBJECT "${BACKEND_TEST_BINARY_ROOT}/aarch64_return_add_sub_chain.o"
  EXPECTED_MACHINE_LE b700
  CASE_TIMEOUT_SEC 10
  LABELS aarch64 object output_contract dual-route cli scalar scan
)

c4c_add_backend_codegen_object_test(
  backend_cli_aarch64_cts_00001_writes_elf_obj
  SRC "${PROJECT_SOURCE_DIR}/tests/c/external/c-testsuite/src/00001.c"
  TARGET_TRIPLE aarch64-linux-gnu
  OUT_OBJECT "${BACKEND_TEST_BINARY_ROOT}/aarch64_cts_00001.o"
  EXPECTED_MACHINE_LE b700
  CASE_TIMEOUT_SEC 10
  LABELS aarch64 c_testsuite output_contract dual-route cli smoke scan
)

c4c_add_backend_codegen_object_test(
  backend_cli_aarch64_cts_00002_writes_elf_obj
  SRC "${PROJECT_SOURCE_DIR}/tests/c/external/c-testsuite/src/00002.c"
  TARGET_TRIPLE aarch64-linux-gnu
  OUT_OBJECT "${BACKEND_TEST_BINARY_ROOT}/aarch64_cts_00002.o"
  EXPECTED_MACHINE_LE b700
  CASE_TIMEOUT_SEC 10
  LABELS aarch64 c_testsuite output_contract dual-route cli smoke scan
)

c4c_add_backend_codegen_object_test(
  backend_cli_aarch64_cts_00011_writes_elf_obj
  SRC "${PROJECT_SOURCE_DIR}/tests/c/external/c-testsuite/src/00011.c"
  TARGET_TRIPLE aarch64-linux-gnu
  OUT_OBJECT "${BACKEND_TEST_BINARY_ROOT}/aarch64_cts_00011.o"
  EXPECTED_MACHINE_LE b700
  CASE_TIMEOUT_SEC 10
  LABELS aarch64 c_testsuite output_contract dual-route cli smoke scan
)

c4c_add_backend_codegen_object_test(
  backend_cli_aarch64_cts_00012_writes_elf_obj
  SRC "${PROJECT_SOURCE_DIR}/tests/c/external/c-testsuite/src/00012.c"
  TARGET_TRIPLE aarch64-linux-gnu
  OUT_OBJECT "${BACKEND_TEST_BINARY_ROOT}/aarch64_cts_00012.o"
  EXPECTED_MACHINE_LE b700
  CASE_TIMEOUT_SEC 10
  LABELS aarch64 c_testsuite output_contract dual-route cli smoke scan
)

c4c_add_backend_codegen_object_failure_test(
  backend_cli_x86_unsupported_target_diagnostic_obj
  SRC "${BACKEND_CASE_ROOT}/aarch64_return_zero_smoke.c"
  TARGET_TRIPLE x86_64-linux-gnu
  OUT_OBJECT "${BACKEND_TEST_BINARY_ROOT}/x86_return_zero_smoke.o"
  EXPECTED_DIAGNOSTICS "error: --codegen obj failed|backend object route unsupported target|x86_64-linux-gnu"
  CASE_TIMEOUT_SEC 10
  LABELS x86 object unsupported cli
)

c4c_add_backend_codegen_object_test(
  backend_cli_riscv64_unsupported_global_diagnostic_obj
  SRC "${BACKEND_CASE_ROOT}/riscv64_zero_aggregate_global_storage.c"
  TARGET_TRIPLE riscv64-linux-gnu
  OUT_OBJECT "${BACKEND_TEST_BINARY_ROOT}/riscv64_unsupported_global.o"
  EXPECTED_MACHINE_LE f300
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 object global cli
)

c4c_add_backend_codegen_object_test(
  backend_cli_riscv64_variadic_entry_missing_contract_obj
  SRC "${BACKEND_CASE_ROOT}/riscv64_variadic_entry_missing_contract.c"
  TARGET_TRIPLE riscv64-linux-gnu
  OUT_OBJECT "${BACKEND_TEST_BINARY_ROOT}/riscv64_variadic_entry_missing_contract.o"
  EXPECTED_MACHINE_LE f300
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 object variadic missing_contract cli
)

c4c_add_backend_dump_flag_test(
  backend_dump_riscv64_variadic_aggregate_overflow_helper_contract
  SRC "${BACKEND_CASE_ROOT}/riscv64_variadic_aggregate_overflow_helper_contract.c"
  TARGET_TRIPLE riscv64-linux-gnu
  DUMP_FLAG --dump-prepared-bir
  REQUIRED_SNIPPETS "prepared.func @pick_second_byte named_params=1 named_gp=1 named_fp=0 helpers=2|overflow_area required=yes|va_list_layout required=yes size=8 align=8 fields=1|field kind=overflow_arg_area offset=0 size=8|helper_resources scratch_registers=3 scratch_stack=0 helpers=[va_start,va_arg_aggregate]|helper_operand kind=va_start block=0 inst=0|dst_va_list=%lv.ap:stack_slot|dst_va_list_addr=%lv.ap:register:reg=s1|helper_operand kind=va_arg_aggregate block=0 inst=1|src_va_list=%lv.ap:register:reg=s1|aggregate_access_plan=source_class=overflow_arg_area:payload_size=9:payload_align=1|destination_payload=%t0/register:payload_write_address=%t0/frame_slot:slot=#19:offset=26:materialization_block=entry:materialization_inst=1|source_field=overflow_arg_area@0:source_payload_offset=0:source_slot=9:copy_size=9:copy_align=1:progression_field=overflow_arg_area@0:progression_stride=9:overflow_source_field=overflow_arg_area@0:overflow_stride=9|helper_operand kind=va_arg_aggregate block=0 inst=20|aggregate_dst=%t1:stack_slot|destination_payload=%t1/stack_slot:payload_write_address=%t1/frame_slot:slot=#28:offset=35:materialization_block=entry:materialization_inst=20|source_field=overflow_arg_area@0:source_payload_offset=0:source_slot=9:copy_size=9:copy_align=1:progression_field=overflow_arg_area@0:progression_stride=9:overflow_source_field=overflow_arg_area@0:overflow_stride=9"
  FORBIDDEN_SNIPPETS "destination_payload_slot|destination_payload_object"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 variadic aggregate va_arg prepared_printer helper_contract dump
)

c4c_add_backend_codegen_object_failure_test(
  backend_cli_riscv64_variadic_aggregate_overflow_helper_contract_obj
  SRC "${BACKEND_CASE_ROOT}/riscv64_variadic_aggregate_overflow_helper_contract.c"
  TARGET_TRIPLE riscv64-linux-gnu
  OUT_OBJECT "${BACKEND_TEST_BINARY_ROOT}/riscv64_variadic_aggregate_overflow_helper_contract.o"
  EXPECTED_DIAGNOSTICS "unsupported_variadic_helper_lowering: RV64 va_arg_aggregate helper requires payload_write_address in a supported frame slot"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 object variadic aggregate va_arg helper_contract expected_failure cli
)

c4c_add_backend_codegen_object_failure_test(
  backend_cli_riscv64_requires_output_path_obj
  SRC "${BACKEND_CASE_ROOT}/aarch64_return_zero_smoke.c"
  TARGET_TRIPLE riscv64-linux-gnu
  EXPECTED_DIAGNOSTICS "--codegen obj requires -o <path>"
  CASE_TIMEOUT_SEC 10
  NO_OUTPUT_ARG
  LABELS riscv64 object cli
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_riscv64_external_string_literal_strlen_direct_call
  SRC "${PROJECT_SOURCE_DIR}/tests/c/external/c-testsuite/src/00025.c"
  TARGET_TRIPLE riscv64-linux-gnu
  OUT_TEXT "${BACKEND_TEST_BINARY_ROOT}/riscv64_external_00025_string_literal_strlen.s"
  REQUIRED_SNIPPETS ".section .rodata|.str0:|.byte 104, 101, 108, 108, 111, 0|lla t0, .str0|sd t0, 0(sp)|ld t0, 0(sp)|mv a0, t0|call strlen|mv s2, a0|addi t0, s2, -5|mv a0, t0"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 extern_call string_literal direct_call output_contract
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_riscv64_external_strlen_runtime_link_policy
  SRC "${BACKEND_CASE_ROOT}/riscv64_external_strlen_runtime_link.c"
  TARGET_TRIPLE riscv64-linux-gnu
  OUT_TEXT "${BACKEND_TEST_BINARY_ROOT}/riscv64_external_strlen_runtime_link.s"
  REQUIRED_SNIPPETS ".section .rodata|call strlen|mv a0"
  FORBIDDEN_SNIPPETS "strlen:|.globl strlen"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 extern_call string_literal direct_call external_policy expected_repair
)

c4c_add_backend_codegen_failure_test(
  backend_codegen_route_riscv64_external_stdio_unsupported_variadic_diagnostic
  SRC "${BACKEND_CASE_ROOT}/riscv64_external_stdio_declaration_guard.c"
  TARGET_TRIPLE riscv64-linux-gnu
  OUT_TEXT "${BACKEND_TEST_BINARY_ROOT}/riscv64_external_stdio_declaration_guard.s"
  EXPECTED_DIAGNOSTICS "unsupported_external_call|callee='printf'|wrapper_kind=direct_extern_variadic|reason=variadic external calls"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 extern_call stdio declaration external_policy expected_repair
)

c4c_add_backend_codegen_failure_test(
  backend_codegen_route_riscv64_external_user_unresolved_diagnostic
  SRC "${BACKEND_CASE_ROOT}/riscv64_external_user_unresolved_call.c"
  TARGET_TRIPLE riscv64-linux-gnu
  OUT_TEXT "${BACKEND_TEST_BINARY_ROOT}/riscv64_external_user_unresolved_call.s"
  EXPECTED_DIAGNOSTICS "unsupported_external_call|callee='user_external_add'|wrapper_kind=direct_extern_fixed_arity|reason=unsupported runtime external symbol"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 extern_call user_external declaration external_policy expected_repair
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_riscv64_zero_aggregate_global_storage
  SRC "${BACKEND_CASE_ROOT}/riscv64_zero_aggregate_global_storage.c"
  TARGET_TRIPLE riscv64-linux-gnu
  OUT_TEXT "${BACKEND_TEST_BINARY_ROOT}/riscv64_zero_aggregate_global_storage.s"
  REQUIRED_SNIPPETS ".bss|rv64_zero_pair:|.zero 8|rv64_zero_array:|.zero 12|.text|.globl main|main:|li a0, 0|ret"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 global aggregate array storage bss output_contract
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_riscv64_aggregate_global_field_access
  SRC "${BACKEND_CASE_ROOT}/riscv64_aggregate_global_field_access.c"
  TARGET_TRIPLE riscv64-linux-gnu
  OUT_TEXT "${BACKEND_TEST_BINARY_ROOT}/riscv64_aggregate_global_field_access.s"
  REQUIRED_SNIPPETS ".bss|rv64_step4_pair_global:|.zero 8|.text|.globl main|main:|li t1, 13|lla t0, rv64_step4_pair_global|sw t1, 0(t0)|li t1, 29|sw t1, 4(t0)|lw t0, 4(t0)|lw s1, 0(s1)"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 global aggregate load_global store_global output_contract
)

c4c_add_backend_dump_flag_test(
  backend_dump_riscv64_prepared_fused_compare_loop_predicate
  SRC "${BACKEND_CASE_ROOT}/riscv64_prepared_fused_compare_loop_predicate.c"
  TARGET_TRIPLE riscv64-linux-gnu
  DUMP_FLAG --dump-prepared-bir
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|%t0 = bir.load_local i32 %lv.remaining|%t1 = bir.ne i32 %t0, 0|bir.cond_br i32 %t1, block_2, block_3|block block_1 terminator=cond_branch true=block_2 false=block_3|branch_condition block_1 kind=fused_compare condition=%t1 compare=ne i32 %t0, 0 can_fuse_with_branch=yes true=block_2 false=block_3|access block=block_1 inst_index=0 base=frame_slot result=%t0"
  FORBIDDEN_SNIPPETS "define i32 @main()|bir.func @00008|src/00008.c"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 prepared_control_flow fused_compare loop local_load
)

c4c_add_backend_dump_flag_test(
  backend_dump_riscv64_scalar_compare_frame_slot_destination
  SRC "${BACKEND_CASE_ROOT}/riscv64_scalar_compare_frame_slot_destination.c"
  TARGET_TRIPLE riscv64-linux-gnu
  DUMP_FLAG --dump-prepared-bir
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|%t2 = bir.ne i32 %t0, %t1|bir.store_local %lv.comparison, i32 %t2|storage %t2 frame_slot bank=gpr spill_slot=slot#5+stack24 offset=24|home %t2 value_id=2 kind=stack_slot slot_id=5 offset=24|object #2 func=main name=%lv.comparison source_kind=local_slot type=i32 size=4 align=4 address_exposed=yes requires_home_slot=yes permanent_home_slot=yes|slot #0 object_id=2 func=main offset=0 size=4 align=4 fixed_location=yes|store_source function=main block=entry inst=5 source=%t2 status=available intent=store_local_publication source_producer=binary|access block=entry inst_index=5 base=frame_slot stored=%t2 frame_slot=#0 offset=0 size=4 align=4"
  FORBIDDEN_SNIPPETS "semantic_write_event function=main status=available|semantic_materialization_interval function=main status=available|frame_slot_source_fact function=main status=available|branch_stack_load_authority function=main status=available"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 scalar_compare frame_slot_destination prepared_destination dump
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_riscv64_prepared_fused_compare_loop_predicate
  SRC "${BACKEND_CASE_ROOT}/riscv64_prepared_fused_compare_loop_predicate.c"
  TARGET_TRIPLE riscv64-linux-gnu
  OUT_TEXT "${BACKEND_TEST_BINARY_ROOT}/riscv64_prepared_fused_compare_loop_predicate.s"
  REQUIRED_SNIPPETS "main:|.Lmain_block_1:|lw t0, 0(sp)|mv t3, t0|li t4, 0|bne t3, t4, .Lmain_block_2|j .Lmain_block_3|.Lmain_block_2:|j .Lmain_block_1|.Lmain_block_3:|ret"
  FORBIDDEN_SNIPPETS "define i32 @main()|bir.func @00008|src/00008.c"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 prepared_control_flow fused_compare loop local_load emission
)

c4c_add_backend_dump_flag_test(
  backend_dump_riscv64_stack_homed_fused_compare_missing_false_label
  SRC "${BACKEND_CASE_ROOT}/riscv64_stack_homed_fused_compare_missing_false_label.c"
  TARGET_TRIPLE riscv64-linux-gnu
  DUMP_FLAG --dump-prepared-bir
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|block for.cond.1 terminator=cond_branch true=block_1 false=block_2|branch_condition for.cond.1 kind=fused_compare condition=%|compare=slt i32|can_fuse_with_branch=yes true=block_1 false=block_2|storage %|frame_slot bank=gpr|home %|kind=stack_slot"
  FORBIDDEN_SNIPPETS "src/00143.c|Duff"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 prepared_control_flow fused_compare loop stack_publication dump
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_riscv64_stack_homed_fused_compare_missing_false_label
  SRC "${BACKEND_CASE_ROOT}/riscv64_stack_homed_fused_compare_missing_false_label.c"
  TARGET_TRIPLE riscv64-linux-gnu
  OUT_TEXT "${BACKEND_TEST_BINARY_ROOT}/riscv64_stack_homed_fused_compare_missing_false_label.s"
  REQUIRED_SNIPPETS "main:|.Lmain_for_cond_1:|blt t3, t4, .Lmain_block_1|j .Lmain_block_2|.Lmain_block_1:|j .Lmain_for_latch_1|.Lmain_block_2:|ret"
  FORBIDDEN_SNIPPETS "src/00143.c|Duff"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 prepared_control_flow fused_compare loop missing_label emission
)

c4c_add_backend_dump_flag_test(
  backend_dump_riscv64_empty_loop_exit_successor
  SRC "${BACKEND_CASE_ROOT}/riscv64_empty_loop_exit_successor.c"
  TARGET_TRIPLE riscv64-linux-gnu
  DUMP_FLAG --dump-prepared-bir
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|block for.cond.1 terminator=cond_branch true=block_1 false=block_2|bir.store_local %lv.from, ptr|bir.store_local %lv.to, ptr|bir.cond_br i32 %block_2.switch.case.0.match|block block_2 terminator=cond_branch|branch_condition block_2 kind=fused_compare|base=pointer_value|bir.ret i32"
  FORBIDDEN_SNIPPETS "src/00143.c|_IO_stdin_used"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 prepared_control_flow loop empty_exit successor_body pointer_local switch expected_repair dump
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_riscv64_empty_loop_exit_successor
  SRC "${BACKEND_CASE_ROOT}/riscv64_empty_loop_exit_successor.c"
  TARGET_TRIPLE riscv64-linux-gnu
  OUT_TEXT "${BACKEND_TEST_BINARY_ROOT}/riscv64_empty_loop_exit_successor.s"
  REQUIRED_SNIPPETS "main:|j .Lmain_block_|.Lmain_block_|.Lmain_block_2_switch_case_|lh |sh |remw|mv a0|ret"
  FORBIDDEN_SNIPPETS "src/00143.c|_IO_stdin_used"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 prepared_control_flow loop empty_exit successor_body pointer_local switch emission
)

c4c_add_backend_dump_flag_test(
  backend_dump_riscv64_prepared_fused_compare_call_result_predicate
  SRC "${BACKEND_CASE_ROOT}/riscv64_prepared_fused_compare_call_result_predicate.c"
  TARGET_TRIPLE riscv64-linux-gnu
  DUMP_FLAG --dump-prepared-bir
  REQUIRED_SNIPPETS "bir.func @rv64_predicate_value() -> i32 {|bir.func @main() -> i32 {|%t0 = bir.call i32 rv64_predicate_value()|%t1 = bir.ne i32 %t0, 0|bir.cond_br i32 %t1, block_2, block_3|block entry terminator=cond_branch true=block_2 false=block_3|branch_condition entry kind=fused_compare condition=%t1 compare=ne i32 %t0, 0 can_fuse_with_branch=yes true=block_2 false=block_3|call block_index=0 inst_index=0 wrapper_kind=same_module|result value_bank=gpr source_storage=register destination_storage=register destination_value_id=0"
  FORBIDDEN_SNIPPETS "define i32 @main()|bir.func @00030|src/00030.c"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 prepared_control_flow fused_compare call_result
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_riscv64_prepared_fused_compare_call_result_predicate
  SRC "${BACKEND_CASE_ROOT}/riscv64_prepared_fused_compare_call_result_predicate.c"
  TARGET_TRIPLE riscv64-linux-gnu
  OUT_TEXT "${BACKEND_TEST_BINARY_ROOT}/riscv64_prepared_fused_compare_call_result_predicate.s"
  REQUIRED_SNIPPETS "rv64_predicate_value:|ret|main:|call rv64_predicate_value|mv t0, a0|mv t3, t0|li t4, 0|bne t3, t4, .Lmain_block_2|j .Lmain_block_3|.Lmain_block_2:|li a0, 1|ret|.Lmain_block_3:|li a0, 0|ret"
  FORBIDDEN_SNIPPETS "define i32 @main()|bir.func @00030|src/00030.c"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 prepared_control_flow fused_compare call_result emission
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_riscv64_prepared_loop_successor_body_boolean_tail
  SRC "${BACKEND_CASE_ROOT}/riscv64_prepared_loop_successor_body_boolean_tail.c"
  TARGET_TRIPLE riscv64-linux-gnu
  OUT_TEXT "${BACKEND_TEST_BINARY_ROOT}/riscv64_prepared_loop_successor_body_boolean_tail.s"
  REQUIRED_SNIPPETS ".Lmain_for_cond_1:|blt t3, t4, .Lmain_block_1|j .Lmain_block_2|.Lmain_for_latch_1:|sw t1, 0(sp)|j .Lmain_for_cond_1|.Lmain_block_1:|xori|bne t3, t4, .Lmain_block_3|j .Lmain_block_4|.Lmain_block_2:|li a0, 0|ret|.Lmain_block_3:|j .Lmain_for_latch_1|.Lmain_block_4:|j .Lmain_for_latch_1"
  FORBIDDEN_SNIPPETS "define i32 @main()|bir.func @00105|src/00105.c"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 prepared_control_flow fused_compare loop boolean_tail emission
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_riscv64_prepared_signed_div_rem_return_tail
  SRC "${BACKEND_CASE_ROOT}/riscv64_prepared_signed_div_rem_return_tail.c"
  TARGET_TRIPLE riscv64-linux-gnu
  OUT_TEXT "${BACKEND_TEST_BINARY_ROOT}/riscv64_prepared_signed_div_rem_return_tail.s"
  REQUIRED_SNIPPETS "main:|li t1, 42|sw t1, 0(sp)|lw t0, 0(sp)|li t4, 5|divw s1, t0, t4|sw t1, 0(sp)|lw t0, 0(sp)|li t4, 3|remw s1, t0, t4|sw t1, 0(sp)|lw t0, 0(sp)|addi s1, t0, -2|mv a0, s1|ret"
  FORBIDDEN_SNIPPETS "define i32 @main()|bir.func @00009|src/00009.c"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 prepared_control_flow signed_div_rem return_tail emission
)

c4c_add_backend_dump_flag_test(
  backend_dump_riscv64_prepared_local_array_base_pointer
  SRC "${BACKEND_CASE_ROOT}/riscv64_prepared_local_array_base_pointer.c"
  TARGET_TRIPLE riscv64-linux-gnu
  DUMP_FLAG --dump-prepared-bir
  REQUIRED_SNIPPETS "bir.store_local %lv.arr.0, i32 11|bir.store_local %lv.arr.1, i32 31|bir.add ptr %lv.arr.0, 0|bir.store_local %lv.p, ptr %t|bir.load_local ptr %lv.p|access block=entry inst_index=3 base=frame_slot stored=%t|address_materialization block=entry inst_index=2 kind=frame_slot result=%t|frame_slot_order slot_id="
  FORBIDDEN_SNIPPETS "define i32 @main()|bir.func @00032|src/00032.c|src/00072.c"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 prepared_local_address array_base pointer_local dump
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_riscv64_prepared_local_array_base_pointer
  SRC "${BACKEND_CASE_ROOT}/riscv64_prepared_local_array_base_pointer.c"
  TARGET_TRIPLE riscv64-linux-gnu
  OUT_TEXT "${BACKEND_TEST_BINARY_ROOT}/riscv64_prepared_local_array_base_pointer.s"
  REQUIRED_SNIPPETS "main:|addi s1, sp,|sd s1,|ld|lw|add|addi s1,|mv a0,|ret"
  FORBIDDEN_SNIPPETS "define i32 @main()|bir.func @00032|src/00032.c|src/00072.c"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 prepared_local_address array_base pointer_local emission
)

c4c_add_backend_dump_flag_test(
  backend_dump_riscv64_prepared_local_array_subobject_pointer
  SRC "${BACKEND_CASE_ROOT}/riscv64_prepared_local_array_subobject_pointer.c"
  TARGET_TRIPLE riscv64-linux-gnu
  DUMP_FLAG --dump-prepared-bir
  REQUIRED_SNIPPETS "bir.store_local %lv.arr.28, i32 37|bir.add ptr %lv.arr.16, 12|bir.store_local %lv.p, ptr %t|bir.load_local ptr %lv.p|bir.load_local i32 %t9.addr, addr %t|object #4 func=main name=%lv.arr.16 source_kind=local_slot type=i32 size=4 align=4 address_exposed=yes|access block=entry inst_index=2 base=frame_slot stored=%t|address_materialization block=entry inst_index=1 kind=frame_slot result=%t"
  FORBIDDEN_SNIPPETS "define i32 @main()|bir.func @00130|src/00130.c"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 prepared_local_address array_subobject pointer_local dump
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_riscv64_prepared_local_array_subobject_pointer
  SRC "${BACKEND_CASE_ROOT}/riscv64_prepared_local_array_subobject_pointer.c"
  TARGET_TRIPLE riscv64-linux-gnu
  OUT_TEXT "${BACKEND_TEST_BINARY_ROOT}/riscv64_prepared_local_array_subobject_pointer.s"
  REQUIRED_SNIPPETS "main:|addi s1, sp,|sd s1,|ld|lw|addi s1, s2, -37|mv a0, s1|ret"
  FORBIDDEN_SNIPPETS "define i32 @main()|bir.func @00130|src/00130.c"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 prepared_local_address array_subobject pointer_local emission
)

c4c_add_backend_dump_flag_test(
  backend_dump_riscv64_prepared_local_array_pointer_step
  SRC "${BACKEND_CASE_ROOT}/riscv64_prepared_local_array_pointer_step.c"
  TARGET_TRIPLE riscv64-linux-gnu
  DUMP_FLAG --dump-prepared-bir
  REQUIRED_SNIPPETS "bir.add ptr %lv.arr.0, 0|bir.store_local %lv.p, ptr|bir.load_local ptr %lv.p|bir.add ptr %lv.arr.0, 4|bir.sub ptr %lv.arr.0, 4|base=pointer_value|address_materialization block=entry|offset=-4"
  FORBIDDEN_SNIPPETS "define i32 @main()|bir.func @00032|src/00032.c"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 prepared_local_address array_base pointer_local pointer_step dump
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_riscv64_prepared_local_array_pointer_step
  SRC "${BACKEND_CASE_ROOT}/riscv64_prepared_local_array_pointer_step.c"
  TARGET_TRIPLE riscv64-linux-gnu
  OUT_TEXT "${BACKEND_TEST_BINARY_ROOT}/riscv64_prepared_local_array_pointer_step.s"
  REQUIRED_SNIPPETS "main:|addi s1, sp, 0|sd s1,|ld s1,|addi s2, sp, 4|sd s2,|lw s2, 0(s1)|.Lmain_block_4:|addi s1, sp, 4|sd s1,|ld s1,|addi s1, sp, 0|sd s1,|lw s1, 0(sp)|.Lmain_block_6:|addi s1, sp, 0|sd s1,|ld s1,|addi s2, sp, -4|sd s2,|lw s2, 0(s1)|li a0, 0|ret"
  FORBIDDEN_SNIPPETS "define i32 @main()|bir.func @00032|src/00032.c"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 prepared_local_address array_base pointer_local pointer_step emission
)

c4c_add_backend_dump_flag_test(
  backend_dump_riscv64_loop_carried_pointer_postincrement
  SRC "${BACKEND_CASE_ROOT}/riscv64_loop_carried_pointer_postincrement.c"
  TARGET_TRIPLE riscv64-linux-gnu
  DUMP_FLAG --dump-prepared-bir
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|bir.load_local ptr %lv.from|%t29 = bir.add ptr %t28, 2|bir.store_local %lv.from, ptr %t29|bir.load_local ptr %lv.to|%t32 = bir.add ptr %t31, 2|bir.store_local %lv.to, ptr %t32|base=pointer_value result=|offset=0 size=2|base=pointer_value stored=|offset=0 size=2"
  FORBIDDEN_SNIPPETS "src/00143.c|Duff|missing_destination_access|%t29 = bir.add ptr %lv.source.0, 2|%t32 = bir.add ptr %lv.sink.0, 2"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 prepared_local_address array_base pointer_local pointer_step loop dump
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_riscv64_loop_carried_pointer_postincrement
  SRC "${BACKEND_CASE_ROOT}/riscv64_loop_carried_pointer_postincrement.c"
  TARGET_TRIPLE riscv64-linux-gnu
  OUT_TEXT "${BACKEND_TEST_BINARY_ROOT}/riscv64_loop_carried_pointer_postincrement.s"
  REQUIRED_SNIPPETS "main:|ld t0,|addi t3, t0, 2|sd s1,|lh t3, 0(t0)|sh t1, 0(t0)|j .Lmain_|.Lmain_|ret"
  FORBIDDEN_SNIPPETS "src/00143.c|Duff|addi t3, sp, 0\n    addi t3, t3, 2|addi t3, sp, 8\n    addi t3, t3, 2"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 prepared_local_address array_base pointer_local pointer_step loop emission
)

c4c_add_backend_dump_flag_test(
  backend_dump_riscv64_duff_fallthrough_pointer_update_producers
  SRC "${BACKEND_CASE_ROOT}/riscv64_duff_fallthrough_pointer_update_producers.c"
  TARGET_TRIPLE riscv64-linux-gnu
  DUMP_FLAG --dump-prepared-bir
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|%t45 = bir.add ptr %t44, 2|bir.store_local %lv.src, ptr %t45|%t48 = bir.add ptr %t47, 2|bir.store_local %lv.dst, ptr %t48|block_7:|%t55 = bir.add ptr %t54, 2|bir.store_local %lv.src, ptr %t55|%t58 = bir.add ptr %t57, 2|bir.store_local %lv.dst, ptr %t58|block_8:|%t60 = bir.add ptr %t59, 2|bir.store_local %lv.src, ptr %t60|%t63 = bir.add ptr %t62, 2|bir.store_local %lv.dst, ptr %t63|store_source function=main block=block_4 inst=2 source=%t45 status=available intent=store_local_publication source_producer=binary|store_source function=main block=block_7 inst=2 source=%t55 status=available intent=store_local_publication source_producer=binary|store_source function=main block=block_7 inst=6 source=%t58 status=available intent=store_local_publication source_producer=binary|store_source function=main block=block_8 inst=2 source=%t60 status=available intent=store_local_publication source_producer=binary|store_source function=main block=block_8 inst=6 source=%t63 status=available intent=store_local_publication source_producer=binary|base=pointer_value result=|base=pointer_value stored="
  FORBIDDEN_SNIPPETS "src/00143.c|Tom Duff|0x0000000500000002|store_source function=main block=block_7 inst=1 source=%t55 status=available intent=store_local_publication source_producer=unknown|store_source function=main block=block_7 inst=4 source=%t58 status=available intent=store_local_publication source_producer=unknown"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 prepared_local_address array_base pointer_local pointer_step fallthrough dump
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_riscv64_duff_fallthrough_pointer_update_producers
  SRC "${BACKEND_CASE_ROOT}/riscv64_duff_fallthrough_pointer_update_producers.c"
  TARGET_TRIPLE riscv64-linux-gnu
  OUT_TEXT "${BACKEND_TEST_BINARY_ROOT}/riscv64_duff_fallthrough_pointer_update_producers.s"
  REQUIRED_SNIPPETS "main:|.Lmain_block_7:|ld t0,|addi t3, t0, 2|sd s1,|lh t3, 0(t0)|sh t1, 0(t0)|.Lmain_block_8:|ld t0,|addi t3, t0, 2|sd s1,|lh t3, 0(t0)|sh t1, 0(t0)|li a0, 0|ret"
  FORBIDDEN_SNIPPETS "src/00143.c|Tom Duff|0x0000000500000002|addi t3, sp, 0\n    addi t3, t3, 2|addi t3, sp, 10\n    addi t3, t3, 2"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 prepared_local_address array_base pointer_local pointer_step fallthrough emission
)

c4c_add_backend_dump_flag_test(
  backend_dump_riscv64_prepared_local_array_i8_element_access
  SRC "${BACKEND_CASE_ROOT}/riscv64_prepared_local_array_i8_element_access.c"
  TARGET_TRIPLE riscv64-linux-gnu
  DUMP_FLAG --dump-prepared-bir
  REQUIRED_SNIPPETS "bir.store_local %lv.arr.7, i8 2|bir.load_local i8 %lv.arr.7|bir.load_local i8 %t32.addr, addr %t31|bir.sext i8|base=pointer_value result=%t32|address_materialization block=entry inst_index=1 kind=frame_slot result=%t4"
  FORBIDDEN_SNIPPETS "define i32 @main()|bir.func @00130|src/00130.c"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 prepared_local_address array_subobject i8_load pointer_local dump
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_riscv64_prepared_local_array_i8_element_access
  SRC "${BACKEND_CASE_ROOT}/riscv64_prepared_local_array_i8_element_access.c"
  TARGET_TRIPLE riscv64-linux-gnu
  OUT_TEXT "${BACKEND_TEST_BINARY_ROOT}/riscv64_prepared_local_array_i8_element_access.s"
  REQUIRED_SNIPPETS "main:|addi t1, sp, 0|sd t1, 8(sp)|addi s2, sp, 7|sd s2, 16(sp)|sb t1, 7(sp)|lb s2, 7(sp)|bne t3, t4, .Lmain_block_1|ld t0, 16(sp)|lb s1, 0(t0)|li a0, 0|ret"
  FORBIDDEN_SNIPPETS "define i32 @main()|bir.func @00130|src/00130.c"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 prepared_local_address array_subobject i8_load pointer_local emission
)

c4c_add_backend_dump_flag_test(
  backend_dump_riscv64_i16_local_array_select_store
  SRC "${BACKEND_CASE_ROOT}/riscv64_i16_local_array_select_store.c"
  TARGET_TRIPLE riscv64-linux-gnu
  DUMP_FLAG --dump-prepared-bir
  REQUIRED_SNIPPETS "bir.store_local %lv.values.2, i16 3|bir.load_local i16 %lv.replacement|bir.select eq i64 %t15, 2, i16 %t13, %t16.elt2|bir.store_local %lv.values.2, i16 %t16.store2|store_source function=main block=entry inst=19 source=%t16.store2 status=available|source_producer=select_materialization|select_chain function=main block=entry value=%t16.store2 root_is_select=yes|access block=entry inst_index=19 base=frame_slot stored=%t16.store2|size=2"
  FORBIDDEN_SNIPPETS "src/00143.c|%t9.store0|missing_destination_access|i32 %t16.store2"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 prepared_local_address array_subobject i16_load select store_source dump
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_riscv64_i16_local_array_select_store
  SRC "${BACKEND_CASE_ROOT}/riscv64_i16_local_array_select_store.c"
  TARGET_TRIPLE riscv64-linux-gnu
  OUT_TEXT "${BACKEND_TEST_BINARY_ROOT}/riscv64_i16_local_array_select_store.s"
  REQUIRED_SNIPPETS "main:|sh t1,|lh t3,|.Lmain_select_|sh t3,|mv a0, t0|ret"
  FORBIDDEN_SNIPPETS "src/00143.c|%t9.store0|lhu "
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 prepared_local_address array_subobject i16_load select store_source emission
)

c4c_add_backend_dump_flag_test(
  backend_dump_riscv64_pointer_to_pointer_local_address
  SRC "${BACKEND_CASE_ROOT}/riscv64_pointer_to_pointer_local_address.c"
  TARGET_TRIPLE riscv64-linux-gnu
  DUMP_FLAG --dump-prepared-bir
  REQUIRED_SNIPPETS "bir.store_local %lv.p, ptr %lv.value|bir.store_local %lv.pp, ptr %lv.p|bir.load_local ptr %lv.pp|bir.store_local %t1.store.addr, i32 7, addr %t1|object #1 func=main name=%lv.p source_kind=local_slot type=ptr size=8 align=8 address_exposed=yes|address_materialization block=entry inst_index=2 kind=frame_slot result=%lv.p"
  FORBIDDEN_SNIPPETS "src/00005.c"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 prepared_local_address pointer_local pointer_to_pointer dump
)

c4c_add_backend_dump_flag_test(
  backend_dump_riscv64_pointer_integer_select_chain
  SRC "${BACKEND_CASE_ROOT}/riscv64_pointer_integer_select_chain.c"
  TARGET_TRIPLE riscv64-linux-gnu
  DUMP_FLAG --dump-prepared-bir
  REQUIRED_SNIPPETS "bir.select ne i32 %t0, 0, i32 0, 0|bir.inttoptr i32 %t7 to ptr|bir.store_local %lv.p, ptr %t8|bir.inttoptr i32 %t32 to ptr|bir.store_local %lv.p, ptr %t33|bir.store_local %lv.q, ptr %t34|bir.ne ptr %t35, 0|select_chain function=main block=tern.end.15 value=%t32 root_is_select=yes"
  FORBIDDEN_SNIPPETS "src/00144.c"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 pointer select pointer_local dump
)

c4c_add_backend_dump_flag_test(
  backend_dump_riscv64_pointer_typed_select_publication
  SRC "${BACKEND_CASE_ROOT}/riscv64_pointer_typed_select_publication.c"
  TARGET_TRIPLE riscv64-linux-gnu
  DUMP_FLAG --dump-prepared-bir
  REQUIRED_SNIPPETS "bir.select ne i32 %t0, 0, ptr %lv.value, 0|bir.store_local %lv.selected, ptr %t7|bir.store_local %lv.published, ptr %t8|select_chain function=main block=tern.end.6 value=%t7 root_is_select=yes|store_source function=main block=tern.end.6 inst=1 source=%t7 status=available"
  FORBIDDEN_SNIPPETS "src/00144.c|riscv64_pointer_integer_select_chain"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 pointer select pointer_local dump
)

c4c_add_backend_dump_flag_test(
  backend_dump_riscv64_nested_select_store_source_publication
  SRC "${BACKEND_CASE_ROOT}/riscv64_nested_select_store_source_publication.c"
  TARGET_TRIPLE riscv64-linux-gnu
  DUMP_FLAG --dump-prepared-bir
  REQUIRED_SNIPPETS "bir.store_local %lv.chosen, ptr %t8|bir.store_local %lv.published, ptr %t36|store_source function=main block=|source=%t8 status=available|store_source function=main block=|source=%t36 status=available|source_producer=select_materialization|source_select=yes|store_source function=main block=|source=%t56 status=available|source_cast=yes|select_chain function=main block=|value=%t36 root_is_select=yes"
  FORBIDDEN_SNIPPETS "src/00144.c|riscv64_pointer_typed_select_publication|i16|status=missing_destination_access"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 pointer select nested_select store_source dump
)

c4c_add_backend_dump_flag_test(
  backend_dump_riscv64_short_circuit_select_false_lhs
  SRC "${BACKEND_CASE_ROOT}/riscv64_short_circuit_select_false_lhs.c"
  TARGET_TRIPLE riscv64-linux-gnu
  DUMP_FLAG --dump-prepared-bir
  REQUIRED_SNIPPETS "bir.select ne i32 %t5, 5, i32 %t16, 0|bir.load_local i32 %t15.addr, addr %lv.box.4|bir.load_local i32 %lv.box.8|select_chain function=main block=logic.end.12 value=%t20 root_is_select=yes|block_entry_publication successor=logic.end.12 status=available"
  FORBIDDEN_SNIPPETS "src/00046.c"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 select short_circuit aggregate dump
)

c4c_add_backend_dump_flag_test(
  backend_dump_riscv64_compare_result_select_false_arm
  SRC "${BACKEND_CASE_ROOT}/riscv64_compare_result_select_false_arm.c"
  TARGET_TRIPLE riscv64-linux-gnu
  DUMP_FLAG --dump-prepared-bir
  REQUIRED_SNIPPETS "bir.select ne i32 %t0, 5, i32 %t10, 0|bir.store_local %lv.selected, i32 %t12|bir.load_local i32 %lv.selected|select_chain function=main block=tern.end.8 value=%t12 root_is_select=yes"
  FORBIDDEN_SNIPPETS "src/00046.c|riscv64_short_circuit_select_false_lhs"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 select short_circuit dump
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_riscv64_pointer_to_pointer_local_address
  SRC "${BACKEND_CASE_ROOT}/riscv64_pointer_to_pointer_local_address.c"
  TARGET_TRIPLE riscv64-linux-gnu
  OUT_TEXT "${BACKEND_TEST_BINARY_ROOT}/riscv64_pointer_to_pointer_local_address.s"
  REQUIRED_SNIPPETS ".globl main|sd t1,|ld s2,|ld t3, 0(s2)|sd t3,|ld t3,|sw t1, 0(t3)|ret"
  FORBIDDEN_SNIPPETS "src/00005.c"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 prepared_local_address pointer_local pointer_to_pointer emission
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_riscv64_pointer_integer_select_chain
  SRC "${BACKEND_CASE_ROOT}/riscv64_pointer_integer_select_chain.c"
  TARGET_TRIPLE riscv64-linux-gnu
  OUT_TEXT "${BACKEND_TEST_BINARY_ROOT}/riscv64_pointer_integer_select_chain.s"
  REQUIRED_SNIPPETS ".globl main|mv s1, t0|sd s1,|ld t0,|bne t3, t4|mv a0, t0|ret"
  FORBIDDEN_SNIPPETS "src/00144.c"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 pointer select pointer_local emission
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_riscv64_pointer_typed_select_publication
  SRC "${BACKEND_CASE_ROOT}/riscv64_pointer_typed_select_publication.c"
  TARGET_TRIPLE riscv64-linux-gnu
  OUT_TEXT "${BACKEND_TEST_BINARY_ROOT}/riscv64_pointer_typed_select_publication.s"
  REQUIRED_SNIPPETS ".Lmain_tern_end_6:|bne t3, t4, .Lmain_select_|li t0, 0|addi t0, sp,|sd t0,|ld t0,|mv a0, t0|ret"
  FORBIDDEN_SNIPPETS "mv t0, t0|src/00144.c|riscv64_pointer_integer_select_chain"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 pointer select pointer_local emission
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_riscv64_short_circuit_select_false_lhs
  SRC "${BACKEND_CASE_ROOT}/riscv64_short_circuit_select_false_lhs.c"
  TARGET_TRIPLE riscv64-linux-gnu
  OUT_TEXT "${BACKEND_TEST_BINARY_ROOT}/riscv64_short_circuit_select_false_lhs.s"
  REQUIRED_SNIPPETS ".Lmain_logic_end_12:|li t0, 0|li t4, 7|li t4, 5|bne t3, t4, .Lmain_select_|bne t3, t4, .Lmain_block_1|li a0, 0|ret"
  FORBIDDEN_SNIPPETS "src/00046.c"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 select short_circuit aggregate emission
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_riscv64_compare_result_select_false_arm
  SRC "${BACKEND_CASE_ROOT}/riscv64_compare_result_select_false_arm.c"
  TARGET_TRIPLE riscv64-linux-gnu
  OUT_TEXT "${BACKEND_TEST_BINARY_ROOT}/riscv64_compare_result_select_false_arm.s"
  REQUIRED_SNIPPETS ".Lmain_tern_end_8:|li t0, 0|li t4, 7|li t4, 5|bne t3, t4, .Lmain_select_|lw t0, 8(sp)|bne t3, t4, .Lmain_block_1|li a0, 0|ret"
  FORBIDDEN_SNIPPETS "src/00046.c|riscv64_short_circuit_select_false_lhs"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 select short_circuit emission
)

c4c_add_backend_dump_flag_test(
  backend_dump_riscv64_byval_aggregate_fixed_call
  SRC "${BACKEND_CASE_ROOT}/riscv64_byval_aggregate_fixed_call.c"
  TARGET_TRIPLE riscv64-linux-gnu
  DUMP_FLAG --dump-prepared-bir
  REQUIRED_SNIPPETS "bir.func @consume_parcel(ptr byval(size=16, align=8) %p.value, ptr %p.mirror) -> i32|%lv.param.p.value.aggregate.param.copy.0 = bir.load_local i32 %lv.param.p.value.0, addr %p.value|%lv.param.p.value.aggregate.param.copy.8 = bir.load_local ptr %lv.param.p.value.8, addr %p.value+8|%lv.param.value.byval.copy.0 = bir.load_local i32 %lv.param.value.0, addr %p.value|%t9 = bir.call i32 consume_parcel(ptr byval(size=16, align=8) %lv.value, ptr %lv.mirror.0)|arg0 bank=aggregate_address from=register:s1 to=none|move from_value_id=20 to_value_id=20 destination_kind=call_argument_abi destination_storage=stack_slot|source_kind=byval_param type=ptr size=16 align=8|base=pointer_value result=%lv.param.p.value.aggregate.param.copy.0 pointer=%p.value|base=pointer_value result=%lv.param.value.byval.copy.8 pointer=%p.value"
  FORBIDDEN_SNIPPETS "src/00140.c|%p.f"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 byval aggregate call dump
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_riscv64_byval_aggregate_fixed_call
  SRC "${BACKEND_CASE_ROOT}/riscv64_byval_aggregate_fixed_call.c"
  TARGET_TRIPLE riscv64-linux-gnu
  OUT_TEXT "${BACKEND_TEST_BINARY_ROOT}/riscv64_byval_aggregate_fixed_call.s"
  REQUIRED_SNIPPETS ".globl consume_parcel|consume_parcel:|.Lconsume_parcel_entry:|sd a0, 0(sp)|ld t3, 0(sp)|.globl main|main:|sd ra, 48(sp)|addi sp, sp, -16|ld t3, 24(sp)|sd t3, 8(sp)|lw t3, 32(sp)|sw t3, 0(sp)|lw t3, 40(sp)|sw t3, 4(sp)|mv a0, sp|addi a1, sp, 16|call consume_parcel|addi sp, sp, 16|ret"
  FORBIDDEN_SNIPPETS "src/00140.c|%p.f"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 byval aggregate call emission
)

c4c_add_backend_dump_flag_test(
  backend_dump_riscv64_byval_aggregate_float_lane
  SRC "${BACKEND_CASE_ROOT}/riscv64_byval_aggregate_float_lane.c"
  TARGET_TRIPLE riscv64-linux-gnu
  DUMP_FLAG --dump-prepared-bir
  REQUIRED_SNIPPETS "bir.func @consume_float_parcel(ptr byval(size=16, align=4) %p.value) -> i32|%lv.param.p.value.aggregate.param.copy.4 = bir.load_local float %lv.param.p.value.4, addr %p.value+4|bir.store_local %lv.param.p.value.4, float %lv.param.p.value.aggregate.param.copy.4|%lv.param.value.byval.copy.4 = bir.load_local float %lv.param.value.4, addr %p.value+4|bir.store_local %lv.param.value.4, float %lv.param.value.byval.copy.4|storage %lv.param.p.value.aggregate.param.copy.4 register bank=fpr|storage %lv.param.value.byval.copy.4 register bank=fpr|base=pointer_value result=%lv.param.p.value.aggregate.param.copy.4 pointer=%p.value|base=frame_slot stored=%lv.param.p.value.aggregate.param.copy.4|base=pointer_value result=%lv.param.value.byval.copy.4 pointer=%p.value|base=frame_slot stored=%lv.param.value.byval.copy.4"
  FORBIDDEN_SNIPPETS "src/00140.c|%p.f"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 byval aggregate scalar_fp call dump
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_riscv64_byval_aggregate_float_lane
  SRC "${BACKEND_CASE_ROOT}/riscv64_byval_aggregate_float_lane.c"
  TARGET_TRIPLE riscv64-linux-gnu
  OUT_TEXT "${BACKEND_TEST_BINARY_ROOT}/riscv64_byval_aggregate_float_lane.s"
  REQUIRED_SNIPPETS ".globl consume_float_parcel|consume_float_parcel:|sd a0, 0(sp)|ld t3, 0(sp)|flw ft0, 4(t3)|fsw ft0,|li t4, 5|li t4, 7|li t4, 9|ret"
  FORBIDDEN_SNIPPETS "src/00140.c|%p.f"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 byval aggregate scalar_fp call emission
)

c4c_add_backend_dump_flag_test(
  backend_dump_riscv64_byval_preserved_pointer_args
  SRC "${BACKEND_CASE_ROOT}/riscv64_byval_preserved_pointer_args.c"
  TARGET_TRIPLE riscv64-linux-gnu
  DUMP_FLAG --dump-prepared-bir
  REQUIRED_SNIPPETS "bir.call i32 consume_pairbox(ptr byval(size=16, align=4) %lv.value, ptr %lv.value)|arg index=0 value_bank=aggregate_address source_encoding=register source_value_id=20 source_placement=gpr:callee_saved#0/w1 source_reg=s1 source_bank=gpr dest_bank=none arg.source_selection=local_frame_address_materialization selection_source_value_id=20 selection_source_value=%lv.value selection_source_home=register selection_source_slot=#11 selection_source_stack_offset=0 selection_source_size=16 selection_source_align=4 selection_source_delta=0 selection_materialization_block=entry selection_materialization_inst=6 selection_materialization_slot=#11 selection_materialization_offset=0 selection_preserved_bank=none arg.aggregate_transport=stack_copy payload_size=16 payload_align=4 copy_size=16 copy_align=4 transport_source_slot=#11 transport_source_stack_offset=0 chunk index=0 kind=required_payload payload_offset=0 source_offset=0 dest_offset=0 size=4 align=4 preferred_width=4 chunk index=1 kind=required_payload payload_offset=4 source_offset=4 dest_offset=4 size=4 align=4 preferred_width=4 chunk index=2 kind=required_payload payload_offset=8 source_offset=8 dest_offset=8 size=4 align=4 preferred_width=4 chunk index=3 kind=required_payload payload_offset=12 source_offset=12 dest_offset=12 size=4 align=4 preferred_width=4 scratch=general_purpose scratch_width=8 scratch_overlap_source=no scratch_overlap_dest=no|arg index=1 value_bank=gpr source_encoding=register source_value_id=20 source_placement=gpr:callee_saved#0/w1 source_reg=s1 source_bank=gpr dest_placement=gpr:call_argument#1/w1 dest_reg=a1 dest_bank=gpr arg.source_selection=local_frame_address_materialization selection_source_value_id=20 selection_source_value=%lv.value selection_source_home=register selection_source_slot=#11 selection_source_stack_offset=0 selection_source_size=8 selection_source_align=8 selection_source_delta=0 selection_materialization_block=entry selection_materialization_inst=6 selection_materialization_slot=#11 selection_materialization_offset=0 selection_preserved_bank=none|address_materialization block=entry inst_index=6 kind=frame_slot result=%lv.value"
  FORBIDDEN_SNIPPETS "src/00140.c|%p.f"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 byval aggregate call pointer prepared_call_boundary dump
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_riscv64_byval_preserved_pointer_args
  SRC "${BACKEND_CASE_ROOT}/riscv64_byval_preserved_pointer_args.c"
  TARGET_TRIPLE riscv64-linux-gnu
  OUT_TEXT "${BACKEND_TEST_BINARY_ROOT}/riscv64_byval_preserved_pointer_args.s"
  REQUIRED_SNIPPETS "main:|addi sp, sp, -16|mv a0, sp|addi a1, sp, 16|call consume_pairbox|addi sp, sp, 16|addi sp, sp, -16|mv a0, sp|addi a1, sp, 16|call consume_pairbox"
  FORBIDDEN_SNIPPETS "mv a1, s1|src/00140.c|%p.f"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 byval aggregate call pointer prepared_call_boundary emission
)

c4c_add_backend_dump_flag_test(
  backend_dump_riscv64_byval_formal_gpr_publication
  SRC "${BACKEND_CASE_ROOT}/riscv64_byval_formal_gpr_publication.c"
  TARGET_TRIPLE riscv64-linux-gnu
  DUMP_FLAG --dump-prepared-bir
  REQUIRED_SNIPPETS "bir.func @consume_formal_box(ptr byval(size=16, align=4) %p.value, ptr %p.mirror, i32 %p.delta) -> i32|storage %p.value frame_slot bank=gpr|storage %p.mirror register bank=gpr placement=gpr:caller_saved#0/w1 reg=t0|storage %p.delta register bank=gpr placement=gpr:callee_saved#0/w1 reg=s1|arg index=1 value_bank=gpr|dest_reg=a1|arg index=2 value_bank=gpr|dest_reg=a2|base=pointer_value result=%t4 pointer=%p.mirror|base=pointer_value result=%t18 pointer=%p.mirror"
  FORBIDDEN_SNIPPETS "src/00140.c|%p.f|%lv.f"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 byval aggregate call formal_publication dump
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_riscv64_byval_formal_gpr_publication
  SRC "${BACKEND_CASE_ROOT}/riscv64_byval_formal_gpr_publication.c"
  TARGET_TRIPLE riscv64-linux-gnu
  OUT_TEXT "${BACKEND_TEST_BINARY_ROOT}/riscv64_byval_formal_gpr_publication.s"
  REQUIRED_SNIPPETS ".globl consume_formal_box|consume_formal_box:|mv t0, a1|mv s1, a2|lw t3, 0(t0)|mv t4, s1|.globl main|main:|li a2, 5|call consume_formal_box"
  FORBIDDEN_SNIPPETS "src/00140.c|%p.f|%lv.f"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 byval aggregate call formal_publication emission
)

c4c_add_backend_dump_flag_test(
  backend_dump_riscv64_aggregate_local_self_pointer_chain
  SRC "${BACKEND_CASE_ROOT}/riscv64_aggregate_local_self_pointer_chain.c"
  TARGET_TRIPLE riscv64-linux-gnu
  DUMP_FLAG --dump-prepared-bir
  REQUIRED_SNIPPETS "bir.store_local %lv.node.8, i32 9|bir.store_local %lv.node.0, ptr %lv.node|bir.load_local ptr %t5.addr, addr %lv.node|bir.load_local ptr %t7.addr, addr %t5|bir.load_local i32 %t9.addr, addr %t7+8|base=pointer_value result=%t5 pointer=%lv.node|base=pointer_value result=%t7 pointer=%t5|base=pointer_value result=%t9 pointer=%t7"
  FORBIDDEN_SNIPPETS "define i32 @main()|src/00019.c"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 aggregate local pointer_local aggregate_subobject dump
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_riscv64_aggregate_local_self_pointer_chain
  SRC "${BACKEND_CASE_ROOT}/riscv64_aggregate_local_self_pointer_chain.c"
  TARGET_TRIPLE riscv64-linux-gnu
  OUT_TEXT "${BACKEND_TEST_BINARY_ROOT}/riscv64_aggregate_local_self_pointer_chain.s"
  REQUIRED_SNIPPETS "main:|li t1, 9|sw t1, 8(sp)|sd s1, 0(sp)|ld s2, 0(s1)|ld s1, 0(s2)|lw s2, 8(s1)|mv a0, s1|ret"
  FORBIDDEN_SNIPPETS "src/00019.c"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 aggregate local pointer_local aggregate_subobject emission
)

c4c_add_backend_dump_flag_test(
  backend_dump_riscv64_aggregate_local_anonymous_union_fields
  SRC "${BACKEND_CASE_ROOT}/riscv64_aggregate_local_anonymous_union_fields.c"
  TARGET_TRIPLE riscv64-linux-gnu
  DUMP_FLAG --dump-prepared-bir
  REQUIRED_SNIPPETS "bir.store_local %lv.value.0, i32 1|bir.store_local %lv.value.4, i32 2, addr %lv.value.4|bir.store_local %lv.value.8, i32 3, addr %lv.value.8|bir.store_local %lv.value.12, i32 4|bir.load_local i32 %t13.addr, addr %lv.value.4|bir.load_local i32 %t18.addr, addr %lv.value.4|bir.load_local i32 %t25.addr, addr %lv.value.8|range_verdict=proven_in_bounds"
  FORBIDDEN_SNIPPETS "define i32 @main()|src/00046.c"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 aggregate local aggregate_subobject anonymous_union dump
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_riscv64_aggregate_local_anonymous_union_fields
  SRC "${BACKEND_CASE_ROOT}/riscv64_aggregate_local_anonymous_union_fields.c"
  TARGET_TRIPLE riscv64-linux-gnu
  OUT_TEXT "${BACKEND_TEST_BINARY_ROOT}/riscv64_aggregate_local_anonymous_union_fields.s"
  REQUIRED_SNIPPETS "main:|li t1, 1|sw t1, 0(sp)|li t1, 2|sw t1, 4(sp)|li t1, 3|sw t1, 8(sp)|li t1, 4|sw t1, 12(sp)|lw s1, 4(sp)|lw t0, 4(sp)|lw s1, 8(sp)|lw t0, 12(sp)|mv a0, t0|ret"
  FORBIDDEN_SNIPPETS "src/00046.c"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 aggregate local aggregate_subobject anonymous_union emission
)

c4c_add_backend_dump_flag_test(
  backend_dump_riscv64_function_pointer_local_storage
  SRC "${BACKEND_CASE_ROOT}/riscv64_function_pointer_local_storage.c"
  TARGET_TRIPLE riscv64-linux-gnu
  DUMP_FLAG --dump-prepared-bir
  REQUIRED_SNIPPETS "bir.store_local %lv.slot.0, ptr @add_three|bir.store_local %lv.local_fn, ptr @add_three|bir.load_local ptr %lv.local_fn|bir.call i32 %t3(i32 4)|wrapper_kind=indirect|indirect_callee=%t3|address_materialization block=entry inst_index=0 kind=direct_global result=@add_three symbol=add_three|address_materialization block=entry inst_index=1 kind=direct_global result=@add_three symbol=add_three"
  FORBIDDEN_SNIPPETS "src/00087.c"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 function_pointer local_storage indirect_call dump
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_riscv64_function_pointer_local_storage
  SRC "${BACKEND_CASE_ROOT}/riscv64_function_pointer_local_storage.c"
  TARGET_TRIPLE riscv64-linux-gnu
  OUT_TEXT "${BACKEND_TEST_BINARY_ROOT}/riscv64_function_pointer_local_storage.s"
  REQUIRED_SNIPPETS "add_three:|main:|addi sp, sp, -48|sd ra, 32(sp)|lla s1, add_three|sd s1, 0(sp)|sd s1, 8(sp)|ld t0, 8(sp)|li a0, 4|jalr ra, 0(t0)|mv s2, a0|addi t0, s2, -7|mv a0, t0|ld ra, 32(sp)|ret"
  FORBIDDEN_SNIPPETS "call add_three|src/00087.c"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 function_pointer local_storage indirect_call emission
)

c4c_add_backend_dump_flag_test(
  backend_dump_riscv64_function_pointer_return_chain
  SRC "${BACKEND_CASE_ROOT}/riscv64_function_pointer_return_chain.c"
  TARGET_TRIPLE riscv64-linux-gnu
  DUMP_FLAG --dump-prepared-bir
  REQUIRED_SNIPPETS "bir.ret ptr @sub|bir.ret ptr 0|%t0 = bir.call ptr choose(i32 1)|bir.store_local %lv.selected, ptr %t0|bir.load_local ptr %lv.selected|bir.call i32 %t1(i32 9, i32 4)|wrapper_kind=indirect|indirect_callee=%t1|home @sub value_id=5 kind=register reg=t0"
  FORBIDDEN_SNIPPETS "src/00124.c"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 function_pointer return_chain indirect_call dump
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_riscv64_function_pointer_return_chain
  SRC "${BACKEND_CASE_ROOT}/riscv64_function_pointer_return_chain.c"
  TARGET_TRIPLE riscv64-linux-gnu
  OUT_TEXT "${BACKEND_TEST_BINARY_ROOT}/riscv64_function_pointer_return_chain.s"
  REQUIRED_SNIPPETS "choose:|.Lchoose_block_2:|lla a0, sub|ret|.Lchoose_block_3:|li a0, 0|ret|main:|call choose|sd t0, 0(sp)|ld t0, 0(sp)|li a0, 9|li a1, 4|jalr ra, 0(t0)|mv s1, a0|addi t0, s1, -5|mv a0, t0|ld ra, 16(sp)|ret"
  FORBIDDEN_SNIPPETS "call sub|src/00124.c"
  CASE_TIMEOUT_SEC 10
  LABELS riscv64 function_pointer return_chain indirect_call emission
)

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

function(c4c_add_backend_rv64_runtime_case test_name)
  set(options)
  set(one_value_args SRC CASE_TIMEOUT_SEC EXPECTED_RUN_CODE)
  set(multi_value_args LABELS)
  cmake_parse_arguments(ARG "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

  if(ARG_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "c4c_add_backend_rv64_runtime_case(${test_name}) got unexpected arguments: ${ARG_UNPARSED_ARGUMENTS}")
  endif()

  if(NOT DEFINED ARG_SRC OR "${ARG_SRC}" STREQUAL "")
    message(FATAL_ERROR
      "c4c_add_backend_rv64_runtime_case(${test_name}) missing SRC")
  endif()

  if(NOT CLANG_EXECUTABLE)
    message(FATAL_ERROR
      "c4c_add_backend_rv64_runtime_case(${test_name}) requires CLANG_EXECUTABLE")
  endif()
  if(NOT QEMU_RISCV64_EXECUTABLE)
    message(FATAL_ERROR
      "c4c_add_backend_rv64_runtime_case(${test_name}) requires qemu-riscv64")
  endif()

  get_filename_component(case_stem "${ARG_SRC}" NAME_WE)
  set(case_timeout "${ARG_CASE_TIMEOUT_SEC}")
  if("${case_timeout}" STREQUAL "")
    set(case_timeout 30)
  endif()
  set(expected_run_code "${ARG_EXPECTED_RUN_CODE}")
  if("${expected_run_code}" STREQUAL "")
    set(expected_run_code 0)
  endif()

  add_test(
    NAME "${test_name}"
    COMMAND "${CMAKE_COMMAND}"
            -DCOMPILER=$<TARGET_FILE:c4cll>
            -DCLANG=${CLANG_EXECUTABLE}
            -DQEMU_RISCV64=${QEMU_RISCV64_EXECUTABLE}
            -DSRC=${ARG_SRC}
            -DTARGET_TRIPLE=riscv64-linux-gnu
            -DOUT_ASM=${CMAKE_BINARY_DIR}/backend_rv64_runtime/${case_stem}.s
            -DOUT_BIN=${CMAKE_BINARY_DIR}/backend_rv64_runtime/${case_stem}.bin
            -DSYSROOT=/usr/riscv64-linux-gnu
            -DCASE_TIMEOUT_SEC=${case_timeout}
            -DEXPECTED_RUN_CODE=${expected_run_code}
            -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_rv64_runtime_case.cmake"
  )
  c4c_set_backend_test_labels("${test_name}" backend_runtime rv64 qemu case ${ARG_LABELS})
endfunction()

function(c4c_add_backend_rv64_object_runtime_case test_name)
  set(options)
  set(one_value_args SRC CASE_TIMEOUT_SEC EXPECTED_RUN_CODE)
  set(multi_value_args LABELS)
  cmake_parse_arguments(ARG "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

  if(ARG_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "c4c_add_backend_rv64_object_runtime_case(${test_name}) got unexpected arguments: ${ARG_UNPARSED_ARGUMENTS}")
  endif()

  if(NOT DEFINED ARG_SRC OR "${ARG_SRC}" STREQUAL "")
    message(FATAL_ERROR
      "c4c_add_backend_rv64_object_runtime_case(${test_name}) missing SRC")
  endif()

  if(NOT CLANG_EXECUTABLE)
    message(FATAL_ERROR
      "c4c_add_backend_rv64_object_runtime_case(${test_name}) requires CLANG_EXECUTABLE")
  endif()
  if(NOT QEMU_RISCV64_EXECUTABLE)
    message(FATAL_ERROR
      "c4c_add_backend_rv64_object_runtime_case(${test_name}) requires qemu-riscv64")
  endif()

  get_filename_component(case_stem "${ARG_SRC}" NAME_WE)
  set(case_timeout "${ARG_CASE_TIMEOUT_SEC}")
  if("${case_timeout}" STREQUAL "")
    set(case_timeout 30)
  endif()
  set(expected_run_code "${ARG_EXPECTED_RUN_CODE}")
  if("${expected_run_code}" STREQUAL "")
    set(expected_run_code 0)
  endif()

  add_test(
    NAME "${test_name}"
    COMMAND "${CMAKE_COMMAND}"
            -DCOMPILER=$<TARGET_FILE:c4cll>
            -DCLANG=${CLANG_EXECUTABLE}
            -DQEMU_RISCV64=${QEMU_RISCV64_EXECUTABLE}
            -DSRC=${ARG_SRC}
            -DTARGET_TRIPLE=riscv64-linux-gnu
            -DOUT_OBJECT=${CMAKE_BINARY_DIR}/backend_rv64_object_runtime/${case_stem}.o
            -DOUT_BIN=${CMAKE_BINARY_DIR}/backend_rv64_object_runtime/${case_stem}.bin
            -DSYSROOT=/usr/riscv64-linux-gnu
            -DCASE_TIMEOUT_SEC=${case_timeout}
            -DEXPECTED_RUN_CODE=${expected_run_code}
            -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_rv64_object_runtime_case.cmake"
  )
  c4c_set_backend_test_labels("${test_name}" backend_runtime object rv64 qemu case ${ARG_LABELS})
endfunction()

c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_return_zero
  SRC "${BACKEND_CASE_ROOT}/return_zero.c"
  CASE_TIMEOUT_SEC 10
  LABELS scalar return
)

c4c_add_backend_rv64_object_runtime_case(
  backend_obj_runtime_rv64_return_zero
  SRC "${BACKEND_CASE_ROOT}/return_zero.c"
  CASE_TIMEOUT_SEC 10
  LABELS scalar return smoke
)

c4c_add_backend_rv64_object_runtime_case(
  backend_obj_runtime_rv64_cts_00001
  SRC "${PROJECT_SOURCE_DIR}/tests/c/external/c-testsuite/src/00001.c"
  CASE_TIMEOUT_SEC 10
  EXPECTED_RUN_CODE 0
  LABELS c_testsuite smoke scan
)

c4c_add_backend_rv64_object_runtime_case(
  backend_obj_runtime_rv64_cts_00002
  SRC "${PROJECT_SOURCE_DIR}/tests/c/external/c-testsuite/src/00002.c"
  CASE_TIMEOUT_SEC 10
  EXPECTED_RUN_CODE 0
  LABELS c_testsuite smoke scan
)

c4c_add_backend_rv64_object_runtime_case(
  backend_obj_runtime_rv64_cts_00011
  SRC "${PROJECT_SOURCE_DIR}/tests/c/external/c-testsuite/src/00011.c"
  CASE_TIMEOUT_SEC 10
  EXPECTED_RUN_CODE 0
  LABELS c_testsuite smoke scan
)

c4c_add_backend_rv64_object_runtime_case(
  backend_obj_runtime_rv64_cts_00012
  SRC "${PROJECT_SOURCE_DIR}/tests/c/external/c-testsuite/src/00012.c"
  CASE_TIMEOUT_SEC 10
  EXPECTED_RUN_CODE 0
  LABELS c_testsuite smoke scan
)

c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_return_add
  SRC "${BACKEND_CASE_ROOT}/return_add.c"
  CASE_TIMEOUT_SEC 10
  EXPECTED_RUN_CODE 5
  LABELS scalar return call alu
)
c4c_add_backend_rv64_object_runtime_case(
  backend_obj_runtime_rv64_return_add
  SRC "${BACKEND_CASE_ROOT}/return_add.c"
  CASE_TIMEOUT_SEC 10
  EXPECTED_RUN_CODE 5
  LABELS scalar return call alu scan
)
c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_two_arg_helper
  SRC "${BACKEND_CASE_ROOT}/two_arg_helper.c"
  CASE_TIMEOUT_SEC 10
  EXPECTED_RUN_CODE 12
  LABELS scalar return call
)
c4c_add_backend_rv64_object_runtime_case(
  backend_obj_runtime_rv64_two_arg_helper
  SRC "${BACKEND_CASE_ROOT}/two_arg_helper.c"
  CASE_TIMEOUT_SEC 10
  EXPECTED_RUN_CODE 12
  LABELS scalar return call scan
)
c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_two_arg_local_arg
  SRC "${BACKEND_CASE_ROOT}/two_arg_local_arg.c"
  CASE_TIMEOUT_SEC 10
  EXPECTED_RUN_CODE 12
  LABELS scalar return call local
)
c4c_add_backend_rv64_object_runtime_case(
  backend_obj_runtime_rv64_two_arg_local_arg
  SRC "${BACKEND_CASE_ROOT}/two_arg_local_arg.c"
  CASE_TIMEOUT_SEC 10
  EXPECTED_RUN_CODE 12
  LABELS scalar return call local scan
)
c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_riscv64_prepared_local_array_pointer_step
  SRC "${BACKEND_CASE_ROOT}/riscv64_prepared_local_array_pointer_step.c"
  CASE_TIMEOUT_SEC 10
  LABELS prepared_local_address array_base pointer_local pointer_step
)
c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_riscv64_loop_carried_pointer_postincrement
  SRC "${BACKEND_CASE_ROOT}/riscv64_loop_carried_pointer_postincrement.c"
  CASE_TIMEOUT_SEC 10
  LABELS prepared_local_address array_base pointer_local pointer_step loop
)
c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_riscv64_duff_fallthrough_pointer_update_producers
  SRC "${BACKEND_CASE_ROOT}/riscv64_duff_fallthrough_pointer_update_producers.c"
  CASE_TIMEOUT_SEC 10
  LABELS prepared_local_address array_base pointer_local pointer_step fallthrough
)
c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_riscv64_prepared_local_array_i8_element_access
  SRC "${BACKEND_CASE_ROOT}/riscv64_prepared_local_array_i8_element_access.c"
  CASE_TIMEOUT_SEC 10
  LABELS prepared_local_address array_subobject i8_load pointer_local
)
c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_riscv64_i16_local_array_select_store
  SRC "${BACKEND_CASE_ROOT}/riscv64_i16_local_array_select_store.c"
  CASE_TIMEOUT_SEC 10
  LABELS prepared_local_address array_subobject i16_load select store_source
)
c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_riscv64_empty_loop_exit_successor
  SRC "${BACKEND_CASE_ROOT}/riscv64_empty_loop_exit_successor.c"
  CASE_TIMEOUT_SEC 10
  LABELS prepared_control_flow loop empty_exit successor_body pointer_local switch
)
c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_riscv64_pointer_to_pointer_local_address
  SRC "${BACKEND_CASE_ROOT}/riscv64_pointer_to_pointer_local_address.c"
  CASE_TIMEOUT_SEC 10
  LABELS prepared_local_address pointer_local pointer_to_pointer
)
c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_riscv64_pointer_integer_select_chain
  SRC "${BACKEND_CASE_ROOT}/riscv64_pointer_integer_select_chain.c"
  CASE_TIMEOUT_SEC 10
  LABELS pointer select pointer_local
)
c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_riscv64_pointer_typed_select_publication
  SRC "${BACKEND_CASE_ROOT}/riscv64_pointer_typed_select_publication.c"
  CASE_TIMEOUT_SEC 10
  LABELS pointer select pointer_local
)
c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_riscv64_short_circuit_select_false_lhs
  SRC "${BACKEND_CASE_ROOT}/riscv64_short_circuit_select_false_lhs.c"
  CASE_TIMEOUT_SEC 10
  LABELS select short_circuit aggregate
)
c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_riscv64_compare_result_select_false_arm
  SRC "${BACKEND_CASE_ROOT}/riscv64_compare_result_select_false_arm.c"
  CASE_TIMEOUT_SEC 10
  LABELS select short_circuit
)
c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_riscv64_stack_homed_fused_compare_missing_false_label
  SRC "${BACKEND_CASE_ROOT}/riscv64_stack_homed_fused_compare_missing_false_label.c"
  CASE_TIMEOUT_SEC 10
  LABELS prepared_control_flow fused_compare loop stack_publication
)
c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_riscv64_aggregate_local_self_pointer_chain
  SRC "${BACKEND_CASE_ROOT}/riscv64_aggregate_local_self_pointer_chain.c"
  CASE_TIMEOUT_SEC 10
  LABELS aggregate local pointer_local aggregate_subobject
)
c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_riscv64_aggregate_local_anonymous_union_fields
  SRC "${BACKEND_CASE_ROOT}/riscv64_aggregate_local_anonymous_union_fields.c"
  CASE_TIMEOUT_SEC 10
  LABELS aggregate local aggregate_subobject anonymous_union
)
c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_riscv64_function_pointer_local_storage
  SRC "${BACKEND_CASE_ROOT}/riscv64_function_pointer_local_storage.c"
  CASE_TIMEOUT_SEC 10
  LABELS function_pointer local_storage indirect_call
)
c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_riscv64_function_pointer_return_chain
  SRC "${BACKEND_CASE_ROOT}/riscv64_function_pointer_return_chain.c"
  CASE_TIMEOUT_SEC 10
  LABELS function_pointer return_chain indirect_call
)
c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_riscv64_byval_aggregate_fixed_call
  SRC "${BACKEND_CASE_ROOT}/riscv64_byval_aggregate_fixed_call.c"
  CASE_TIMEOUT_SEC 10
  LABELS byval aggregate call
)
c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_riscv64_byval_preserved_pointer_args
  SRC "${BACKEND_CASE_ROOT}/riscv64_byval_preserved_pointer_args.c"
  CASE_TIMEOUT_SEC 10
  LABELS byval aggregate call pointer prepared_call_boundary
)
c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_riscv64_byval_formal_gpr_publication
  SRC "${BACKEND_CASE_ROOT}/riscv64_byval_formal_gpr_publication.c"
  CASE_TIMEOUT_SEC 10
  LABELS byval aggregate call formal_publication
)
c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_riscv64_external_strlen_runtime_link_policy
  SRC "${BACKEND_CASE_ROOT}/riscv64_external_strlen_runtime_link.c"
  CASE_TIMEOUT_SEC 10
  LABELS extern_call string_literal direct_call external_policy expected_repair
)
c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_two_arg_both_local_arg
  SRC "${BACKEND_CASE_ROOT}/two_arg_both_local_arg.c"
  CASE_TIMEOUT_SEC 10
  EXPECTED_RUN_CODE 12
  LABELS scalar return call local
)
c4c_add_backend_rv64_object_runtime_case(
  backend_obj_runtime_rv64_two_arg_both_local_arg
  SRC "${BACKEND_CASE_ROOT}/two_arg_both_local_arg.c"
  CASE_TIMEOUT_SEC 10
  EXPECTED_RUN_CODE 12
  LABELS scalar return call local scan
)
c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_two_arg_second_local_arg
  SRC "${BACKEND_CASE_ROOT}/two_arg_second_local_arg.c"
  CASE_TIMEOUT_SEC 10
  EXPECTED_RUN_CODE 12
  LABELS scalar return call local
)
c4c_add_backend_rv64_object_runtime_case(
  backend_obj_runtime_rv64_two_arg_second_local_arg
  SRC "${BACKEND_CASE_ROOT}/two_arg_second_local_arg.c"
  CASE_TIMEOUT_SEC 10
  EXPECTED_RUN_CODE 12
  LABELS scalar return call local scan
)
c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_local_arg_call
  SRC "${BACKEND_CASE_ROOT}/local_arg_call.c"
  CASE_TIMEOUT_SEC 10
  EXPECTED_RUN_CODE 6
  LABELS scalar return call local
)
c4c_add_backend_rv64_object_runtime_case(
  backend_obj_runtime_rv64_local_arg_call
  SRC "${BACKEND_CASE_ROOT}/local_arg_call.c"
  CASE_TIMEOUT_SEC 10
  EXPECTED_RUN_CODE 6
  LABELS scalar return call local scan
)
c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_return_add_sub_chain
  SRC "${BACKEND_CASE_ROOT}/return_add_sub_chain.c"
  CASE_TIMEOUT_SEC 10
  EXPECTED_RUN_CODE 4
  LABELS scalar return alu
)
c4c_add_backend_rv64_object_runtime_case(
  backend_obj_runtime_rv64_return_add_sub_chain
  SRC "${BACKEND_CASE_ROOT}/return_add_sub_chain.c"
  CASE_TIMEOUT_SEC 10
  EXPECTED_RUN_CODE 4
  LABELS scalar return alu scan
)
c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_local_temp
  SRC "${BACKEND_CASE_ROOT}/local_temp.c"
  CASE_TIMEOUT_SEC 10
  EXPECTED_RUN_CODE 5
  LABELS scalar local
)
c4c_add_backend_rv64_object_runtime_case(
  backend_obj_runtime_rv64_local_temp
  SRC "${BACKEND_CASE_ROOT}/local_temp.c"
  CASE_TIMEOUT_SEC 10
  EXPECTED_RUN_CODE 5
  LABELS scalar local scan
)
c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_local_array
  SRC "${BACKEND_CASE_ROOT}/local_array.c"
  CASE_TIMEOUT_SEC 10
  EXPECTED_RUN_CODE 7
  LABELS local memory array
)
c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_local_pointer_deref
  SRC "${BACKEND_CASE_ROOT}/local_pointer_deref.c"
  CASE_TIMEOUT_SEC 10
  EXPECTED_RUN_CODE 7
  LABELS local memory pointer
)
c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_local_dynamic_member_array
  SRC "${BACKEND_CASE_ROOT}/local_dynamic_member_array.c"
  CASE_TIMEOUT_SEC 10
  EXPECTED_RUN_CODE 11
  LABELS local memory pointer call
)
c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_global_load
  SRC "${BACKEND_CASE_ROOT}/global_load.c"
  CASE_TIMEOUT_SEC 10
  EXPECTED_RUN_CODE 11
  LABELS global memory load
)
c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_global_load_zero_init
  SRC "${BACKEND_CASE_ROOT}/global_load_zero_init.c"
  CASE_TIMEOUT_SEC 10
  LABELS global memory load
)
c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_defined_global_array
  SRC "${BACKEND_CASE_ROOT}/defined_global_array.c"
  CASE_TIMEOUT_SEC 10
  EXPECTED_RUN_CODE 7
  LABELS global memory array load
)
c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_defined_global_array_store
  SRC "${BACKEND_CASE_ROOT}/defined_global_array_store.c"
  CASE_TIMEOUT_SEC 10
  EXPECTED_RUN_CODE 9
  LABELS global memory array store
)
c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_defined_global_array_pointer
  SRC "${BACKEND_CASE_ROOT}/defined_global_array_pointer.c"
  CASE_TIMEOUT_SEC 10
  EXPECTED_RUN_CODE 7
  LABELS global memory array pointer load
)
c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_defined_global_array_pointer_store
  SRC "${BACKEND_CASE_ROOT}/defined_global_array_pointer_store.c"
  CASE_TIMEOUT_SEC 10
  EXPECTED_RUN_CODE 9
  LABELS global memory array pointer store
)
c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_global_store
  SRC "${BACKEND_CASE_ROOT}/global_store.c"
  CASE_TIMEOUT_SEC 10
  EXPECTED_RUN_CODE 7
  LABELS global memory store
)
c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_riscv64_global_store_prepared_value_preserves_source
  SRC "${BACKEND_CASE_ROOT}/riscv64_global_store_prepared_value_preserves_source.c"
  CASE_TIMEOUT_SEC 10
  EXPECTED_RUN_CODE 0
  LABELS global memory store prepared_value
)
c4c_add_backend_rv64_object_runtime_case(
  backend_obj_runtime_rv64_global_store_prepared_value_preserves_source
  SRC "${BACKEND_CASE_ROOT}/riscv64_global_store_prepared_value_preserves_source.c"
  CASE_TIMEOUT_SEC 10
  EXPECTED_RUN_CODE 0
  LABELS global memory store prepared_value scan
)
c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_packed_local_member_offsets
  SRC "${BACKEND_CASE_ROOT}/packed_local_member_offsets.c"
  CASE_TIMEOUT_SEC 10
  EXPECTED_RUN_CODE 8
  LABELS local memory packed
)
c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_branch_if_eq
  SRC "${BACKEND_CASE_ROOT}/branch_if_eq.c"
  CASE_TIMEOUT_SEC 10
  LABELS branch basic_control_flow
)
c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_branch_if_ne
  SRC "${BACKEND_CASE_ROOT}/branch_if_ne.c"
  CASE_TIMEOUT_SEC 10
  LABELS branch basic_control_flow
)
c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_branch_if_lt
  SRC "${BACKEND_CASE_ROOT}/branch_if_lt.c"
  CASE_TIMEOUT_SEC 10
  LABELS branch basic_control_flow signed_compare
)
c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_branch_if_le
  SRC "${BACKEND_CASE_ROOT}/branch_if_le.c"
  CASE_TIMEOUT_SEC 10
  LABELS branch basic_control_flow signed_compare
)
c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_branch_if_gt
  SRC "${BACKEND_CASE_ROOT}/branch_if_gt.c"
  CASE_TIMEOUT_SEC 10
  LABELS branch basic_control_flow signed_compare
)
c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_branch_if_ge
  SRC "${BACKEND_CASE_ROOT}/branch_if_ge.c"
  CASE_TIMEOUT_SEC 10
  LABELS branch basic_control_flow signed_compare
)
c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_branch_if_ule
  SRC "${BACKEND_CASE_ROOT}/branch_if_ule.c"
  CASE_TIMEOUT_SEC 10
  LABELS branch basic_control_flow unsigned_compare
)
c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_branch_if_ult
  SRC "${BACKEND_CASE_ROOT}/branch_if_ult.c"
  CASE_TIMEOUT_SEC 10
  LABELS branch basic_control_flow unsigned_compare
)
c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_branch_if_ugt
  SRC "${BACKEND_CASE_ROOT}/branch_if_ugt.c"
  CASE_TIMEOUT_SEC 10
  LABELS branch basic_control_flow unsigned_compare
)
c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_branch_if_uge
  SRC "${BACKEND_CASE_ROOT}/branch_if_uge.c"
  CASE_TIMEOUT_SEC 10
  LABELS branch basic_control_flow unsigned_compare
)

add_test(
  NAME backend_codegen_route_aarch64_global_function_pointer_table_selected_indirect_call
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DTARGET_TRIPLE=aarch64-linux-gnu
          -DSRC=${BACKEND_CASE_ROOT}/aarch64_global_function_pointer_table_selected_indirect_call.c
          -DOUT_ASM=${BACKEND_TEST_BINARY_ROOT}/aarch64_global_function_pointer_table_selected_indirect_call.s
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_aarch64_selected_indirect_call_case.cmake"
)
c4c_set_backend_test_labels(
  backend_codegen_route_aarch64_global_function_pointer_table_selected_indirect_call
  backend_route
  aarch64
  selected_indirect_call
)

add_subdirectory(mir)

add_test(
  NAME backend_codegen_route_aarch64_hfa_result_home_publication_contract
  COMMAND $<TARGET_FILE:backend_aarch64_call_boundary_owner_test>
)
c4c_set_backend_test_labels(
  backend_codegen_route_aarch64_hfa_result_home_publication_contract
  backend_route
  aarch64
  hfa
  aggregate_return
  prepared_call_boundary
  contract
)

add_subdirectory(bir)

get_property(C4C_BACKEND_MIR_TARGETS
  DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/mir"
  PROPERTY BUILDSYSTEM_TARGETS
)
get_property(C4C_BACKEND_BIR_TARGETS
  DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bir"
  PROPERTY BUILDSYSTEM_TARGETS
)
add_custom_target(c4c_backend_tests
  DEPENDS ${C4C_BACKEND_MIR_TARGETS} ${C4C_BACKEND_BIR_TARGETS}
)
