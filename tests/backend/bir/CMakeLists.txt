add_executable(backend_prepare_phi_materialize_test
    "${BACKEND_TEST_ROOT}/bir/backend_prepare_phi_materialize_test.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/bir/bir.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/bir/bir_printer.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/prealloc/legalize.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/prealloc/out_of_ssa.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/prealloc/prepared_printer.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/prealloc/prepared_printer/addressing.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/prealloc/prepared_printer/atomics.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/prealloc/prepared_printer/calls.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/prealloc/prepared_printer/control_flow.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/prealloc/prepared_printer/frame.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/prealloc/prepared_printer/functions.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/prealloc/prepared_printer/inline_asm.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/prealloc/prepared_printer/intrinsics.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/prealloc/prepared_printer/regalloc.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/prealloc/prepared_printer/runtime_helpers.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/prealloc/prepared_printer/special_carriers.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/prealloc/prepared_printer/storage.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/prealloc/prepared_printer/value_locations.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/prealloc/prepared_printer/variadic.cpp"
)
target_include_directories(backend_prepare_phi_materialize_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
)
target_link_libraries(backend_prepare_phi_materialize_test PRIVATE
    c4c_codegen
)
set_target_properties(backend_prepare_phi_materialize_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

add_test(
  NAME backend_prepare_phi_materialize
  COMMAND $<TARGET_FILE:backend_prepare_phi_materialize_test>
)
set_tests_properties(backend_prepare_phi_materialize PROPERTIES
  LABELS "backend;internal;cpp"
)

add_executable(backend_prepare_block_only_control_flow_test
    "${BACKEND_TEST_ROOT}/bir/backend_prepare_block_only_control_flow_test.cpp"
)
target_include_directories(backend_prepare_block_only_control_flow_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
)
target_link_libraries(backend_prepare_block_only_control_flow_test PRIVATE
    c4c_backend
)
set_target_properties(backend_prepare_block_only_control_flow_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

add_test(
  NAME backend_prepare_block_only_control_flow
  COMMAND $<TARGET_FILE:backend_prepare_block_only_control_flow_test>
)
set_tests_properties(backend_prepare_block_only_control_flow PROPERTIES
  LABELS "backend;internal;cpp"
)

add_executable(backend_prepare_authoritative_join_ownership_test
    "${BACKEND_TEST_ROOT}/bir/backend_prepare_authoritative_join_ownership_test.cpp"
)
target_include_directories(backend_prepare_authoritative_join_ownership_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
)
target_link_libraries(backend_prepare_authoritative_join_ownership_test PRIVATE
    c4c_backend
)
set_target_properties(backend_prepare_authoritative_join_ownership_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

add_test(
  NAME backend_prepare_authoritative_join_ownership
  COMMAND $<TARGET_FILE:backend_prepare_authoritative_join_ownership_test>
)
set_tests_properties(backend_prepare_authoritative_join_ownership PROPERTIES
  LABELS "backend;internal;cpp"
)

add_executable(backend_lir_to_bir_notes_test
    "${BACKEND_TEST_ROOT}/bir/backend_lir_to_bir_notes_test.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/bir/bir.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/bir/bir_validate.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/bir/lir_to_bir.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/bir/lir_to_bir/aggregate.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/bir/lir_to_bir/analysis.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/bir/lir_to_bir/call_abi.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/bir/lir_to_bir/calling.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/bir/lir_to_bir/cfg.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/bir/lir_to_bir/context.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/bir/lir_to_bir/global_initializers.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/bir/lir_to_bir/globals.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/bir/lir_to_bir/memory/addressing.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/bir/lir_to_bir/memory/coordinator.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/bir/lir_to_bir/memory/intrinsics.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/bir/lir_to_bir/memory/local_gep.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/bir/lir_to_bir/memory/local_slots.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/bir/lir_to_bir/memory/provenance.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/bir/lir_to_bir/memory/value_materialization.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/bir/lir_to_bir/module.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/bir/lir_to_bir/scalar.cpp"
    "${PROJECT_SOURCE_DIR}/src/backend/bir/lir_to_bir/types.cpp"
)
target_include_directories(backend_lir_to_bir_notes_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
)
target_link_libraries(backend_lir_to_bir_notes_test PRIVATE
    c4c_codegen
)
set_target_properties(backend_lir_to_bir_notes_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

add_test(
  NAME backend_lir_to_bir_notes
  COMMAND $<TARGET_FILE:backend_lir_to_bir_notes_test>
)
set_tests_properties(backend_lir_to_bir_notes PROPERTIES
  LABELS "backend;internal;cpp"
)

add_executable(backend_prepare_structured_context_test
    "${BACKEND_TEST_ROOT}/bir/backend_prepare_structured_context_test.cpp"
)
target_include_directories(backend_prepare_structured_context_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
)
target_link_libraries(backend_prepare_structured_context_test PRIVATE
    c4c_backend
)
set_target_properties(backend_prepare_structured_context_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

add_test(
  NAME backend_prepare_structured_context
  COMMAND $<TARGET_FILE:backend_prepare_structured_context_test>
)
set_tests_properties(backend_prepare_structured_context PROPERTIES
  LABELS "backend;internal;cpp"
)

add_executable(backend_prepare_liveness_test
    "${BACKEND_TEST_ROOT}/bir/backend_prepare_liveness_test.cpp"
)
target_include_directories(backend_prepare_liveness_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
)
target_link_libraries(backend_prepare_liveness_test PRIVATE
    c4c_backend
)
set_target_properties(backend_prepare_liveness_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

add_test(
  NAME backend_prepare_liveness
  COMMAND $<TARGET_FILE:backend_prepare_liveness_test>
)
set_tests_properties(backend_prepare_liveness PROPERTIES
  LABELS "backend;internal;cpp"
)

add_executable(backend_prepare_stack_layout_test
    "${BACKEND_TEST_ROOT}/bir/backend_prepare_stack_layout_test.cpp"
)
target_include_directories(backend_prepare_stack_layout_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
)
target_link_libraries(backend_prepare_stack_layout_test PRIVATE
    c4c_backend
)
set_target_properties(backend_prepare_stack_layout_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

add_test(
  NAME backend_prepare_stack_layout
  COMMAND $<TARGET_FILE:backend_prepare_stack_layout_test>
)
set_tests_properties(backend_prepare_stack_layout PROPERTIES
  LABELS "backend;internal;cpp"
)

add_executable(backend_prepare_frame_stack_call_contract_test
    "${BACKEND_TEST_ROOT}/bir/backend_prepare_frame_stack_call_contract_test.cpp"
)
target_include_directories(backend_prepare_frame_stack_call_contract_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
)
target_link_libraries(backend_prepare_frame_stack_call_contract_test PRIVATE
    c4c_backend
)
set_target_properties(backend_prepare_frame_stack_call_contract_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

add_test(
  NAME backend_prepare_frame_stack_call_contract
  COMMAND $<TARGET_FILE:backend_prepare_frame_stack_call_contract_test>
)
set_tests_properties(backend_prepare_frame_stack_call_contract PROPERTIES
  LABELS "backend;internal;cpp"
)

add_executable(backend_prepared_printer_test
    "${BACKEND_TEST_ROOT}/bir/backend_prepared_printer_test.cpp"
)
target_include_directories(backend_prepared_printer_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
)
target_link_libraries(backend_prepared_printer_test PRIVATE
    c4c_backend
)
set_target_properties(backend_prepared_printer_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

add_test(
  NAME backend_prepared_printer
  COMMAND $<TARGET_FILE:backend_prepared_printer_test>
)
set_tests_properties(backend_prepared_printer PROPERTIES
  LABELS "backend;internal;cpp"
)

add_executable(backend_x86_prepared_handoff_label_authority_test
    "${BACKEND_TEST_ROOT}/bir/backend_x86_prepared_handoff_label_authority_test.cpp"
)
target_include_directories(backend_x86_prepared_handoff_label_authority_test PRIVATE
    "${PROJECT_SOURCE_DIR}"
)
target_link_libraries(backend_x86_prepared_handoff_label_authority_test PRIVATE
    c4c_backend
)
set_target_properties(backend_x86_prepared_handoff_label_authority_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

add_test(
  NAME backend_x86_prepared_handoff_label_authority
  COMMAND $<TARGET_FILE:backend_x86_prepared_handoff_label_authority_test>
)
set_tests_properties(backend_x86_prepared_handoff_label_authority PROPERTIES
  LABELS "backend;internal;cpp;x86"
)

add_test(
  NAME backend_cli_dump_bir_is_semantic
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DTARGET_TRIPLE=x86_64-unknown-linux-gnu
          -DDUMP_FLAG=--dump-bir
          -DSRC=${PROJECT_SOURCE_DIR}/tests/c/internal/compare_case/smoke_expr_branch_lifecycle.c
          "-DREQUIRED_SNIPPETS=bir.func @bump(ptr %p.slot, i32 %p.delta) -> i32 {|bir.func @run() -> i32 {|bir.phi i32"
          "-DFORBIDDEN_SNIPPETS=prepared.module|--- prepared-bir ---"
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_dump_flag_case.cmake"
)
set_tests_properties(backend_cli_dump_bir_is_semantic PROPERTIES
  LABELS "backend;internal;cli"
)

add_test(
  NAME backend_cli_dump_prepared_bir_is_prepared
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DTARGET_TRIPLE=x86_64-unknown-linux-gnu
          -DDUMP_FLAG=--dump-prepared-bir
          -DSRC=${PROJECT_SOURCE_DIR}/tests/c/internal/compare_case/smoke_expr_branch_lifecycle.c
          "-DREQUIRED_SNIPPETS=prepared.module target=x86_64-unknown-linux-gnu|--- prepared-bir ---|prepared-control-flow"
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_dump_flag_case.cmake"
)
set_tests_properties(backend_cli_dump_prepared_bir_is_prepared PROPERTIES
  LABELS "backend;internal;cli"
)

add_test(
  NAME backend_cli_dump_prepared_bir_exposes_contract_sections
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DTARGET_TRIPLE=x86_64-unknown-linux-gnu
          -DDUMP_FLAG=--dump-prepared-bir
          -DSRC=${PROJECT_SOURCE_DIR}/tests/c/internal/compare_case/smoke_expr_branch_lifecycle.c
          "-DREQUIRED_SNIPPETS=prepared.module target=x86_64-unknown-linux-gnu|--- prepared-bir ---|--- prepared-function-summaries ---|--- prepared-control-flow ---|--- prepared-value-locations ---|--- prepared-stack-layout ---|--- prepared-frame-plan ---|--- prepared-dynamic-stack-plan ---|--- prepared-call-plans ---|--- prepared-storage-plans ---|--- prepared-addressing ---"
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_dump_flag_case.cmake"
)
set_tests_properties(backend_cli_dump_prepared_bir_exposes_contract_sections PROPERTIES
  LABELS "backend;internal;cli"
)

add_test(
  NAME backend_cli_dump_bir_vla_goto_stackrestore_cfg
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DTARGET_TRIPLE=x86_64-unknown-linux-gnu
          -DDUMP_FLAG=--dump-bir
          -DSRC=${PROJECT_SOURCE_DIR}/tests/backend/case/vla_goto_stackrestore.c
          "-DREQUIRED_SNIPPETS=bir.func @accumulate_vla(i32 %p.n) -> i32 {|entry:|ulbl_loop:|block_1:|block_2:|block_3:|%t0 = bir.call ptr llvm.stacksave()|%t3 = bir.call ptr llvm.dynamic_alloca.i32(i64 %t2)|bir.call void llvm.stackrestore(ptr %t0)|bir.cond_br i1 %t17, block_2, block_3|bir.br block_1"
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_dump_flag_case.cmake"
)
set_tests_properties(backend_cli_dump_bir_vla_goto_stackrestore_cfg PROPERTIES
  LABELS "backend;internal;cli"
)

add_test(
  NAME backend_cli_dump_bir_layout_sensitive_aggregate
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DTARGET_TRIPLE=x86_64-unknown-linux-gnu
          -DDUMP_FLAG=--dump-bir
          -DSRC=${PROJECT_SOURCE_DIR}/tests/backend/case/aggregate_param_return_pair.c
          "-DREQUIRED_SNIPPETS=bir.func @id_pair(ptr sret(size=8, align=4) %ret.sret, ptr byval(size=8, align=4) %p.p) -> void {|%lv.param.p.p.aggregate.param.copy.4 = bir.load_local i32 %lv.param.p.p.4, addr %p.p+4|bir.store_local %lv.param.p.p.4, i32 id_pair.ret.sret.copy.4, addr %ret.sret+4|bir.call void id_pair(ptr sret(size=8, align=4) %t3, ptr byval(size=8, align=4) %lv.p)"
          "-DFORBIDDEN_SNIPPETS=prepared.module|--- prepared-control-flow ---|define %struct.Pair @id_pair|call %struct.Pair"
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_dump_flag_case.cmake"
)
set_tests_properties(backend_cli_dump_bir_layout_sensitive_aggregate PROPERTIES
  LABELS "backend;internal;cli"
)

add_test(
  NAME backend_cli_dump_prepared_bir_vla_goto_stackrestore_cfg
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DTARGET_TRIPLE=x86_64-unknown-linux-gnu
          -DDUMP_FLAG=--dump-prepared-bir
          "-DEXTRA_ARGS=--mir-focus-function;accumulate_vla"
          -DSRC=${PROJECT_SOURCE_DIR}/tests/backend/case/vla_goto_stackrestore.c
          "-DREQUIRED_SNIPPETS=prepared.module target=x86_64-unknown-linux-gnu route=semantic_bir_shared|--- prepared-function-summaries ---|prepared.summary @accumulate_vla stable_base=fp frame_size=24 frame_alignment=8 has_dynamic_stack=yes|dynamic_stack block=entry inst=1 kind=stack_save result=%t0|dynamic_stack block=block_1 inst=2 kind=dynamic_alloca result=%t3 operand=%t2|--- prepared-control-flow ---|prepared.func @accumulate_vla|block entry terminator=branch target=ulbl_loop|block ulbl_loop terminator=branch target=block_1|block block_1 terminator=cond_branch true=block_2 false=block_3|block block_2 terminator=branch target=block_1|block block_3 terminator=return|--- prepared-frame-plan ---|prepared.func @accumulate_vla frame_size=24 frame_alignment=8 has_dynamic_stack=yes fixed_slots_use_fp=yes|--- prepared-dynamic-stack-plan ---|prepared.func @accumulate_vla requires_stack_save_restore=yes fixed_slots_use_fp=yes|dynamic_stack_op block=entry inst_index=1 kind=stack_save result=%t0|dynamic_stack_op block=block_1 inst_index=2 kind=dynamic_alloca result=%t3 operand=%t2 alloc_type=i32|dynamic_stack_op block=block_2 inst_index=0 kind=stack_restore operand=%t0|--- prepared-addressing ---|prepared.func @accumulate_vla frame_size="
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_dump_flag_case.cmake"
)
set_tests_properties(backend_cli_dump_prepared_bir_vla_goto_stackrestore_cfg PROPERTIES
  LABELS "backend;internal;cli"
)

add_test(
  NAME backend_cli_dump_prepared_bir_local_arg_call_contract
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DTARGET_TRIPLE=x86_64-unknown-linux-gnu
          -DDUMP_FLAG=--dump-prepared-bir
          "-DEXTRA_ARGS=--mir-focus-function;main"
          -DSRC=${PROJECT_SOURCE_DIR}/tests/backend/case/local_arg_call.c
          "-DREQUIRED_SNIPPETS=--- prepared-function-summaries ---|prepared.summary @main stable_base=rsp frame_size=4 frame_alignment=4 has_dynamic_stack=no|callsite block=0 inst=2 wrapper=same_module callee=add_one variadic_fpr_args=0 args=1 result_bank=gpr|arg0 bank=gpr from=register:r11 to=rdi|result bank=gpr from=register:rax to=register:rbx|storage %t0 register bank=gpr placement=gpr:caller_saved#0/w1 reg=r11|storage %t1 register bank=gpr placement=gpr:callee_saved#0/w1 reg=rbx|--- prepared-call-plans ---|prepared.func @main|call block_index=0 inst_index=2 wrapper_kind=same_module variadic_fpr_arg_register_count=0 indirect=no callee=add_one|--- prepared-storage-plans ---|storage %t0 value_id=2 encoding=register bank=gpr placement=gpr:caller_saved#0/w1 reg=r11|storage %t1 value_id=3 encoding=register bank=gpr placement=gpr:callee_saved#0/w1 reg=rbx"
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_dump_flag_case.cmake"
)
set_tests_properties(backend_cli_dump_prepared_bir_local_arg_call_contract PROPERTIES
  LABELS "backend;internal;cli"
)

add_test(
  NAME backend_cli_dump_bir_00204_stdarg_semantic_handoff
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DTARGET_TRIPLE=x86_64-unknown-linux-gnu
          -DDUMP_FLAG=--dump-bir
          -DSRC=${PROJECT_SOURCE_DIR}/tests/c/external/c-testsuite/src/00204.c
          "-DREQUIRED_SNIPPETS=bir.func @stdarg() -> void {|%t1 = bir.call i32 printf(ptr @.str48)|bir.call void myprintf(ptr @.str49|bir.call i32 match(ptr %lv.s, ptr"
          "-DFORBIDDEN_SNIPPETS=prepared.module|--- prepared-control-flow ---|x86 route summary|x86 route trace"
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_dump_flag_case.cmake"
)
set_tests_properties(backend_cli_dump_bir_00204_stdarg_semantic_handoff PROPERTIES
  LABELS "backend;internal;cli"
)

add_test(
  NAME backend_cli_dump_bir_00204_stdarg_movi_zext_immediate_fold
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DTARGET_TRIPLE=aarch64-linux-gnu
          -DDUMP_FLAG=--dump-bir
          "-DEXTRA_ARGS=--mir-focus-function;movi"
          -DSRC=${PROJECT_SOURCE_DIR}/tests/c/external/c-testsuite/src/00204.c
          "-DREQUIRED_SNIPPETS=focus function: movi|focused functions matched: 1|bir.func @movi() -> void {|bir.call void pll(i64 2882338816)|bir.call void pll(i64 2863311530)"
          "-DFORBIDDEN_SNIPPETS=bir.call void pll(i64 -1412628480)|bir.call void pll(i64 -1431655766)"
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_dump_flag_case.cmake"
)
set_tests_properties(backend_cli_dump_bir_00204_stdarg_movi_zext_immediate_fold PROPERTIES
  LABELS "backend;internal;cli"
)

add_test(
  NAME backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DTARGET_TRIPLE=x86_64-unknown-linux-gnu
          -DDUMP_FLAG=--dump-prepared-bir
          -DSRC=${PROJECT_SOURCE_DIR}/tests/c/external/c-testsuite/src/00204.c
          "-DREQUIRED_SNIPPETS=prepared.module target=x86_64-unknown-linux-gnu route=semantic_bir_shared|completed_phases: legalize stack_layout liveness out_of_ssa regalloc|[stack_layout] stack layout prepared function 'stdarg'|[regalloc] regalloc seeded function 'stdarg'|bir.func @stdarg() -> void {|bir.call void myprintf(ptr @.str49|--- prepared-control-flow ---|prepared.func @stdarg|prepared.func @stdarg frame_size=|access block=entry"
          "-DFORBIDDEN_SNIPPETS=x86 route summary|x86 route trace"
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_dump_flag_case.cmake"
)
set_tests_properties(backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff PROPERTIES
  LABELS "backend;internal;cli"
)

add_test(
  NAME backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DTARGET_TRIPLE=aarch64-linux-gnu
          -DDUMP_FLAG=--dump-prepared-bir
          -DSRC=${PROJECT_SOURCE_DIR}/tests/c/external/c-testsuite/src/00204.c
          "-DREQUIRED_SNIPPETS=prepared.module target=aarch64-linux-gnu route=semantic_bir_shared|%t22 = bir.add ptr %t21, 1|%t24 = bir.add ptr %t23, 1|%t34 = bir.add ptr %t31, -1|bir.call i32 match(ptr %lv.s, ptr @.str30)|arg1 bank=gpr from=symbol_address:@.str30 to=x1|branch_condition logic.end.8 kind=fused_compare condition=%t20 compare=ne i32 %t19, 0|branch_condition block_66 kind=fused_compare condition=%t30 compare=ne i32 %t29, 0|join_transfer logic.end.8 result=%t19 kind=phi_edge carrier=select_materialization|incoming [logic.rhs.end.7] -> %t15|address_materialization block=vaarg.join.14 inst_index=29 kind=frame_slot result=%lv.t7.0|call block_index=0 inst_index=491 wrapper_kind=same_module variadic_fpr_arg_register_count=7 indirect=no callee=myprintf|arg index=8 value_bank=vreg source_encoding=frame_slot source_value_id=2504 source_slot=#3131 source_stack_offset=9232 source_bank=fpr dest_bank=none dest_stack_offset=0|arg index=12 value_bank=vreg source_encoding=frame_slot source_value_id=2516 source_slot=#3143 source_stack_offset=9424 source_bank=fpr dest_bank=none dest_stack_offset=64"
          "-DFORBIDDEN_SNIPPETS=match(ptr %lv.s, ptr %t5)|arg index=8 value_bank=vreg source_encoding=frame_slot source_value_id=2504 source_slot=#3131 source_stack_offset=9232 source_bank=fpr dest_placement=vreg:call_argument#7/w1 dest_reg=q7|x86 route summary|x86 route trace"
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_dump_flag_case.cmake"
)
set_tests_properties(
  backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication
  PROPERTIES LABELS "backend;internal;cli"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_aarch64_pointer_value_named_scalar_writeback_uses_computed_store_value
  SRC "${BACKEND_CASE_ROOT}/aarch64_pointer_value_named_scalar_writeback.c"
  TARGET_TRIPLE aarch64-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/aarch64_pointer_value_named_scalar_writeback.s"
  REQUIRED_SNIPPETS "writeback_data:|str w20, [x21]"
  FORBIDDEN_SNIPPETS "sub x9, x9, x10|add x20, x20, x9"
  LABELS aarch64 pointer_value_store writeback
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_aarch64_scalar_fp_literal_add_publishes_fpr_result
  SRC "${BACKEND_CASE_ROOT}/aarch64_scalar_fp_literal_add_publication.c"
  TARGET_TRIPLE aarch64-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/aarch64_scalar_fp_literal_add_publication.s"
  REQUIRED_SNIPPETS "fadd d|bl printf"
  FORBIDDEN_SNIPPETS ""
  LABELS aarch64 scalar_fp publication
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_aarch64_local_aggregate_address_pointer_copy_publishes_frame_address
  SRC "${BACKEND_CASE_ROOT}/aarch64_local_aggregate_address_pointer_copy.c"
  TARGET_TRIPLE aarch64-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/aarch64_local_aggregate_address_pointer_copy.s"
  REQUIRED_SNIPPETS "add x9, sp, #0|str x9, [sp, #56]|ldrb w9, [x10]"
  FORBIDDEN_SNIPPETS ""
  LABELS aarch64 aggregate local_address publication
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor
  SRC "${BACKEND_CASE_ROOT}/aarch64_dynamic_stack_fixed_slot_uses_fp_anchor.c"
  TARGET_TRIPLE aarch64-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/aarch64_dynamic_stack_fixed_slot_uses_fp_anchor.s"
  REQUIRED_SNIPPETS "dynamic_stack_fixed_slot_uses_fp_anchor:|mov sp, x17|add w13, w13, #1"
  FORBIDDEN_SNIPPETS "str w0, [sp, #8]|ldr w13, [sp, #8]|str w13, [sp, #8]|ldr w20, [sp, #8]"
  LABELS aarch64 dynamic_stack vla fixed_slot
)

add_test(
  NAME backend_cli_dump_bir_focus_function_filters_00204
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DTARGET_TRIPLE=x86_64-unknown-linux-gnu
          -DDUMP_FLAG=--dump-bir
          "-DEXTRA_ARGS=--mir-focus-function;stdarg"
          -DSRC=${PROJECT_SOURCE_DIR}/tests/c/external/c-testsuite/src/00204.c
          "-DREQUIRED_SNIPPETS=focus function: stdarg|focused functions matched: 1|bir.func @stdarg() -> void {|%t1 = bir.call i32 printf(ptr @.str48)|bir.call void myprintf(ptr @.str49"
          "-DFORBIDDEN_SNIPPETS=bir.func @myprintf|bir.func @remove(ptr %p.__filename)"
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_dump_flag_case.cmake"
)
set_tests_properties(backend_cli_dump_bir_focus_function_filters_00204 PROPERTIES
  LABELS "backend;internal;cli"
)

add_test(
  NAME backend_cli_dump_prepared_bir_focus_function_filters_00204
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DTARGET_TRIPLE=x86_64-unknown-linux-gnu
          -DDUMP_FLAG=--dump-prepared-bir
          "-DEXTRA_ARGS=--mir-focus-function;stdarg"
          -DSRC=${PROJECT_SOURCE_DIR}/tests/c/external/c-testsuite/src/00204.c
          "-DREQUIRED_SNIPPETS=focus function: stdarg|focused functions matched: 1|prepared.module target=x86_64-unknown-linux-gnu route=semantic_bir_shared|[stack_layout] stack layout prepared function 'stdarg'|[regalloc] regalloc seeded function 'stdarg'|bir.func @stdarg() -> void {|prepared.func @stdarg|access block=entry"
          "-DFORBIDDEN_SNIPPETS=stack layout prepared function 'fa_s1'|regalloc seeded function 'myprintf'"
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_dump_flag_case.cmake"
)
set_tests_properties(backend_cli_dump_prepared_bir_focus_function_filters_00204 PROPERTIES
  LABELS "backend;internal;cli"
)

add_test(
  NAME backend_cli_dump_bir_focus_block_entry_00204
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DTARGET_TRIPLE=x86_64-unknown-linux-gnu
          -DDUMP_FLAG=--dump-bir
          "-DEXTRA_ARGS=--mir-focus-function;stdarg;--mir-focus-block;entry"
          -DSRC=${PROJECT_SOURCE_DIR}/tests/c/external/c-testsuite/src/00204.c
          "-DREQUIRED_SNIPPETS=focus function: stdarg|focus block: entry|focused functions matched: 1|focused bir blocks matched: 1|bir.func @stdarg() -> void {|entry:|bir.call i32 printf(ptr @.str48)|bir.call void myprintf(ptr @.str49"
          "-DFORBIDDEN_SNIPPETS=if.then:|if.end:|bir.func @myprintf|bir.func @addlm123"
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_dump_flag_case.cmake"
)
set_tests_properties(backend_cli_dump_bir_focus_block_entry_00204 PROPERTIES
  LABELS "backend;internal;cli"
)

add_test(
  NAME backend_cli_dump_bir_focus_block_missing_00204
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DTARGET_TRIPLE=x86_64-unknown-linux-gnu
          -DDUMP_FLAG=--dump-bir
          "-DEXTRA_ARGS=--mir-focus-function;stdarg;--mir-focus-block;missing.block"
          -DSRC=${PROJECT_SOURCE_DIR}/tests/c/external/c-testsuite/src/00204.c
          "-DREQUIRED_SNIPPETS=focus function: stdarg|focus block: missing.block|focused functions matched: 1|focused bir blocks matched: 0|bir.func @stdarg() -> void {"
          "-DFORBIDDEN_SNIPPETS=entry:|if.then:|if.end:|bir.func @myprintf|bir.func @addlm123"
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_dump_flag_case.cmake"
)
set_tests_properties(backend_cli_dump_bir_focus_block_missing_00204 PROPERTIES
  LABELS "backend;internal;cli"
)

add_test(
  NAME backend_cli_dump_prepared_bir_focus_block_entry_00204
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DTARGET_TRIPLE=x86_64-unknown-linux-gnu
          -DDUMP_FLAG=--dump-prepared-bir
          "-DEXTRA_ARGS=--mir-focus-function;stdarg;--mir-focus-block;entry"
          -DSRC=${PROJECT_SOURCE_DIR}/tests/c/external/c-testsuite/src/00204.c
          "-DREQUIRED_SNIPPETS=focus function: stdarg|focus block: entry|focused functions matched: 1|focused bir blocks matched: 1|focused prepared blocks matched: 1|prepared.module target=x86_64-unknown-linux-gnu route=semantic_bir_shared|prepared.func @stdarg|  block entry terminator=return|access block=entry"
          "-DFORBIDDEN_SNIPPETS=  block if.then|  block if.end|access block=if.then|access block=if.end"
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_dump_flag_case.cmake"
)
set_tests_properties(backend_cli_dump_prepared_bir_focus_block_entry_00204 PROPERTIES
  LABELS "backend;internal;cli"
)

add_test(
  NAME backend_cli_dump_prepared_bir_focus_block_missing_00204
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DTARGET_TRIPLE=x86_64-unknown-linux-gnu
          -DDUMP_FLAG=--dump-prepared-bir
          "-DEXTRA_ARGS=--mir-focus-function;stdarg;--mir-focus-block;missing.block"
          -DSRC=${PROJECT_SOURCE_DIR}/tests/c/external/c-testsuite/src/00204.c
          "-DREQUIRED_SNIPPETS=focus function: stdarg|focus block: missing.block|focused functions matched: 1|focused bir blocks matched: 0|focused prepared blocks matched: 0|prepared.module target=x86_64-unknown-linux-gnu route=semantic_bir_shared|prepared.func @stdarg"
          "-DFORBIDDEN_SNIPPETS=entry:|  block entry|access block=entry"
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_dump_flag_case.cmake"
)
set_tests_properties(backend_cli_dump_prepared_bir_focus_block_missing_00204 PROPERTIES
  LABELS "backend;internal;cli"
)

add_test(
  NAME backend_cli_dump_bir_focus_value_t1_00204
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DTARGET_TRIPLE=x86_64-unknown-linux-gnu
          -DDUMP_FLAG=--dump-bir
          "-DEXTRA_ARGS=--mir-focus-function;stdarg;--mir-focus-value;t1"
          -DSRC=${PROJECT_SOURCE_DIR}/tests/c/external/c-testsuite/src/00204.c
          "-DREQUIRED_SNIPPETS=focus function: stdarg|focus value: %t1|focused functions matched: 1|focused semantic value-owned lines matched: 1|bir.func @stdarg() -> void {|entry:|%t1 = bir.call i32 printf(ptr @.str48)|bir.ret"
          "-DFORBIDDEN_SNIPPETS=bir.call void myprintf(ptr @.str49|%t3.global.aggregate.load.0|bir.func @myprintf"
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_dump_flag_case.cmake"
)
set_tests_properties(backend_cli_dump_bir_focus_value_t1_00204 PROPERTIES
  LABELS "backend;internal;cli"
)

add_test(
  NAME backend_cli_dump_bir_focus_value_missing_00204
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DTARGET_TRIPLE=x86_64-unknown-linux-gnu
          -DDUMP_FLAG=--dump-bir
          "-DEXTRA_ARGS=--mir-focus-function;stdarg;--mir-focus-value;missing"
          -DSRC=${PROJECT_SOURCE_DIR}/tests/c/external/c-testsuite/src/00204.c
          "-DREQUIRED_SNIPPETS=focus function: stdarg|focus value: %missing|focused functions matched: 1|focused semantic value-owned lines matched: 0|bir.func @stdarg() -> void {|}"
          "-DFORBIDDEN_SNIPPETS=entry:|%t1 = bir.call i32 printf(ptr @.str48)|bir.call void myprintf(ptr @.str49|bir.func @myprintf"
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_dump_flag_case.cmake"
)
set_tests_properties(backend_cli_dump_bir_focus_value_missing_00204 PROPERTIES
  LABELS "backend;internal;cli"
)

add_test(
  NAME backend_cli_dump_prepared_bir_focus_value_t1_00204
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DTARGET_TRIPLE=x86_64-unknown-linux-gnu
          -DDUMP_FLAG=--dump-prepared-bir
          "-DEXTRA_ARGS=--mir-focus-function;stdarg;--mir-focus-value;t1"
          -DSRC=${PROJECT_SOURCE_DIR}/tests/c/external/c-testsuite/src/00204.c
          "-DREQUIRED_SNIPPETS=focus function: stdarg|focus value: %t1|focused functions matched: 1|focused semantic value-owned lines matched: 1|prepared.module target=x86_64-unknown-linux-gnu route=semantic_bir_shared|bir.func @stdarg() -> void {|entry:|%t1 = bir.call i32 printf(ptr @.str48)|prepared.summary @stdarg|calls=1|storage_values=1|callsite block=0 inst=0 wrapper=direct_extern_variadic|storage %t1 register bank=gpr placement=gpr:caller_saved#0/w1 reg=r11|home %t1 value_id=|move_bundle phase=after_call authority=none block_index=0 instruction_index=0|move from_value_id="
          "-DFORBIDDEN_SNIPPETS=bir.call void myprintf(ptr @.str49|callsite block=0 inst=129|storage %t3.global.aggregate.load.0|access block=entry"
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_dump_flag_case.cmake"
)
set_tests_properties(backend_cli_dump_prepared_bir_focus_value_t1_00204 PROPERTIES
  LABELS "backend;internal;cli"
)

add_test(
  NAME backend_cli_dump_prepared_bir_focus_value_missing_00204
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DTARGET_TRIPLE=x86_64-unknown-linux-gnu
          -DDUMP_FLAG=--dump-prepared-bir
          "-DEXTRA_ARGS=--mir-focus-function;stdarg;--mir-focus-value;missing"
          -DSRC=${PROJECT_SOURCE_DIR}/tests/c/external/c-testsuite/src/00204.c
          "-DREQUIRED_SNIPPETS=focus function: stdarg|focus value: %missing|focused functions matched: 1|focused semantic value-owned lines matched: 0|focused prepared value-owned lines matched: 0|prepared.module target=x86_64-unknown-linux-gnu route=semantic_bir_shared|bir.func @stdarg() -> void {|prepared.func @stdarg"
          "-DFORBIDDEN_SNIPPETS=entry:|%t1 = bir.call i32 printf(ptr @.str48)|home %t1 value_id=|move_bundle phase=after_call|access block=entry"
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_dump_flag_case.cmake"
)
set_tests_properties(backend_cli_dump_prepared_bir_focus_value_missing_00204 PROPERTIES
  LABELS "backend;internal;cli"
)

if(C4C_ENABLE_X86_BACKEND_TESTS)
  add_executable(backend_x86_route_debug_test
      "${BACKEND_TEST_ROOT}/bir/backend_x86_route_debug_test.cpp"
  )
  target_include_directories(backend_x86_route_debug_test PRIVATE
      "${PROJECT_SOURCE_DIR}"
  )
  target_link_libraries(backend_x86_route_debug_test PRIVATE
      c4c_backend
  )
  set_target_properties(backend_x86_route_debug_test PROPERTIES
      CXX_STANDARD 17
      CXX_STANDARD_REQUIRED ON
  )

  add_test(
    NAME backend_x86_route_debug
    COMMAND $<TARGET_FILE:backend_x86_route_debug_test>
  )
  set_tests_properties(backend_x86_route_debug PROPERTIES
    LABELS "backend;internal;cpp"
  )
endif()

if(C4C_ENABLE_X86_BACKEND_TESTS)
  add_executable(backend_x86_handoff_boundary_test
      "${BACKEND_TEST_ROOT}/bir/backend_x86_handoff_boundary_test.cpp"
      "${BACKEND_TEST_ROOT}/bir/backend_x86_handoff_boundary_compare_branch_test.cpp"
      "${BACKEND_TEST_ROOT}/bir/backend_x86_handoff_boundary_direct_extern_call_test.cpp"
      "${BACKEND_TEST_ROOT}/bir/backend_x86_handoff_boundary_i32_guard_chain_test.cpp"
      "${BACKEND_TEST_ROOT}/bir/backend_x86_handoff_boundary_joined_branch_test.cpp"
      "${BACKEND_TEST_ROOT}/bir/backend_x86_handoff_boundary_lir_test.cpp"
      "${BACKEND_TEST_ROOT}/bir/backend_x86_handoff_boundary_local_i16_guard_test.cpp"
      "${BACKEND_TEST_ROOT}/bir/backend_x86_handoff_boundary_local_i32_guard_test.cpp"
      "${BACKEND_TEST_ROOT}/bir/backend_x86_handoff_boundary_local_slot_guard_lane_test.cpp"
      "${BACKEND_TEST_ROOT}/bir/backend_x86_handoff_boundary_multi_defined_call_test.cpp"
      "${BACKEND_TEST_ROOT}/bir/backend_x86_handoff_boundary_multi_defined_rejection_test.cpp"
      "${BACKEND_TEST_ROOT}/bir/backend_x86_handoff_boundary_scalar_smoke_test.cpp"
      "${BACKEND_TEST_ROOT}/bir/backend_x86_handoff_boundary_short_circuit_test.cpp"
      "${BACKEND_TEST_ROOT}/bir/backend_x86_handoff_boundary_loop_countdown_test.cpp"
  )
  target_include_directories(backend_x86_handoff_boundary_test PRIVATE
      "${PROJECT_SOURCE_DIR}"
  )
  target_link_libraries(backend_x86_handoff_boundary_test PRIVATE
      c4c_backend
  )
  set_target_properties(backend_x86_handoff_boundary_test PROPERTIES
      CXX_STANDARD 17
      CXX_STANDARD_REQUIRED ON
  )

  add_test(
    NAME backend_x86_handoff_boundary
    COMMAND $<TARGET_FILE:backend_x86_handoff_boundary_test>
  )
  set_tests_properties(backend_x86_handoff_boundary PROPERTIES
    LABELS "backend;internal;cpp"
  )
endif()

c4c_add_backend_codegen_route_test(
  backend_codegen_route_riscv64_two_param_select_eq_split_predecessor_deeper_then_mixed_affine_post_add_sub_observes_semantic_phi
  SRC "${BACKEND_CASE_ROOT}/three_way_phi_merge_post_add_sub.c"
  TARGET_TRIPLE riscv64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/three_way_phi_merge_post_add_sub_semantic_phi_riscv64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @choose3_phi_post_chain(i32 %p.x, i32 %p.y, i32 %p.z) -> i32 {|%t19 = bir.phi i32 [tern.then.end.13, %t17] [tern.else.end.15, %t18]|%t20 = bir.phi i32 [tern.then.end.4, %t8] [tern.else.end.6, %t19]|%t21 = bir.add i32 %t20, 6|%t22 = bir.sub i32 %t21, 2|bir.ret i32 %t22"
  FORBIDDEN_SNIPPETS "define i32 @choose3_phi_post_chain(i32 %p.x, i32 %p.y, i32 %p.z)|bir.store_local|semantic_phi|bir.select"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_riscv64_two_param_select_eq_split_predecessor_deeper_then_mixed_affine_post_add_sub_observes_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/three_way_phi_merge_post_add_sub.c"
  TARGET_TRIPLE riscv64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/three_way_phi_merge_post_add_sub_semantic_bir_riscv64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @choose3_phi_post_chain(i32 %p.x, i32 %p.y, i32 %p.z) -> i32 {|bir.phi i32|bir.ret i32"
  FORBIDDEN_SNIPPETS "define i32 @choose3_phi_post_chain(i32 %p.x, i32 %p.y, i32 %p.z)|bir.store_local|semantic_phi|bir.select"
)

add_test(
  NAME backend_codegen_route_x86_64_byval_member_array_params_observe_semantic_bir
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DTARGET_TRIPLE=x86_64-unknown-linux-gnu
          -DSRC_A=${BACKEND_CASE_ROOT}/param_member_array.c
          -DOUT_A=${CMAKE_BINARY_DIR}/backend_route/param_member_array_semantic_bir_x86_64.ll
          "-DREQUIRED_SNIPPETS_A=bir.func @get_second(ptr byval(size=8, align=4) %p.p) -> i32 {|bir.call i32 get_second(ptr byval(size=8, align=4) %lv.p)|bir.ret i32 %t3"
          -DSRC_B=${BACKEND_CASE_ROOT}/nested_param_member_array.c
          -DOUT_B=${CMAKE_BINARY_DIR}/backend_route/nested_param_member_array_semantic_bir_x86_64.ll
          "-DREQUIRED_SNIPPETS_B=bir.func @get_second(ptr byval(size=8, align=4) %p.o) -> i32 {|bir.call i32 get_second(ptr byval(size=8, align=4) %lv.outer)|bir.ret i32 %t4"
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_dual_codegen_route_case.cmake"
)
c4c_set_backend_test_labels(
  "backend_codegen_route_x86_64_byval_member_array_params_observe_semantic_bir"
  backend_route
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_aggregate_return_pair_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/aggregate_return_pair.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/aggregate_return_pair_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @make_pair(ptr sret(size=8, align=4) %ret.sret) -> void {|bir.store_local %lv.p.0, i32 3|bir.store_local %lv.p.0, i32 make_pair.ret.sret.copy.0, addr %ret.sret|bir.call void make_pair(ptr sret(size=8, align=4) %t0)|bir.ret i32 %t2"
  FORBIDDEN_SNIPPETS "define %struct.Pair @make_pair()|call %struct.Pair () @make_pair()"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_aggregate_param_return_pair_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/aggregate_param_return_pair.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/aggregate_param_return_pair_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @id_pair(ptr sret(size=8, align=4) %ret.sret, ptr byval(size=8, align=4) %p.p) -> void {|%lv.param.p.p.aggregate.param.copy.0 = bir.load_local i32 %lv.param.p.p.0, addr %p.p|bir.store_local %lv.param.p.p.0, i32 id_pair.ret.sret.copy.0, addr %ret.sret|bir.call void id_pair(ptr sret(size=8, align=4) %t3, ptr byval(size=8, align=4) %lv.p)|bir.ret i32 %t5"
  FORBIDDEN_SNIPPETS "define %struct.Pair @id_pair(%struct.Pair %p.p)|call %struct.Pair (%struct.Pair) @id_pair(%struct.Pair %t2)"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_indirect_aggregate_param_return_pair_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/indirect_aggregate_param_return_pair.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/indirect_aggregate_param_return_pair_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @id_pair(ptr sret(size=8, align=4) %ret.sret, ptr byval(size=8, align=4) %p.p) -> void {|bir.store_local %lv.p.0, i32 4|bir.call void @id_pair(ptr sret(size=8, align=4) %t4, ptr byval(size=8, align=4) %lv.p)|bir.ret i32 %t6"
  FORBIDDEN_SNIPPETS "define %struct.Pair @id_pair(%struct.Pair %p.p)|call %struct.Pair (%struct.Pair) %t2(%struct.Pair %t3)"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_function_pointer_param_direct_arg_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/function_pointer_param_direct_arg.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/function_pointer_param_direct_arg_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @apply(ptr %p.fn, i32 %p.x) -> i32 {|%t0 = bir.call i32 %p.fn(i32 %p.x)|bir.call i32 apply(ptr @inc, i32 4)|bir.ret i32 %t0"
  FORBIDDEN_SNIPPETS "define i32 @main()|call i32 (ptr, i32) @apply(ptr @inc, i32 4)"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_aggregate_member_function_pointer_call_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/aggregate_member_function_pointer_call.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/aggregate_member_function_pointer_call_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @apply(ptr %p.h, i32 %p.x) -> i32 {|%t1 = bir.load_local ptr %t1.addr, addr %p.h|%t2 = bir.call i32 %t1(i32 %p.x)|bir.store_local %lv.h.0, ptr @inc|bir.call i32 apply(ptr %lv.h, i32 4)|bir.ret i32 %t1"
  FORBIDDEN_SNIPPETS "call i32 (ptr, i32) @apply("
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_global_aggregate_function_pointer_call_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/global_aggregate_function_pointer_call.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/global_aggregate_function_pointer_call_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|%t1 = bir.load_global ptr @g|%t2 = bir.call i32 %t1(i32 4)|bir.ret i32 %t2"
  FORBIDDEN_SNIPPETS "call i32 (ptr, i32) @g.fn|addr @inc+"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_global_aggregate_array_function_pointer_call_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/global_aggregate_array_function_pointer_call.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/global_aggregate_array_function_pointer_call_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|%t3 = bir.load_global ptr @gs, offset 8|%t4 = bir.call i32 %t3(i32 4)|bir.ret i32 %t4"
  FORBIDDEN_SNIPPETS "call i32 (ptr, i32) %t|addr @inc+8"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_global_nested_aggregate_function_pointer_call_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/global_nested_aggregate_function_pointer_call.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/global_nested_aggregate_function_pointer_call_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|%t2 = bir.load_global ptr @g, offset 8|%t3 = bir.call i32 %t2(i32 4)|bir.ret i32 %t3"
  FORBIDDEN_SNIPPETS "call i32 (ptr, i32) @g.inner.fn|addr @inc+8"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_global_nested_aggregate_array_function_pointer_call_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/global_nested_aggregate_array_function_pointer_call.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/global_nested_aggregate_array_function_pointer_call_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|%t4 = bir.load_global ptr @g, offset 16|%t5 = bir.call i32 %t4(i32 4)|bir.ret i32 %t5"
  FORBIDDEN_SNIPPETS "call i32 (ptr, i32) @g.inner.1.fn|addr @inc+16"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_global_function_pointer_value_call_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/global_function_pointer_value_call.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/global_function_pointer_value_call_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|%t0 = bir.load_global ptr @gp|%t1 = bir.load_local ptr %lv.p|%t2 = bir.call i32 %t1(i32 4)|bir.ret i32 %t2"
  FORBIDDEN_SNIPPETS "define i32 @main()|call i32 (i32)"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_defined_global_struct_store_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/defined_global_struct_store.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/defined_global_struct_store_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|bir.store_global @v, i32 1|bir.store_global @v, offset 4, i32 2|%t3 = bir.load_global i32 @v|%t6 = bir.load_global i32 @v, offset 4|bir.ret i32 %t7"
  FORBIDDEN_SNIPPETS "define i32 @main()"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_global_struct_array_read_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/global_struct_array_read.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/global_struct_array_read_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|%t3 = bir.load_global i32 @pairs, offset 12|bir.ret i32 %t3"
  FORBIDDEN_SNIPPETS "define i32 @main()"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_global_struct_array_store_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/global_struct_array_store.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/global_struct_array_store_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|bir.store_global @pairs, offset 8, i32 9|%t6 = bir.load_global i32 @pairs, offset 8|bir.ret i32 %t6"
  FORBIDDEN_SNIPPETS "define i32 @main()"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_nested_global_struct_array_read_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/nested_global_struct_array_read.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/nested_global_struct_array_read_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|%t4 = bir.load_global i32 @s, offset 8|bir.ret i32 %t4"
  FORBIDDEN_SNIPPETS "define i32 @main()"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_nested_global_struct_array_store_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/nested_global_struct_array_store.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/nested_global_struct_array_store_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|bir.store_global @s, offset 4, i32 9|%t8 = bir.load_global i32 @s, offset 4|bir.ret i32 %t8"
  FORBIDDEN_SNIPPETS "define i32 @main()"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_local_aggregate_member_offsets_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/local_aggregate_member_offsets.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/local_aggregate_member_offsets_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|bir.store_local %lv.p.0, i32 1|bir.store_local %lv.p.4, i32 2|bir.store_local %lv.p.4, i32 5|%t4 = bir.load_local i32 %lv.p.0|%t6 = bir.load_local i32 %lv.p.4|bir.ret i32 %t7"
  FORBIDDEN_SNIPPETS "define i32 @main()|getelementptr %struct.Pair, ptr"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_local_aggregate_bitfield_layout_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/local_aggregate_bitfield_layout.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/local_aggregate_bitfield_layout_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @read_code(ptr %p.node) -> i32 {|addr %p.node+16|bir.func @main() -> i32 {|bir.store_local %lv.node.0, ptr 0|bir.store_local %lv.node.8, ptr 0|bir.load_local i32 %lv.node.16|bir.store_local %lv.node.16, i32 %t4.bf.comb|bir.call i32 read_code(ptr %lv.node)"
  FORBIDDEN_SNIPPETS "define i32 @main()|addr %p.node+2|%lv.node.2,|getelementptr %struct.TreeNode|getelementptr %struct.NodeCommon"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_local_arg_call_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/local_arg_call.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/local_arg_call_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @add_one(i32 %p.x) -> i32 {|%t0 = bir.add i32 %p.x, 1|bir.func @main() -> i32 {|bir.store_local %lv.x, i32 5|%t0 = bir.load_local i32 %lv.x|%t1 = bir.call i32 add_one(i32 %t0)|bir.ret i32 %t1"
  FORBIDDEN_SNIPPETS "define i32 @main()|call i32 (i32) @add_one(i32 %t0)"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_call_helper_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/call_helper.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/call_helper_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @helper() -> i32|bir.func @main() -> i32 {|%t0 = bir.call i32 helper()|bir.ret i32 %t0"
  FORBIDDEN_SNIPPETS "define i32 @main()|call i32 @helper()"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_pointer_return_direct_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/pointer_return_direct.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/pointer_return_direct_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @id_ptr(ptr %p.p) -> ptr {|bir.func @forward(ptr %p.p) -> ptr {|%t0 = bir.call ptr id_ptr(ptr %p.p)|bir.ret ptr %t0"
  FORBIDDEN_SNIPPETS "define ptr @forward(ptr %p.p)|call ptr (ptr) @id_ptr(ptr %p.p)"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_local_pointer_deref_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/local_pointer_deref.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/local_pointer_deref_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|bir.store_local %lv.x, i32 7|bir.store_local %lv.p, ptr %lv.x|%t0 = bir.load_local ptr %lv.p|%t1 = bir.load_local i32 %lv.x|bir.ret i32 %t1"
  FORBIDDEN_SNIPPETS "define i32 @main()|store i32 7, ptr %lv.x|%t1 = load i32, ptr %t0"
)

add_test(
  NAME backend_codegen_route_x86_64_builtin_abs_observe_semantic_bir
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DTARGET_TRIPLE=x86_64-unknown-linux-gnu
          -DSRC_A=${BACKEND_CASE_ROOT}/builtin_abs_i32.c
          -DOUT_A=${CMAKE_BINARY_DIR}/backend_route/builtin_abs_i32_semantic_bir_x86_64.ll
          "-DREQUIRED_SNIPPETS_A=bir.func @take_abs_int(i32 %p.x) -> i32 {|%t0.neg = bir.sub i32 0, %p.x|%t0 = bir.select slt i32 %p.x, 0, i32 %t0.neg, %p.x|bir.ret i32 %t0"
          "-DFORBIDDEN_SNIPPETS_A=@llvm.abs.i32|bir.call i32 @llvm.abs.i32"
          -DSRC_B=${BACKEND_CASE_ROOT}/builtin_abs_i64.c
          -DOUT_B=${CMAKE_BINARY_DIR}/backend_route/builtin_abs_i64_semantic_bir_x86_64.ll
          "-DREQUIRED_SNIPPETS_B=bir.func @take_abs_long(i64 %p.x) -> i64 {|%t0.neg = bir.sub i64 0, %p.x|%t0 = bir.select slt i64 %p.x, 0, i64 %t0.neg, %p.x|bir.ret i64 %t0"
          "-DFORBIDDEN_SNIPPETS_B=@llvm.abs.i64|bir.call i64 @llvm.abs.i64"
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_dual_codegen_route_case.cmake"
)
c4c_set_backend_test_labels(
  "backend_codegen_route_x86_64_builtin_abs_observe_semantic_bir"
  backend_route
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_builtin_fabs_f64_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/builtin_fabs_f64.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/builtin_fabs_f64_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @take_fabs(double %p.x) -> double {|%t0 = bir.call double llvm.fabs.double(double %p.x)|bir.ret double %t0"
  FORBIDDEN_SNIPPETS "define double @take_fabs|call double @llvm.fabs.double"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_builtin_fabs_f32_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/builtin_fabs_f32.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/builtin_fabs_f32_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @take_fabsf(float %p.x) -> float {|%t0 = bir.call float llvm.fabs.float(float %p.x)|bir.ret float %t0"
  FORBIDDEN_SNIPPETS "define float @take_fabsf|call float @llvm.fabs.float"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_builtin_fabsl_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/builtin_fabsl.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/builtin_fabsl_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @take_fabsl(f128 %p.x) -> f128 {|%t0 = bir.call f128 llvm.fabs.x86_fp80(f128 %p.x)|bir.ret f128 %t0"
  FORBIDDEN_SNIPPETS "define x86_fp80 @take_fabsl|call x86_fp80 @llvm.fabs.x86_fp80"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_variadic_sum2_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/variadic_sum2.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/variadic_sum2_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @sum2(i32 %p.first) -> i32 {|bir.call void llvm.va_start.p0(ptr %lv.ap)|%t0 = bir.call i32 llvm.va_arg.i32(ptr %lv.ap)|bir.call void llvm.va_end.p0(ptr %lv.ap)|bir.ret i32 %t2|bir.func @main() -> i32 {|bir.call i32 sum2(i32 10, i32 32)|bir.ret i32 %t0"
  FORBIDDEN_SNIPPETS "define i32 @sum2|call void @llvm.va_start.p0|call void @llvm.va_end.p0|call void @llvm.memcpy.p0.p0.i64"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/variadic_double_bytes.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/variadic_double_bytes_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @variadic_double_bytes(i32 %p.seed) -> i32 {|bir.call void llvm.va_start.p0(ptr %lv.ap)|%t0 = bir.call double llvm.va_arg.double(ptr %lv.ap)|bir.store_local %lv.second, double %t0|bir.call void llvm.va_end.p0(ptr %lv.ap)|bir.store_local %lv.bytes, ptr %lv.second|%t1 = bir.load_local ptr %lv.bytes|%t4 = bir.load_local i8 %t4.addr, addr %lv.second+6|%t7 = bir.load_local ptr %lv.bytes|%t10 = bir.load_local i8 %t10.addr, addr %lv.second+7|bir.ret i32 %t12|bir.func @main() -> i32 {|bir.call i32 variadic_double_bytes(i32 1, double 0x4002000000000000)|bir.ret i32 %t0"
  FORBIDDEN_SNIPPETS "define i32 @variadic_double_bytes|call void @llvm.va_start.p0|call void @llvm.va_end.p0|va_arg ptr %lv.ap, double"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/variadic_pair_second.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/variadic_pair_second_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @variadic_pair_second(i32 %p.seed) -> i32 {|bir.call void llvm.va_start.p0(ptr %lv.ap)|bir.call void llvm.va_arg.aggregate(ptr sret(size=8, align=4) %t0, ptr %lv.ap)|%lv.second.aggregate.copy.0 = bir.load_local i32 %t0.0|bir.store_local %lv.second.0, i32 %lv.second.aggregate.copy.0|%lv.second.aggregate.copy.4 = bir.load_local i32 %t0.4|bir.store_local %lv.second.4, i32 %lv.second.aggregate.copy.4|bir.call void llvm.va_end.p0(ptr %lv.ap)|%t2 = bir.load_local i32 %lv.second.4|bir.ret i32 %t3|bir.func @main() -> i32 {|%t4.memcpy.copy.0 = bir.load_local i32 %lv.pair.0|bir.store_local %lv.pair.0, i32 %t4.memcpy.copy.0, addr %t4|%t4.memcpy.copy.4 = bir.load_local i32 %lv.pair.4|bir.store_local %lv.pair.4, i32 %t4.memcpy.copy.4, addr %t4+4|bir.call i32 variadic_pair_second(i32 1, i64 %t5)|bir.ret i32 %t6"
  FORBIDDEN_SNIPPETS "define i32 @variadic_pair_second|call void @llvm.va_start.p0|call void @llvm.va_end.p0|va_arg ptr %lv.ap, %struct.Pair|call void @llvm.memcpy.p0.p0.i64|call void @llvm.memset.p0.i64|bir.call ptr memcpy|bir.call ptr memset"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_variadic_va_copy_accumulate_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/variadic_va_copy_accumulate.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/variadic_va_copy_accumulate_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @variadic_va_copy_accumulate(i32 %p.seed) -> i64 {|bir.call void llvm.va_start.p0(ptr %lv.ap)|bir.call void llvm.va_copy.p0.p0(ptr %lv.copy, ptr %lv.ap)|%t0 = bir.call i64 llvm.va_arg.i64(ptr %lv.ap)|%t1 = bir.call i64 llvm.va_arg.i64(ptr %lv.copy)|bir.call void llvm.va_end.p0(ptr %lv.copy)|bir.call void llvm.va_end.p0(ptr %lv.ap)|bir.ret i64 %t6|bir.func @main() -> i32 {|%t0 = bir.call i64 variadic_va_copy_accumulate(i32 1, i64 10)|bir.ret i32 %t2"
  FORBIDDEN_SNIPPETS "define i64 @variadic_va_copy_accumulate|call void @llvm.va_copy.p0.p0|call void @llvm.va_start.p0|call void @llvm.va_end.p0|va_arg ptr %lv.ap, i64|va_arg ptr %lv.copy, i64|bir.call ptr memcpy|bir.call ptr memset"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_indirect_variadic_sum2_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/indirect_variadic_sum2.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/indirect_variadic_sum2_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @sum2(i32 %p.first) -> i32 {|bir.call void llvm.va_start.p0(ptr %lv.ap)|%t0 = bir.call i32 llvm.va_arg.i32(ptr %lv.ap)|bir.call void llvm.va_end.p0(ptr %lv.ap)|bir.ret i32 %t2|bir.func @apply(ptr %p.fn) -> i32 {|%t0 = bir.call i32 %p.fn(i32 10, i32 32)|bir.ret i32 %t0|bir.func @main() -> i32 {|bir.call i32 apply(ptr @sum2)|bir.ret i32 %t0"
  FORBIDDEN_SNIPPETS "define i32 @sum2|define i32 @apply|define i32 @main()|call void @llvm.va_start.p0|call void @llvm.va_end.p0|va_arg ptr %lv.ap, i32|call i32 (i32, ...) %p.fn(i32 10, i32 32)"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_indirect_variadic_pair_second_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/indirect_variadic_pair_second.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/indirect_variadic_pair_second_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @variadic_pair_second(i32 %p.seed) -> i32 {|bir.call void llvm.va_start.p0(ptr %lv.ap)|bir.call void llvm.va_arg.aggregate(ptr sret(size=8, align=4) %t0, ptr %lv.ap)|%lv.second.aggregate.copy.0 = bir.load_local i32 %t0.0|bir.store_local %lv.second.0, i32 %lv.second.aggregate.copy.0|%lv.second.aggregate.copy.4 = bir.load_local i32 %t0.4|bir.store_local %lv.second.4, i32 %lv.second.aggregate.copy.4|bir.call void llvm.va_end.p0(ptr %lv.ap)|bir.ret i32 %t3|bir.func @apply(ptr %p.fn, ptr byval(size=8, align=4) %p.pair) -> i32 {|%t0 = bir.call i32 %p.fn(i32 1, ptr byval(size=8, align=4) %lv.param.p.pair)|bir.ret i32 %t0|bir.func @main() -> i32 {|%t3 = bir.call i32 apply(ptr @variadic_pair_second, ptr byval(size=8, align=4) %lv.pair)|bir.ret i32 %t3"
  FORBIDDEN_SNIPPETS "define i32 @variadic_pair_second|define i32 @apply|define i32 @main()|call void @llvm.va_start.p0|call void @llvm.va_end.p0|va_arg ptr %lv.ap, %struct.Pair|call i32 (i32, ...) %p.fn(i32 1, i64 %t3)"
)

if(CLANG_EXECUTABLE)
  c4c_add_backend_runtime_case(
    backend_runtime_byval_helper_payload_8_to_13
    SRC "${BACKEND_CASE_ROOT}/byval_helper_payload_8_to_13.c"
    LABELS x86 byval abi
  )

  c4c_add_backend_runtime_case(
    backend_runtime_byval_helper_payload_9_to_14
    SRC "${BACKEND_CASE_ROOT}/byval_helper_payload_9_to_14.c"
    LABELS x86 byval abi
  )

  c4c_add_backend_runtime_case(
    backend_runtime_byval_helper_mixed_hfa_payload
    SRC "${BACKEND_CASE_ROOT}/byval_helper_mixed_hfa_payload.c"
    LABELS x86 byval abi hfa
  )
endif()

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_vla_goto_stackrestore_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/vla_goto_stackrestore.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/vla_goto_stackrestore_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @accumulate_vla(i32 %p.n) -> i32 {|%t0 = bir.call ptr llvm.stacksave()|%t3 = bir.call ptr llvm.dynamic_alloca.i32(i64 %t2)|bir.call void llvm.stackrestore(ptr %t0)|bir.ret i32|bir.func @main() -> i32 {|bir.call i32 accumulate_vla(i32 3)|bir.ret i32 %t0"
  FORBIDDEN_SNIPPETS "define i32 @accumulate_vla|call ptr @llvm.stacksave()|call void @llvm.stackrestore(ptr %t0)|alloca i32, i64 %t2"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_inline_asm_nop_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/inline_asm_nop.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/inline_asm_nop_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|bir.call void llvm.inline_asm() [asm=\"nop\", constraints=\"\", sideeffect]|bir.ret i32 0"
  FORBIDDEN_SNIPPETS "define i32 @main()|call void asm sideeffect \"nop\", \"\"()"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_inline_asm_input_i32_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/inline_asm_input_i32.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/inline_asm_input_i32_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|bir.store_local %lv.x, i32 7|%t0 = bir.load_local i32 %lv.x|bir.call void llvm.inline_asm(i32 %t0) [asm=\"\", constraints=\"r\", sideeffect]|%t1 = bir.load_local i32 %lv.x|bir.ret i32 %t1"
  FORBIDDEN_SNIPPETS "define i32 @main()|call void asm sideeffect \"\", \"r\"(i32 %t0)|raw_args="
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_inline_asm_input_two_i32_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/inline_asm_input_two_i32.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/inline_asm_input_two_i32_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|bir.store_local %lv.x, i32 7|bir.store_local %lv.y, i32 9|%t0 = bir.load_local i32 %lv.x|%t1 = bir.load_local i32 %lv.y|bir.call void llvm.inline_asm(i32 %t0, i32 %t1) [asm=\"\", constraints=\"r,r\", sideeffect]|%t2 = bir.load_local i32 %lv.x|%t3 = bir.load_local i32 %lv.y|%t4 = bir.add i32 %t2, %t3|bir.ret i32 %t4"
  FORBIDDEN_SNIPPETS "define i32 @main()|call void asm sideeffect \"\", \"r,r\"(i32 %t0, i32 %t1)|raw_args="
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_inline_asm_input_ptr_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/inline_asm_input_ptr.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/inline_asm_input_ptr_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @touch(ptr %p.p) -> i32 {|bir.call void llvm.inline_asm(ptr %p.p) [asm=\"\", constraints=\"r\", sideeffect]|bir.ret i32 0|bir.func @main() -> i32 {|bir.store_local %lv.x, i32 7|%t0 = bir.call i32 touch(ptr %lv.x)|bir.ret i32 %t0"
  FORBIDDEN_SNIPPETS "define i32 @touch|call void asm sideeffect \"\", \"r\"(ptr %p.p)|raw_args="
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_inline_asm_input_local_ptr_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/inline_asm_input_local_ptr.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/inline_asm_input_local_ptr_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|bir.store_local %lv.x, i32 7|bir.store_local %lv.p, ptr %lv.x|%t0 = bir.load_local ptr %lv.p|bir.call void llvm.inline_asm(ptr %t0) [asm=\"\", constraints=\"r\", sideeffect]|%t1 = bir.load_local ptr %lv.p|%t2 = bir.load_local i32 %lv.x|bir.ret i32 %t2"
  FORBIDDEN_SNIPPETS "define i32 @main()|call void asm sideeffect \"\", \"r\"(ptr %t0)|raw_args=|load i32, ptr %t1"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_inline_asm_output_i32_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/inline_asm_output_i32.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/inline_asm_output_i32_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|bir.store_local %lv.x, i32 7|bir.store_local %lv.y, i32 0|%t0 = bir.load_local i32 %lv.x|%t1 = bir.call i32 llvm.inline_asm(i32 %t0) [asm=\"\", constraints=\"=r,r\", sideeffect]|bir.store_local %lv.y, i32 %t1|%t2 = bir.load_local i32 %lv.y|bir.ret i32 %t2"
  FORBIDDEN_SNIPPETS "define i32 @main()|call i32 asm sideeffect \"\", \"=r,r\"(i32 %t0)|raw_args=|bir.ret i32 0"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_inline_asm_output_readwrite_i32_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/inline_asm_output_readwrite_i32.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/inline_asm_output_readwrite_i32_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|bir.store_local %lv.x, i32 7|%t0 = bir.load_local i32 %lv.x|%t1 = bir.call i32 llvm.inline_asm(i32 %t0) [asm=\"\", constraints=\"=r,0\", sideeffect]|bir.store_local %lv.x, i32 %t1|%t2 = bir.load_local i32 %lv.x|bir.ret i32 %t2"
  FORBIDDEN_SNIPPETS "define i32 @main()|call i32 asm sideeffect \"\", \"+r\"(i32 %t0)|raw_args=|bir.call i32 llvm.inline_asm() [asm=\"\", constraints=\"=r,0\", sideeffect]"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_inline_asm_output_ptr_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/inline_asm_output_ptr.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/inline_asm_output_ptr_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|bir.store_local %lv.x, i32 7|bir.store_local %lv.p, ptr %lv.x|%t0 = bir.load_local ptr %lv.p|%t1 = bir.call ptr llvm.inline_asm(ptr %t0) [asm=\"\", constraints=\"=r,r\", sideeffect]|bir.store_local %lv.q, ptr %t1|bir.ret i32 0"
  FORBIDDEN_SNIPPETS "define i32 @main()|call ptr asm sideeffect \"\", \"=r,r\"(ptr %t0)|raw_args=|bir.call ptr llvm.inline_asm() [asm=\"\", constraints=\"=r,r\", sideeffect]"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_inline_asm_output_readwrite_ptr_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/inline_asm_output_readwrite_ptr.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/inline_asm_output_readwrite_ptr_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|bir.store_local %lv.x, i32 7|bir.store_local %lv.p, ptr %lv.x|%t0 = bir.load_local ptr %lv.p|%t1 = bir.call ptr llvm.inline_asm(ptr %t0) [asm=\"\", constraints=\"=r,0\", sideeffect]|bir.store_local %lv.p, ptr %t1|bir.ret i32 0"
  FORBIDDEN_SNIPPETS "define i32 @main()|call ptr asm sideeffect \"\", \"+r\"(ptr %t0)|raw_args=|bir.call ptr llvm.inline_asm() [asm=\"\", constraints=\"=r,0\", sideeffect]"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_builtin_memcpy_local_pair_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/builtin_memcpy_local_pair.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/builtin_memcpy_local_pair_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|%lv.dst.memcpy.copy.0 = bir.load_local i32 %lv.src.0|bir.store_local %lv.dst.0, i32 %lv.dst.memcpy.copy.0|%lv.dst.memcpy.copy.4 = bir.load_local i32 %lv.src.4|bir.store_local %lv.dst.4, i32 %lv.dst.memcpy.copy.4|bir.ret i32 %t7"
  FORBIDDEN_SNIPPETS "define i32 @main()|call ptr @memcpy|bir.call ptr memcpy"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_builtin_memcpy_nested_pair_field_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/builtin_memcpy_nested_pair_field.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/builtin_memcpy_nested_pair_field_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|%t10.memcpy.copy.0 = bir.load_local i32 %lv.src.4|bir.store_local %lv.dst.4, i32 %t10.memcpy.copy.0|%t10.memcpy.copy.4 = bir.load_local i32 %lv.src.8|bir.store_local %lv.dst.8, i32 %t10.memcpy.copy.4|bir.ret i32 %t22"
  FORBIDDEN_SNIPPETS "define i32 @main()|call ptr @memcpy|bir.call ptr memcpy"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_builtin_memcpy_local_i32_array_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/builtin_memcpy_local_i32_array.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/builtin_memcpy_local_i32_array_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|%t24.memcpy.copy.0 = bir.load_local i32 %lv.src.0|bir.store_local %lv.dst.0, i32 %t24.memcpy.copy.0|%t24.memcpy.copy.4 = bir.load_local i32 %lv.src.1|bir.store_local %lv.dst.1, i32 %t24.memcpy.copy.4|%t24.memcpy.copy.8 = bir.load_local i32 %lv.src.2|bir.store_local %lv.dst.2, i32 %t24.memcpy.copy.8|bir.ret i32 %t37"
  FORBIDDEN_SNIPPETS "define i32 @main()|call ptr @memcpy|bir.call ptr memcpy"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_builtin_memcpy_nested_i32_array_field_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/builtin_memcpy_nested_i32_array_field.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/builtin_memcpy_nested_i32_array_field_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|%t20.memcpy.copy.0 = bir.load_local i32 %lv.w.4|bir.store_local %lv.w.16, i32 %t20.memcpy.copy.0|%t20.memcpy.copy.4 = bir.load_local i32 %lv.w.8|bir.store_local %lv.w.20, i32 %t20.memcpy.copy.4|%t20.memcpy.copy.8 = bir.load_local i32 %lv.w.12|bir.store_local %lv.w.24, i32 %t20.memcpy.copy.8|bir.ret i32 %t40"
  FORBIDDEN_SNIPPETS "define i32 @main()|call ptr @memcpy|bir.call ptr memcpy"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_builtin_memcpy_local_i32_array_to_pair_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/builtin_memcpy_local_i32_array_to_pair.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/builtin_memcpy_local_i32_array_to_pair_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|%lv.dst.memcpy.copy.0 = bir.load_local i32 %lv.src.0|bir.store_local %lv.dst.0, i32 %lv.dst.memcpy.copy.0|%lv.dst.memcpy.copy.4 = bir.load_local i32 %lv.src.1|bir.store_local %lv.dst.4, i32 %lv.dst.memcpy.copy.4|bir.ret i32 %t16"
  FORBIDDEN_SNIPPETS "define i32 @main()|call ptr @memcpy|bir.call ptr memcpy"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_builtin_memcpy_local_pair_to_i32_array_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/builtin_memcpy_local_pair_to_i32_array.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/builtin_memcpy_local_pair_to_i32_array_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|%t10.memcpy.copy.0 = bir.load_local i32 %lv.src.0|bir.store_local %lv.dst.0, i32 %t10.memcpy.copy.0|%t10.memcpy.copy.4 = bir.load_local i32 %lv.src.4|bir.store_local %lv.dst.1, i32 %t10.memcpy.copy.4|bir.ret i32 %t18"
  FORBIDDEN_SNIPPETS "define i32 @main()|call ptr @memcpy|bir.call ptr memcpy"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_builtin_memcpy_prefix_local_i32_array_to_pair_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/builtin_memcpy_prefix_local_i32_array_to_pair.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/builtin_memcpy_prefix_local_i32_array_to_pair_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|%lv.dst.memcpy.copy.0 = bir.load_local i32 %lv.src.4|bir.store_local %lv.dst.0, i32 %lv.dst.memcpy.copy.0|%lv.dst.memcpy.copy.4 = bir.load_local i32 %lv.src.8|bir.store_local %lv.dst.4, i32 %lv.dst.memcpy.copy.4|bir.ret i32 %t27"
  FORBIDDEN_SNIPPETS "define i32 @main()|call ptr @memcpy|bir.call ptr memcpy"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_builtin_memcpy_prefix_local_pair_to_i32_array_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/builtin_memcpy_prefix_local_pair_to_i32_array.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/builtin_memcpy_prefix_local_pair_to_i32_array_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|memcpy.copy.0 = bir.load_local i32 %lv.src.4|bir.store_local %lv.dst.8|memcpy.copy.4 = bir.load_local i32 %lv.src.8|bir.store_local %lv.dst.12|bir.ret i32"
  FORBIDDEN_SNIPPETS "define i32 @main()|call ptr @memcpy|bir.call ptr memcpy"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_builtin_memcpy_prefix_local_i32_subarray_to_nested_pair_field_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/builtin_memcpy_prefix_local_i32_subarray_to_nested_pair_field.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/builtin_memcpy_prefix_local_i32_subarray_to_nested_pair_field_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|%t24.memcpy.copy.0 = bir.load_local i32 %lv.src.8|bir.store_local %lv.dst.8, i32 %t24.memcpy.copy.0|%t24.memcpy.copy.4 = bir.load_local i32 %lv.src.12|bir.store_local %lv.dst.12, i32 %t24.memcpy.copy.4|bir.ret i32 %t51"
  FORBIDDEN_SNIPPETS "define i32 @main()|call ptr @memcpy|bir.call ptr memcpy"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_builtin_memset_local_pair_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/builtin_memset_local_pair.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/builtin_memset_local_pair_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|bir.store_local %lv.dst.0, i32 1|bir.store_local %lv.dst.4, i32 2|bir.store_local %lv.dst.0, i32 0|bir.store_local %lv.dst.4, i32 0|bir.ret i32 %t7"
  FORBIDDEN_SNIPPETS "define i32 @main()|call ptr @memset|bir.call ptr memset"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_builtin_memset_local_i32_array_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/builtin_memset_local_i32_array.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/builtin_memset_local_i32_array_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|bir.store_local %lv.dst.0, i32 1|bir.store_local %lv.dst.1, i32 2|bir.store_local %lv.dst.2, i32 3|bir.store_local %lv.dst.0, i32 0|bir.store_local %lv.dst.1, i32 0|bir.store_local %lv.dst.2, i32 0|bir.ret i32 %t24"
  FORBIDDEN_SNIPPETS "define i32 @main()|call ptr @memset|bir.call ptr memset"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_builtin_memset_nonzero_local_pair_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/builtin_memset_nonzero_local_pair.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/builtin_memset_nonzero_local_pair_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|bir.store_local %lv.dst.0, i32 1|bir.store_local %lv.dst.4, i32 2|bir.store_local %lv.dst.0, i32 -1|bir.store_local %lv.dst.4, i32 -1|bir.ret i32 %t27"
  FORBIDDEN_SNIPPETS "define i32 @main()|call ptr @memset|bir.call ptr memset"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_builtin_memset_nonzero_local_i32_array_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/builtin_memset_nonzero_local_i32_array.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/builtin_memset_nonzero_local_i32_array_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|bir.store_local %lv.dst.0, i32 1|bir.store_local %lv.dst.1, i32 2|bir.store_local %lv.dst.2, i32 3|bir.store_local %lv.dst.0, i32 -1|bir.store_local %lv.dst.1, i32 -1|bir.store_local %lv.dst.2, i32 -1|bir.ret i32 %t54"
  FORBIDDEN_SNIPPETS "define i32 @main()|call ptr @memset|bir.call ptr memset"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_builtin_memset_nonzero_local_i32_scalar_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/builtin_memset_nonzero_local_i32_scalar.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/builtin_memset_nonzero_local_i32_scalar_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|bir.store_local %lv.dst, i32 7|bir.store_local %lv.dst, i32 -1|bir.load_local i32 %lv.dst|bir.ret i32"
  FORBIDDEN_SNIPPETS "define i32 @main()|call ptr @memset|bir.call ptr memset"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_builtin_memset_nonzero_nested_i32_scalar_field_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/builtin_memset_nonzero_nested_i32_scalar_field.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/builtin_memset_nonzero_nested_i32_scalar_field_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|bir.store_local %lv.dst.0, i32 7|bir.store_local %lv.dst.4, i32 5|bir.store_local %lv.dst.8, i32 9|bir.store_local %lv.dst.4, i32 -1|%t33 = bir.load_local i32 %lv.dst.8|%t34 = bir.eq i32 %t33, 9|bir.ret i32 %t45"
  FORBIDDEN_SNIPPETS "define i32 @main()|call ptr @memset|bir.call ptr memset"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_builtin_memset_nested_i32_array_field_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/builtin_memset_nested_i32_array_field.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/builtin_memset_nested_i32_array_field_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|bir.store_local %lv.dst.0, i32 7|bir.store_local %lv.dst.4, i32 1|bir.store_local %lv.dst.8, i32 2|bir.store_local %lv.dst.12, i32 3|bir.store_local %lv.dst.4, i32 0|bir.store_local %lv.dst.8, i32 0|bir.store_local %lv.dst.12, i32 0|bir.ret i32 %t29"
  FORBIDDEN_SNIPPETS "define i32 @main()|call ptr @memset|bir.call ptr memset"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_builtin_memset_nonzero_nested_i32_array_field_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/builtin_memset_nonzero_nested_i32_array_field.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/builtin_memset_nonzero_nested_i32_array_field_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|bir.store_local %lv.dst.0, i32 7|bir.store_local %lv.dst.4, i32 1|bir.store_local %lv.dst.8, i32 2|bir.store_local %lv.dst.12, i32 3|bir.store_local %lv.dst.4, i32 -1|bir.store_local %lv.dst.8, i32 -1|bir.store_local %lv.dst.12, i32 -1|bir.ret i32 %t68"
  FORBIDDEN_SNIPPETS "define i32 @main()|call ptr @memset|bir.call ptr memset"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_builtin_memset_nonzero_prefix_nested_i32_subarray_field_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/builtin_memset_nonzero_prefix_nested_i32_subarray_field.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/builtin_memset_nonzero_prefix_nested_i32_subarray_field_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|bir.store_local %lv.dst.0, i32 7|bir.store_local %lv.dst.4, i32 1|bir.store_local %lv.dst.8, i32 2|bir.store_local %lv.dst.12, i32 3|bir.store_local %lv.dst.16, i32 4|bir.store_local %lv.dst.20, i32 9|bir.store_local %lv.dst.8, i32 -1|bir.store_local %lv.dst.12, i32 -1|%t32 = bir.load_local i32 %lv.dst.4|%t46 = bir.load_local i32 %lv.dst.8|%t61 = bir.load_local i32 %lv.dst.12|%t76 = bir.load_local i32 %lv.dst.16|%t88 = bir.load_local i32 %lv.dst.20|bir.ret i32 %t100"
  FORBIDDEN_SNIPPETS "define i32 @main()|call ptr @memset|bir.call ptr memset"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_builtin_memset_nonzero_prefix_local_i32_subarray_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/builtin_memset_nonzero_prefix_local_i32_subarray.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/builtin_memset_nonzero_prefix_local_i32_subarray_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|bir.store_local %lv.dst.0, i32 1|bir.store_local %lv.dst.1, i32 2|bir.store_local %lv.dst.2, i32 3|bir.store_local %lv.dst.3, i32 4|bir.store_local %lv.dst.4, i32 9|bir.store_local %lv.dst.1, i32 -1|bir.store_local %lv.dst.2, i32 -1|bir.load_local i32 %lv.dst.0|bir.load_local i32 %lv.dst.1|bir.load_local i32 %lv.dst.2|bir.load_local i32 %lv.dst.3|bir.load_local i32 %lv.dst.4|bir.ret i32"
  FORBIDDEN_SNIPPETS "define i32 @main()|call ptr @memset|bir.call ptr memset"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_pointer_param_direct_global_arg_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/pointer_param_direct_global_arg.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/pointer_param_direct_global_arg_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @consume(ptr %p.p) -> i32 {|bir.call i32 consume(ptr @g)|bir.ret i32 %t0"
  FORBIDDEN_SNIPPETS "define i32 @main()|call i32 (ptr) @consume(ptr @g)"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_pointer_param_global_pointer_value_arg_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/pointer_param_global_pointer_value_arg.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/pointer_param_global_pointer_value_arg_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @consume(ptr %p.p) -> i32 {|%t0 = bir.load_global ptr @gp|%t1 = bir.load_local ptr %lv.p|bir.call i32 consume(ptr %t1)|bir.ret i32 %t2"
  FORBIDDEN_SNIPPETS "define i32 @main()|call i32 (ptr) @consume("
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_pointer_param_loaded_char_deref_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/pointer_param_loaded_char_deref.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/pointer_param_loaded_char_deref_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @load_first(ptr %p.s) -> i32 {|%t0 = bir.load_local ptr %t0.addr, addr %p.s|%t1 = bir.load_local i8 %t1.addr, addr %t0|%t2 = bir.sext i8 %t1 to i32|bir.ret i32 %t2|bir.func @main() -> i32 {|bir.store_local %lv.p, ptr %t0|%t1 = bir.call i32 load_first(ptr %lv.p)|%t2 = bir.sub i32 %t1, 104|bir.ret i32 %t2"
  FORBIDDEN_SNIPPETS "latest function failure|failed in load local-memory semantic family|define i32 @load_first"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_pointer_return_fn_param_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/pointer_return_fn_param.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/pointer_return_fn_param_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @id_ptr(ptr %p.p) -> ptr {|bir.func @use(ptr %p.fn, ptr %p.p) -> ptr {|%t0 = bir.call ptr %p.fn(ptr %p.p)|bir.ret ptr %t0"
  FORBIDDEN_SNIPPETS "define ptr @use(ptr %p.fn, ptr %p.p)|call ptr (ptr) %p.fn(ptr %p.p)"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_pointer_return_global_pointer_value_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/pointer_return_global_pointer_value.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/pointer_return_global_pointer_value_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @id_ptr(ptr %p.p) -> ptr {|bir.func @use() -> ptr {|%t0 = bir.load_global ptr @gp|%t1 = bir.load_local ptr %lv.p|bir.call ptr id_ptr(ptr %t1)|bir.ret ptr %t2"
  FORBIDDEN_SNIPPETS "define ptr @use()|call ptr (ptr) @id_ptr("
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_aggregate_param_return_pair_fn_param_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/aggregate_param_return_pair_fn_param.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/aggregate_param_return_pair_fn_param_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @use(ptr %p.fn, ptr byval(size=8, align=4) %p.p) -> i32 {|bir.call void %p.fn(ptr sret(size=8, align=4) %t0, ptr byval(size=8, align=4) %lv.param.p.p)|%t3 = bir.call i32 use(ptr @id_pair, ptr byval(size=8, align=4) %lv.p)|bir.ret i32 %t3"
  FORBIDDEN_SNIPPETS "define i32 @main()|call i32 (ptr, %struct.Pair) @use(ptr @id_pair, %struct.Pair %t2)|call %struct.Pair (%struct.Pair) %p.fn(%struct.Pair %p.p)"
)

add_test(
  NAME backend_codegen_route_x86_64_nested_member_pointer_array_observe_semantic_bir
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DTARGET_TRIPLE=x86_64-unknown-linux-gnu
          -DSRC_A=${BACKEND_CASE_ROOT}/nested_member_pointer_array.c
          -DOUT_A=${CMAKE_BINARY_DIR}/backend_route/nested_member_pointer_array_semantic_bir_x86_64.ll
          "-DREQUIRED_SNIPPETS_A=bir.func @get_second(ptr %p.o) -> i32 {|%t1 = bir.load_local ptr %t1.addr, addr %p.o|%t5 = bir.load_local i32 %t5.addr, addr %t1+4|bir.store_local %lv.outer.0, ptr %lv.inner|bir.call i32 get_second(ptr %lv.outer)|bir.ret i32 %t7"
          -DSRC_B=${BACKEND_CASE_ROOT}/local_array.c
          -DOUT_B=${CMAKE_BINARY_DIR}/backend_route/nested_member_pointer_array_local_array_semantic_bir_x86_64.ll
          "-DREQUIRED_SNIPPETS_B=bir.func @main() -> i32 {|bir.store_local %lv.xs.0, i32 4|bir.store_local %lv.xs.1, i32 3|%t10 = bir.add i32 %t6, %t9|bir.ret i32 %t10"
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_dual_codegen_route_case.cmake"
)
c4c_set_backend_test_labels(
  "backend_codegen_route_x86_64_nested_member_pointer_array_observe_semantic_bir"
  backend_route
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_nested_pointer_param_dynamic_member_array_load_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/nested_pointer_param_dynamic_member_array_load.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/nested_pointer_param_dynamic_member_array_load_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @get_at(ptr %p.p, i32 %p.i) -> i32 {|%t1 = bir.load_local ptr %t1.addr, addr %p.p+8|%t5.elt0 = bir.load_local i32 %t5.elt0.addr, addr %t1+4|%t5.elt1 = bir.load_local i32 %t5.elt1.addr, addr %t1+8|%t5.elt2 = bir.load_local i32 %t5.elt2.addr, addr %t1+12|%t5 = bir.select eq i64 %t3, 0, i32 %t5.elt0, %t5.sel1|bir.call i32 get_at(ptr %lv.outer, i32 2)|bir.ret i32 %t13"
  FORBIDDEN_SNIPPETS "define i32 @get_at|load i32, ptr|getelementptr i32, ptr"
)
c4c_set_backend_test_labels(
  backend_codegen_route_x86_64_nested_pointer_param_dynamic_member_array_load_observe_semantic_bir
  backend_route
)

add_test(
  NAME backend_codegen_route_x86_64_local_dynamic_member_array_observe_semantic_bir
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DTARGET_TRIPLE=x86_64-unknown-linux-gnu
          -DSRC_A=${BACKEND_CASE_ROOT}/local_dynamic_member_array.c
          -DOUT_A=${CMAKE_BINARY_DIR}/backend_route/local_dynamic_member_array_semantic_bir_x86_64.ll
          "-DREQUIRED_SNIPPETS_A=bir.func @get_at(ptr %p.p, i32 %p.i) -> i32 {|%t3.elt0 = bir.load_local i32 %t3.elt0.addr, addr %p.p|%t3.elt1 = bir.load_local i32 %t3.elt1.addr, addr %p.p+4|%t3.elt2 = bir.load_local i32 %t3.elt2.addr, addr %p.p+8|%t3 = bir.select eq i64 %t1, 0, i32 %t3.elt0, %t3.sel1|bir.call i32 get_at(ptr %lv.p, i32 2)|bir.ret i32 %t9"
          "-DFORBIDDEN_SNIPPETS_A=define i32 @get_at|getelementptr [3 x i32], ptr"
          -DSRC_B=${BACKEND_CASE_ROOT}/local_array.c
          -DOUT_B=${CMAKE_BINARY_DIR}/backend_route/local_dynamic_member_array_local_array_semantic_bir_x86_64.ll
          "-DREQUIRED_SNIPPETS_B=bir.func @main() -> i32 {|bir.store_local %lv.xs.0, i32 4|bir.store_local %lv.xs.1, i32 3|%t10 = bir.add i32 %t6, %t9|bir.ret i32 %t10"
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_dual_codegen_route_case.cmake"
)
c4c_set_backend_test_labels(
  "backend_codegen_route_x86_64_local_dynamic_member_array_observe_semantic_bir"
  backend_route
  x86_dynamic_indexed_gep_local_memory_lane
)

add_test(
  NAME backend_codegen_route_x86_64_local_dynamic_member_array_store_observe_semantic_bir
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DTARGET_TRIPLE=x86_64-unknown-linux-gnu
          -DSRC_A=${BACKEND_CASE_ROOT}/local_dynamic_member_array_store.c
          -DOUT_A=${CMAKE_BINARY_DIR}/backend_route/local_dynamic_member_array_store_semantic_bir_x86_64.ll
          "-DREQUIRED_SNIPPETS_A=bir.func @set_at(ptr %p.p, i32 %p.i, i32 %p.v) -> void {|%t1 = bir.sext i32 %p.i to i64|%t2.elt0 = bir.load_local i32 %t2.elt0.load.addr, addr %p.p|%t2.store0 = bir.select eq i64 %t1, 0, i32 %p.v, %t2.elt0|bir.store_local %t2.elt0.store.addr, i32 %t2.store0, addr %p.p|%t2.elt1 = bir.load_local i32 %t2.elt1.load.addr, addr %p.p+4|%t2.store1 = bir.select eq i64 %t1, 1, i32 %p.v, %t2.elt1|bir.store_local %t2.elt1.store.addr, i32 %t2.store1, addr %p.p+4|%t2.elt2 = bir.load_local i32 %t2.elt2.load.addr, addr %p.p+8|%t2.store2 = bir.select eq i64 %t1, 2, i32 %p.v, %t2.elt2|bir.store_local %t2.elt2.store.addr, i32 %t2.store2, addr %p.p+8|bir.ret"
          "-DFORBIDDEN_SNIPPETS_A=define void @set_at|store i32 %p.v, ptr|getelementptr [3 x i32], ptr"
          -DSRC_B=${BACKEND_CASE_ROOT}/local_dynamic_member_array.c
          -DOUT_B=${CMAKE_BINARY_DIR}/backend_route/local_dynamic_member_array_store_neighbor_semantic_bir_x86_64.ll
          "-DREQUIRED_SNIPPETS_B=bir.func @get_at(ptr %p.p, i32 %p.i) -> i32 {|%t3.elt0 = bir.load_local i32 %t3.elt0.addr, addr %p.p|%t3.elt1 = bir.load_local i32 %t3.elt1.addr, addr %p.p+4|%t3.elt2 = bir.load_local i32 %t3.elt2.addr, addr %p.p+8|%t3 = bir.select eq i64 %t1, 0, i32 %t3.elt0, %t3.sel1|bir.call i32 get_at(ptr %lv.p, i32 2)|bir.ret i32 %t9"
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_dual_codegen_route_case.cmake"
)
c4c_set_backend_test_labels(
  "backend_codegen_route_x86_64_local_dynamic_member_array_store_observe_semantic_bir"
  backend_route
  x86_dynamic_indexed_gep_local_memory_lane
)

add_test(
  NAME backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DTARGET_TRIPLE=x86_64-unknown-linux-gnu
          -DSRC_A=${BACKEND_CASE_ROOT}/local_direct_dynamic_member_array_store.c
          -DOUT_A=${CMAKE_BINARY_DIR}/backend_route/local_direct_dynamic_member_array_store_semantic_bir_x86_64.ll
          "-DREQUIRED_SNIPPETS_A=bir.func @main() -> i32 {|bir.store_local %lv.p.0, i32 4|bir.store_local %lv.p.4, i32 8|bir.store_local %lv.p.8, i32 11|bir.store_local %lv.i, i32 1|%t12.elt0 = bir.load_local i32 %lv.p.0|%t12.store1 = bir.select eq i64 %t11, 1, i32 13, %t12.elt1|bir.store_local %lv.p.8, i32 %t12.store2|%t16 = bir.load_local i32 %lv.p.4|bir.ret i32 %t16"
          "-DFORBIDDEN_SNIPPETS_A=define i32 @main()|store i32 13, ptr|getelementptr [3 x i32], ptr"
          -DSRC_B=${BACKEND_CASE_ROOT}/local_array.c
          -DOUT_B=${CMAKE_BINARY_DIR}/backend_route/local_direct_dynamic_member_array_store_local_array_semantic_bir_x86_64.ll
          "-DREQUIRED_SNIPPETS_B=bir.func @main() -> i32 {|bir.store_local %lv.xs.0, i32 4|bir.store_local %lv.xs.1, i32 3|%t10 = bir.add i32 %t6, %t9|bir.ret i32 %t10"
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_dual_codegen_route_case.cmake"
)
c4c_set_backend_test_labels(
  "backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir"
  backend_route
)

add_test(
  NAME backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir
  COMMAND "${CMAKE_COMMAND}"
          -DCOMPILER=$<TARGET_FILE:c4cll>
          -DTARGET_TRIPLE=x86_64-unknown-linux-gnu
          -DSRC_A=${BACKEND_CASE_ROOT}/local_direct_dynamic_member_array_load.c
          -DOUT_A=${CMAKE_BINARY_DIR}/backend_route/local_direct_dynamic_member_array_load_semantic_bir_x86_64.ll
          "-DREQUIRED_SNIPPETS_A=bir.func @main() -> i32 {|bir.store_local %lv.p.0, i8 2|bir.store_local %lv.p.4, i32 4|bir.store_local %lv.p.8, i32 8|bir.store_local %lv.p.12, i32 11|bir.store_local %lv.i, i32 1|%t12 = bir.load_local i32 %lv.i|%t13 = bir.sext i32 %t12 to i64|%t15.elt0 = bir.load_local i32 %lv.p.4|%t15.elt1 = bir.load_local i32 %lv.p.8|%t15.elt2 = bir.load_local i32 %lv.p.12|%t15 = bir.select eq i64 %t13, 0, i32 %t15.elt0, %t15.sel1|bir.ret i32 %t15"
          "-DFORBIDDEN_SNIPPETS_A=define i32 @main()|load i32, ptr|getelementptr [3 x i32], ptr"
          -DSRC_B=${BACKEND_CASE_ROOT}/local_array.c
          -DOUT_B=${CMAKE_BINARY_DIR}/backend_route/local_direct_dynamic_member_array_load_local_array_semantic_bir_x86_64.ll
          "-DREQUIRED_SNIPPETS_B=bir.func @main() -> i32 {|bir.store_local %lv.xs.0, i32 4|bir.store_local %lv.xs.1, i32 3|%t10 = bir.add i32 %t6, %t9|bir.ret i32 %t10"
          -P "${BACKEND_TEST_CMAKE_ROOT}/run_backend_dual_codegen_route_case.cmake"
)
c4c_set_backend_test_labels(
  "backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir"
  backend_route
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_packed_local_member_offsets_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/packed_local_member_offsets.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/packed_local_member_offsets_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|bir.store_local %lv.p.0, i8 2|bir.store_local %lv.p.1, i32 4|bir.store_local %lv.p.5, i32 8|bir.store_local %lv.p.9, i32 11|%t15.elt0 = bir.load_local i32 %lv.p.1|%t15.elt1 = bir.load_local i32 %lv.p.5|%t15.elt2 = bir.load_local i32 %lv.p.9|%t15 = bir.select eq i64 %t13, 0, i32 %t15.elt0, %t15.sel1|bir.ret i32 %t15"
  FORBIDDEN_SNIPPETS "define i32 @main()|bir.store_local %lv.p.4, i32 4|getelementptr [3 x i32], ptr"
)

c4c_add_backend_codegen_route_test(
  backend_codegen_route_x86_64_local_direct_dynamic_struct_array_call_observe_semantic_bir
  SRC "${BACKEND_CASE_ROOT}/local_direct_dynamic_struct_array_call.c"
  TARGET_TRIPLE x86_64-unknown-linux-gnu
  OUT_TEXT "${CMAKE_BINARY_DIR}/backend_route/local_direct_dynamic_struct_array_call_semantic_bir_x86_64.ll"
  EXTRA_COMPILER_ARGS "--backend-bir-stage|semantic"
  REQUIRED_SNIPPETS "bir.func @main() -> i32 {|bir.store_local %lv.pairs.0, i32 1|bir.store_local %lv.pairs.12, i32 4|%t12 = bir.load_local i32 %lv.i|%t13 = bir.sext i32 %t12 to i64|%t14 = bir.select eq i64 %t13, 0, ptr %t14.elt0, %t14.elt1|bir.call i32 get_y(ptr %t14)|bir.ret i32 %t15"
  FORBIDDEN_SNIPPETS "define i32 @main()|call i32 (ptr) @get_y|getelementptr [2 x %struct.Pair], ptr"
)

get_property(C4C_BIR_BACKEND_TESTS DIRECTORY PROPERTY TESTS)
foreach(test_name IN LISTS C4C_BIR_BACKEND_TESTS)
  set_property(TEST "${test_name}" APPEND PROPERTY LABELS bir)
endforeach()
