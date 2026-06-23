Status: Active
Source Idea Path: ideas/open/319_rv64_stack_homed_fused_compare_control_flow.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Reprobe and Close Classification

# Current Packet

## Just Finished

Rewrote the active transcript after reviewer feedback so the current dirty
test slice is recorded as fixture narrowing plus residual classification, not
as Step 3/4 backend repair progress.

Current dirty test-scope correction:

- `tests/backend/case/riscv64_stack_homed_fused_compare_missing_false_label.c`

Focused tests affected:

- `backend_dump_riscv64_stack_homed_fused_compare_missing_false_label`
- `backend_codegen_route_riscv64_stack_homed_fused_compare_missing_false_label`
- `backend_rv64_runtime_riscv64_stack_homed_fused_compare_missing_false_label`

Classification result:

- The fixture now uses 16-element local `int` arrays plus a post-loop switch so
  the prepared loop condition still has a stack-homed fused compare and false
  successor `block_2`, while avoiding the later i16/select local-array residual
  that made the previous short-array fixture truncate before block emission.
- The codegen test now requires both the false-successor jump and the
  `.Lmain_block_2:` label definition, and the runtime test links/runs this
  narrowed focused fixture under qemu.
- This is a valid Step 2/Step 5-style fixture correction and reclassification
  only. It does not implement, and must not be committed as, a Step 3 or Step 4
  compiler/backend repair because the dirty slice has no implementation diff.
- Fresh `src/00077.c` reprobe emits, links, and exits qemu with status 0 in
  the current evidence.
- Fresh `src/00143.c` reprobe still emits a partial file that jumps to
  `.Lmain_block_2` without defining it, but the emitted file truncates inside
  `.Lmain_block_1` after the loop-condition branch has already been emitted.
  Prepared evidence shows the next body operations are the i32-to-i64 index
  cast plus i16 local-array select/store publication (`%t9.store0` family), so
  the current transcript classifies it as a later local-array/select residual
  outside idea 319's compare/control-flow boundary.

## Suggested Next

Proceed with Step 5. The next real work is to decide whether idea 319 can close
on this reclassification evidence, or whether actual Step 3/4 backend repair
is still required. Before accepting close, run or consume the lifecycle
regression guard and create a separate follow-up idea if the halfword
local-array/select residual is not already covered elsewhere.

## Watchouts

- Do not special-case filenames, SSA names such as `%t5` or `%t1`, or observed
  stack offsets.
- Do not describe the current dirty slice as a compiler repair; it changes
  tests and `todo.md` only.
- `src/00077.c` passes in fresh evidence; do not use it as a failing
  acceptance target unless it regresses in the Step 5 close probe.
- `src/00143.c` does not reach qemu because clang rejects the missing false
  successor label first, but the current evidence points at partial output after
  a later i16 local-array select/store residual rather than a missing
  stack-homed fused-compare branch emission rule.
- Do not fold halfword local-array select/store, nested select-chain,
  aggregate/byval, function-pointer, or external-call work into this route.

## Proof

Focused proof passed:
`cmake --build --preset default -j && ctest --test-dir build -j --output-on-failure -R 'backend_(dump|codegen_route|rv64_runtime)_riscv64_stack_homed_fused_compare_missing_false_label'`.

Delegated proof ran:
`cmake --build --preset default -j && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`.

Result: build passed; backend CTest returned nonzero because the existing
unrelated `backend_riscv_prepared_edge_publication` test still fails. The
focused stack-homed fused-compare dump/codegen/runtime tests all pass in
`test_after.log`.

Fresh candidate artifacts:
`build/rv64_c_testsuite_probe_latest/triage_319_step3/probe_results.tsv` and
`build/rv64_c_testsuite_probe_latest/triage_319_step3/summary.md`.

Reviewer trigger:
`review/rv64_stack_homed_fused_compare_route_review.md` accepted the fixture
narrowing only as reclassification and rejected the previous transcript wording
as overclaiming backend repair progress.
