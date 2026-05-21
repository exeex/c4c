# AArch64 Materialized Pointer StoreLocal Writeback Runbook

Status: Active
Source Idea: ideas/open/361_aarch64_materialized_pointer_storelocal_writeback.md

## Purpose

Repair the AArch64 residual where materialized pointer-addressed stores do not
write back to their addressed memory, leaving `00181`'s later Hanoi moves
unchanged after the starting-state fix.

## Goal

Make pointer-addressed `StoreLocal` or equivalent stores update the selected
destination memory through a general lowering rule, while preserving the idea
360 starting-state repair.

## Core Rule

Localize and repair the general materialized pointer-addressed store writeback
boundary. Do not special-case `00181`, `Hanoi`, tower names, literal values,
one stack offset, one ABI register, or one emitted instruction neighborhood.

## Read First

- `ideas/open/361_aarch64_materialized_pointer_storelocal_writeback.md`
- `ideas/open/360_aarch64_hanoi_starting_state_output_mismatch.md`
- `review/360_starting_state_slice_review.md`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `tests/c/external/c-testsuite/src/00181.c`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`
- Recent idea 357, 358, 359, and 360 notes

## Current Targets

- Primary external representative: `c_testsuite_aarch64_backend_src_00181_c`
- Stability representatives: `c_testsuite_aarch64_backend_src_00170_c`,
  `c_testsuite_aarch64_backend_src_00189_c`
- Backend contracts:
  `backend_aarch64_instruction_dispatch`,
  `backend_aarch64_memory_operand_contract`,
  `backend_prepare_frame_stack_call_contract`,
  `backend_cli_dump_prepared_bir_local_arg_call_contract`

## Non-Goals

- Do not reopen the direct `LoadGlobal` current-memory select-store repair
  from idea 360 except to keep it stable.
- Do not reopen stack-preserved pointer formal post-call handling from idea
  359.
- Do not reopen scalar formal post-call reloads from idea 358.
- Do not reopen pointer-formal callee-saved home publication from idea 357.
- Do not reopen address-valued call-argument publication from idea 355 unless
  fresh evidence proves the same owner and lifecycle state is split again.
- Do not handle semantic-BIR dynamic pointer-derived string loads for `00173`.
- Do not handle frontend or semantic admission failure for `00005`.
- Do not change expectations, unsupported classifications, runner behavior,
  timeout policy, CTest registration, or proof-log policy.
- Do not broaden into ABI composite/byval/HFA/f128, variadic floating,
  dynamic stack, or unrelated scalar compare/select residuals.

## Working Model

After commit `87e79a50a`, `00181` prints the correct starting state:
`A: 1 2 3 4`, `B: 0 0 0 0`, `C: 0 0 0 0`. The later tower states remain
unchanged because the broader pointer-addressed `StoreLocal` fallback was
removed from the accepted idea 360 slice. The owner is now the first
materialized pointer-addressed store that should mutate tower memory after the
starting-state print.

## Execution Rules

- Use generated-code and prepared-BIR evidence to confirm the first bad
  pointer-addressed store fact before editing.
- Compare semantic BIR, prepared BIR, and generated AArch64 before choosing
  the owner boundary.
- Prefer existing memory, local, GEP, address-projection, and publication
  helpers and local codegen patterns.
- Add focused backend coverage for the localized pointer-addressed writeback
  shape before claiming capability progress.
- Keep the idea 360 starting-state output correct.
- Keep ideas 357, 358, and 359 stable.
- Treat named-case matching or expectation weakening as route failure.

## Step 1: Localize The Pointer-Addressed Store Boundary

Goal: find the first store after the correct starting-state print that should
change tower memory but does not.

Actions:

- Reproduce the focused subset including `00181`, `00170`, `00189`, and the
  backend contracts listed above.
- Inspect `00181.c`, semantic BIR, prepared BIR, and generated AArch64 for the
  first post-starting-state tower mutation.
- Identify the producing source store, materialized address carrier, expected
  target memory, emitted store behavior, and later load or print consumer.
- Record the first bad fact and any adjacent same-shape examples in `todo.md`.

Completion check:

- `todo.md` names the source store operation, pointer/address carrier,
  expected destination, first wrong boundary, and consumer path.
- The idea 360 starting-state repair remains outside this step unless fresh
  evidence contradicts the split.

## Step 2: Repair The Materialized Pointer Store Rule

Goal: repair the general AArch64 lowering rule that fails to commit
pointer-addressed stores to addressed memory.

Actions:

- Update only the localized AArch64 store/writeback boundary.
- Ensure the repair applies to the general materialized pointer-addressed
  store shape, not just `00181` or one emitted offset.
- Preserve direct `LoadGlobal` current-memory select-store handling from idea
  360.
- Preserve recursive formal post-call repairs from ideas 357, 358, and 359.
- Add focused backend coverage for the repaired writeback shape.

Completion check:

- Focused backend coverage fails before the repair and passes after it.
- Generated code or execution evidence shows the representative store updates
  the addressed memory.
- `git diff --check` passes.

## Step 3: Prove Focused External Progress

Goal: prove the pointer-addressed store repair advances `00181` without
regressing nearby repaired paths.

Actions:

- Run:
  `cmake --build --preset default && ctest --test-dir build -j10 --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_contract|backend_prepare_frame_stack_call_contract|backend_cli_dump_prepared_bir_local_arg_call_contract|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00170_c|c_testsuite_aarch64_backend_src_00189_c)$' | tee test_after.log`
- If `00181` still fails, classify the new first bad fact in `todo.md` and
  split it only if it is outside the pointer-addressed store owner.
- Keep `00170`, `00189`, and the idea 360 starting-state output passing.

Completion check:

- The delegated proof command is recorded in `todo.md`.
- `00181` passes or has a newly localized out-of-scope first bad fact.
- Stability representatives remain passing.

## Step 4: Broader Backend Guard And Lifecycle Decision

Goal: decide whether the residual is complete and safe to close or whether a
new first bad fact needs its own source idea.

Actions:

- Compare the final residual, if any, against the source idea's in-scope and
  out-of-scope lists.
- Ask the supervisor for a broader backend guard before closure because the
  repair touches shared store/writeback behavior.
- Split unrelated residuals into new `ideas/open/` source ideas instead of
  absorbing them into this plan.

Completion check:

- The pointer-addressed store residual closes only when it is repaired,
  guarded, and not masking a separate owner.
- Any unrelated residual is split into a new source idea.
