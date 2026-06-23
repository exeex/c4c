Status: Active
Source Idea Path: ideas/open/317_rv64_select_chain_short_circuit_runtime_lowering.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Focused Select/Short-Circuit Coverage

# Current Packet

## Just Finished

Completed idea 317 Step 2 by adding focused expected-repair backend coverage
for the short-circuit/select-chain boundary found in Step 1.

New focused fixtures:

- `tests/backend/case/riscv64_short_circuit_select_false_lhs.c`
- `tests/backend/case/riscv64_compare_result_select_false_arm.c`

New expected-repair tests:

- `backend_dump_riscv64_short_circuit_select_false_lhs`
- `backend_dump_riscv64_compare_result_select_false_arm`
- `backend_codegen_route_riscv64_short_circuit_select_false_lhs_expected_repair`
- `backend_codegen_route_riscv64_compare_result_select_false_arm_expected_repair`

The short-circuit fixture covers `&&` with a false LHS where the lowered select
must publish `0`, and it includes a small aggregate-field guard so aggregate
stores/reloads stay visible without copying `src/00046.c`. The compare-result
fixture covers a nearby explicit compare-result select with a compare true arm
and `0` false arm feeding a branch. The codegen tests assert the current bad
RV64 route stays captured: the short-circuit case currently reaches the
return-2 path, and the compare-result case currently truncates after no-op
self moves.

## Suggested Next

Repair RV64 select-chain materialization/publication for compare-result select
arms so a false LHS short-circuit publishes `0` and explicit compare-result
selects can flow through local storage/branch use without truncating.

## Watchouts

- Do not reopen aggregate-local subobject stores/loads repaired by idea 314
  based on this route; the Step 2 aggregate guard is only there to keep that
  context visible while testing short-circuit/select behavior.
- Do not special-case `src/00046.c`, the new fixture filenames, fixed offsets,
  or one exact expression shape.
- Runtime/qemu coverage should be converted to positive contracts only after
  the repair; this packet used expected-repair codegen checks so the suite
  stays green before implementation.

## Proof

Focused proof passed 4/4:
`cmake --build --preset default -j && ctest --test-dir build -j
--output-on-failure -R 'backend_(dump|codegen_route)_riscv64_(short_circuit_select_false_lhs|compare_result_select_false_arm)'`.

Ran delegated proof
`cmake --build --preset default -j && ctest --test-dir build -j
--output-on-failure -R '^backend_' > test_after.log`. The new expected-repair
tests passed in the full backend run. The backend subset still did not pass
because the existing `backend_riscv_prepared_edge_publication` test failed with
"RISC-V prepared module should emit a register edge move"; `test_after.log` is
the proof log.
