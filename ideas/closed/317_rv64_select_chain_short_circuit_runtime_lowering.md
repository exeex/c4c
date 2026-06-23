# RV64 Select Chain Short Circuit Runtime Lowering

## Goal

Repair RV64 prepared lowering for select-chain and short-circuit control/value
flow when generated code emits and links but computes the wrong runtime result.

## Why This Exists

Idea 314 repaired the aggregate-local subobject mechanism for `src/00046.c`.
Step 5 evidence in
`build/rv64_c_testsuite_probe_latest/triage_314_step5/summary.md` shows the
candidate now emits aggregate stores/reloads at the expected local offsets, but
qemu exits `2`. The remaining first bad mechanism is a separate select-chain /
short-circuit runtime lowering residual, not nested aggregate store emission.

## In Scope

- RV64 prepared select-chain materialization and short-circuit runtime lowering.
- Control/value joins that feed select-chain results in emitted RV64 code.
- Focused backend coverage for select-chain and short-circuit semantics,
  including the shape exposed by `src/00046.c`.
- Emitted-code evidence that distinguishes control/select failure from local
  aggregate storage failure.

## Out Of Scope

- Reopening aggregate-local subobject stores/loads repaired by idea 314.
- Generic local frame-slot address publication from idea 312.
- Broad boolean/control-flow rewrites unrelated to select-chain or
  short-circuit materialization.
- Byval aggregate call ABI, vararg ABI, or floating aggregate lane repair.

## Acceptance Criteria

- Focused tests cover RV64 select-chain and short-circuit runtime semantics.
- `src/00046.c` either emits, links, and exits under qemu with status `0`, or
  any remaining failure is reclassified with concrete emitted-code evidence as
  a different mechanism.
- Repairs consume or improve prepared select-chain/control facts rather than
  matching the c-testsuite filename, fixed aggregate offsets, or expression
  spelling.
- Existing aggregate-local focused tests remain green.

## Completion Note

Closed after Step 4 evidence in
`build/rv64_c_testsuite_probe_latest/triage_317_step4/summary.md` and
`probe_results.tsv`.

`src/00046.c` now passes the full RV64 probe: BIR dump, prepared-BIR dump, RV64
assembly emission, clang link, and qemu all returned `0`.

The final emitted RV64 code publishes the false-LHS select value, computes the
compare arm, feeds the selected value into the branch, and continues through
the aggregate-field checks. Focused Step 3 coverage was converted to positive
dump, codegen-route, and RV64 runtime contracts for:

- `riscv64_short_circuit_select_false_lhs`
- `riscv64_compare_result_select_false_arm`

Aggregate-local focused neighbors from idea 314 also stayed green:
`riscv64_aggregate_local_self_pointer_chain` and
`riscv64_aggregate_local_anonymous_union_fields`.

No concrete residual was found in the Step 4 reprobe, so no follow-up idea was
created.

Close-time guard: `cmake --build --preset default -j` passed, then
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
matched the rolled-forward `test_before.log` backend subset. The regression
guard passed with `--allow-non-decreasing-passed`: 260 passed, 1 accepted
existing failure (`backend_riscv_prepared_edge_publication`), 0 new failures.

## Reviewer Reject Signals

- The route special-cases `src/00046.c`, fixed offsets, or the exact source
  expression instead of repairing select-chain or short-circuit lowering.
- Progress is claimed by weakening the qemu contract, marking the test
  unsupported, or editing expectations without a runtime semantic repair.
- The patch changes aggregate-local storage again without proving the remaining
  first bad mechanism is aggregate storage rather than select-chain control.
- The implementation adds target-local scans that duplicate prepared
  select-chain facts while leaving the semantic failure unchanged.
- Only one named testcase is proven while nearby select-chain or short-circuit
  shapes remain unexamined.
