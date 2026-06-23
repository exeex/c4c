Status: Active
Source Idea Path: ideas/open/319_rv64_stack_homed_fused_compare_control_flow.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Focused Stack-Homed Compare Control Coverage

# Current Packet

## Just Finished

Completed Step 2 focused expected-repair coverage for the stack-homed fused
compare missing false-successor label failure.

New focused fixture:

- `tests/backend/case/riscv64_stack_homed_fused_compare_missing_false_label.c`

New focused tests:

- `backend_dump_riscv64_stack_homed_fused_compare_missing_false_label`
- `backend_codegen_route_riscv64_stack_homed_fused_compare_missing_false_label`

Focused coverage result:

- The dump test proves a prepared loop condition with a fused compare whose
  false successor is `block_2`, with stack-slot GPR homes present in the same
  prepared route. It avoids c-testsuite filenames, fixed offsets, and exact
  generated compare SSA names.
- The codegen expected-repair test captures the current failure: emitted RV64
  contains a false-successor jump to `.Lmain_block_2` and a true-successor
  `.Lmain_block_1:` label, but no `.Lmain_block_2:` label definition.
- The fixture is a nearby same-feature case using 16-element local short arrays
  plus a post-loop switch; it does not copy `src/00143.c` or Duff's-device
  source shape.
- `src/00077.c` remains passing evidence from Step 1 and is not used as a
  failing acceptance target.

## Suggested Next

Repair RV64 prepared function/block emission so a stack-homed fused compare
loop condition's false successor is emitted as a defined block label, then flip
the expected-repair codegen assertion to a positive label-definition/link or
runtime contract.

## Watchouts

- Do not special-case filenames, SSA names such as `%t5` or `%t1`, or observed
  stack offsets.
- `src/00077.c` now passes in fresh evidence; do not use it as a failing
  acceptance target unless it regresses.
- `src/00143.c` does not reach qemu because clang rejects the missing false
  successor label first.
- The new codegen test intentionally asserts the current broken missing-label
  output and is labeled `expected_repair`; the repair packet should flip it
  rather than preserve that output.
- Do not fold nested select-chain/store-source, aggregate/byval,
  function-pointer, or external-call work into this route.

## Proof

Focused proof passed:
`cmake --build --preset default -j && ctest --test-dir build -j --output-on-failure -R 'backend_(dump|codegen_route)_riscv64_stack_homed_fused_compare_missing_false_label'`.

Delegated proof ran:
`cmake --build --preset default -j && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`.

Result: build passed; backend CTest returned nonzero because the existing
unrelated `backend_riscv_prepared_edge_publication` test still fails. The new
stack-homed fused-compare expected-repair dump/codegen tests pass in the final
proof log. Fresh log: `test_after.log`.
