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
  REQUIRED_SNIPPETS "arg:|bl fa_s16|adrp x10, s17|ldrb w9, [x10]|strb w9, [sp, #136]|ldrb w9, [x10, #16]|strb w9, [sp, #152]|add x0, sp, #136|bl fa_s17"
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
  REQUIRED_SNIPPETS "call_hfa_globals:|adrp x9, global_float2|ldr s0, [x9]|ldr s1, [x9, #4]|bl take_float2|adrp x9, global_double2|ldr d0, [x9]|ldr d1, [x9, #8]|bl take_double2"
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
  REQUIRED_SNIPPETS "call_f128_hfa_globals:|adrp x9, global_f128_2|ldr q0, [x9]|ldr q1, [x9, #16]|bl take_f128_2"
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

c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_return_zero
  SRC "${BACKEND_CASE_ROOT}/return_zero.c"
  CASE_TIMEOUT_SEC 10
  LABELS scalar return
)
c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_return_add
  SRC "${BACKEND_CASE_ROOT}/return_add.c"
  CASE_TIMEOUT_SEC 10
  EXPECTED_RUN_CODE 5
  LABELS scalar return call alu
)
c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_return_add_sub_chain
  SRC "${BACKEND_CASE_ROOT}/return_add_sub_chain.c"
  CASE_TIMEOUT_SEC 10
  EXPECTED_RUN_CODE 4
  LABELS scalar return alu
)
c4c_add_backend_rv64_runtime_case(
  backend_rv64_runtime_local_temp
  SRC "${BACKEND_CASE_ROOT}/local_temp.c"
  CASE_TIMEOUT_SEC 10
  EXPECTED_RUN_CODE 5
  LABELS scalar local
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
