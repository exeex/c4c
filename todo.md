Status: Active
Source Idea Path: ideas/open/317_rv64_select_chain_short_circuit_runtime_lowering.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Select-Chain Or Short-Circuit Lowering

# Current Packet

## Just Finished

Completed idea 317 Step 3 by repairing RV64 select-chain/short-circuit
materialization for compare-result select arms.

Repair boundary:

- RV64 select emission now materializes same-block compare results when they
  are used as selected `i32` arm values, so a false-LHS short-circuit can
  publish `0` while the true arm computes the compare value.
- RV64 prepared function emission records compares used directly by a block's
  conditional terminator and lets the branch emit the compare instead of
  requiring an unused standalone boolean materialization.
- RV64 prepared scalar frame-slot loads now have a value-keyed same-block
  fallback, matching the prepared memory access by load result when
  select-chain carriers shift instruction indexes.
- Prepared `phi` carriers that remain as out-of-SSA/select-chain metadata are
  skipped in the RV64 prepared function loop; the actual value movement is
  handled by the emitted select/materialization path.

Converted focused tests:

- `backend_codegen_route_riscv64_short_circuit_select_false_lhs`
- `backend_codegen_route_riscv64_compare_result_select_false_arm`
- `backend_rv64_runtime_riscv64_short_circuit_select_false_lhs`
- `backend_rv64_runtime_riscv64_compare_result_select_false_arm`

The dump tests remain positive evidence of the prepared select-chain facts.
The aggregate-local focused neighbors stayed green.

## Suggested Next

Reprobe `src/00046.c` for Step 4/next evidence now that the focused
short-circuit/select-chain repair passes runtime.

## Watchouts

- Do not reopen aggregate-local subobject stores/loads repaired by idea 314
  based on this route; the Step 2 aggregate guard is only there to keep that
  context visible while testing short-circuit/select behavior.
- Do not special-case `src/00046.c`, the new fixture filenames, fixed offsets,
  or one exact expression shape.
- `src/00046.c` should be treated as evidence for the next packet, not claimed
  closed solely from the focused fixtures.

## Proof

Focused proof passed 12/12:
`cmake --build --preset default -j && ctest --test-dir build -j
--output-on-failure -R 'backend_(dump|codegen_route|rv64_runtime)_riscv64_(short_circuit_select_false_lhs|compare_result_select_false_arm|aggregate_local_(self_pointer_chain|anonymous_union_fields))'`.

Delegated proof ran:
`cmake --build --preset default -j && ctest --test-dir build -j
--output-on-failure -R '^backend_' > test_after.log`.

Result: build passed; backend CTest returned nonzero because the existing
`backend_riscv_prepared_edge_publication` test still fails with
`RISC-V prepared module should emit a register edge move`. The focused Step 3
dump/codegen/runtime tests passed, including the aggregate-local neighbors.
