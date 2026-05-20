Status: Active
Source Idea Path: ideas/open/339_aarch64_scalar_local_storage_writeback_sizing.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Scalar Local Sizing And Writeback

# Current Packet

## Just Finished

Step 2 repaired the focused scalar local sizing/writeback path for `I16`
locals and the downstream AArch64 halfword emission needed by `00086` and
`00111`.

Prepared sizing is fixed first: LIR-origin scalar local allocas now publish
`size_bytes`, the stack-layout fallback size table includes `I16 = 2`, and
legalization backfills zero scalar size/alignment facts for existing `I16`
locals. Focused backend coverage now proves an `i16` local stack object,
frame slot, frame metrics, and init/load/writeback/reload prepared memory
accesses all carry `size=2 align=2`.

AArch64 emission now consumes those facts: memory selection/printer paths accept
2-byte immediate stores, choose `ldrh`/`strh` for `size_bytes=2`, and cover the
2-byte symbol immediate-store path. Stack-backed simple integer casts now
require explicit load/publication instead of assuming a consumer register was
already populated, and stack-home truncation results are materialized and stored
back with the `I16` width.

Generated AArch64 for the representatives now emits short initialization,
halfword reloads, truncation/writeback stores, and the focused runtime subset
passes for both `00086` and `00111`.

## Suggested Next

Supervisor should review and commit the coherent Step 2 slice, then delegate
Step 3 focused proof/reclassification if broader guard results are acceptable.

## Watchouts

- Keep this owner focused on scalar local storage/writeback sizing for
  non-address-exposed scalar locals.
- Do not fold in parked FP comparison/expression, pointer/null conditional,
  broad return-spill, aggregate, variadic, timeout, runner, expectation, or
  CTest-registration work.
- Do not special-case `00086`, `00111`, stack offsets, variable names, or
  emitted instruction strings.
- The repaired prepared facts are visible in `--dump-prepared-bir`; do not
  reopen the zero-size stack-layout path unless a new proof contradicts those
  facts.
- This packet intentionally changed the dispatch test for stack-sourced simple
  casts to require an explicit load/publication before the cast instead of
  accepting a structured register source with no emitted population.
- The 2-byte symbol immediate-store path is now covered at printer level; do
  not remove it as unreachable without a separate selector/lowering proof.

## Proof

Delegated proof command was run exactly and preserved in `test_after.log`:

`cmake --build build --target c4cll backend_prepare_stack_layout_test backend_prepare_frame_stack_call_contract_test backend_aarch64_memory_operand_records_test backend_aarch64_prepared_memory_operand_records_test backend_aarch64_machine_printer_test backend_aarch64_instruction_dispatch_test -j 2 && ctest --test-dir build -j --output-on-failure -R 'backend_prepare_(stack_layout|frame_stack_call_contract)|backend_aarch64_(memory_operand_records|prepared_memory_operand_records|machine_printer|instruction_dispatch)|c_testsuite_aarch64_backend_src_(00086|00111)_c' > test_after.log 2>&1`

Result: build passed and all 8 selected tests passed, including
`backend_prepare_stack_layout`,
`backend_aarch64_machine_printer`,
`backend_aarch64_instruction_dispatch`,
`c_testsuite_aarch64_backend_src_00086_c`, and
`c_testsuite_aarch64_backend_src_00111_c`.
