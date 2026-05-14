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
