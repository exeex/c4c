# AArch64 Hanoi First Move Peg Selection Mismatch Runbook

Status: Active
Source Idea: ideas/open/364_aarch64_hanoi_first_move_peg_selection_mismatch.md

## Purpose

Repair the next `00181` AArch64 residual after prepared scan joins terminate
correctly: the first printed Hanoi move places disk `1` on `B` instead of the
expected `C`.

## Goal

Localize and repair the backend owner that chooses or publishes the wrong peg
for the first disk move, without reopening the stale scan-join reload repair.

## Core Rule

Repair the general owner for the first wrong move. Do not special-case `00181`,
`Move`, Hanoi tower globals, peg letters, output text, block labels, stack
offsets, ABI registers, or one emitted instruction neighborhood.

## Read First

- `ideas/open/364_aarch64_hanoi_first_move_peg_selection_mismatch.md`
- `ideas/closed/363_aarch64_prepared_select_condition_join_stale_reload.md`
- `ideas/open/362_aarch64_pointer_derived_load_address_scaling_timeout.md`
- `ideas/open/361_aarch64_materialized_pointer_storelocal_writeback.md`
- `ideas/open/360_aarch64_hanoi_starting_state_output_mismatch.md`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `tests/c/external/c-testsuite/src/00181.c`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`

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

- Do not reopen prepared select/condition stale scan-join reloads from idea 363
  unless the exact stale reload failure reappears.
- Do not reopen pointer-derived load/address scaling from idea 362 except to
  keep it stable.
- Do not reopen materialized pointer-addressed store writeback from idea 361
  except to keep it stable.
- Do not reopen the idea 360 direct `LoadGlobal` current-memory select-store
  repair except to keep it stable.
- Do not reopen recursive formal post-call repairs from ideas 357, 358, and
  359 without fresh first-bad-fact evidence that the new mismatch belongs
  there.
- Do not handle semantic-BIR dynamic pointer-derived string loads for `00173`.
- Do not handle frontend or semantic admission failure for `00005`.
- Do not change expectations, unsupported classifications, runner behavior,
  timeout policy, CTest registration, or proof-log policy.
- Do not broaden into ABI composite/byval/HFA/f128, variadic floating, dynamic
  stack, or unrelated load-address scaling residuals.

## Working Model

Commit `0efc079a5` repaired the stale prepared select/condition join reload
owner from idea 363. Live generated `00181` no longer reloads stale branch
operands at `.LBB193_18` or `.LBB193_25`; both scan joins branch from the
published `w13`.

The new first bad fact is a runtime output mismatch. The first subsequent Hanoi
state places disk `1` on `B` (`B: 0 0 0 1`, `C: 0 0 0 0`) instead of expected
`C` (`B: 0 0 0 0`, `C: 0 0 0 1`). The next owner may be argument/peg
selection, recursive call state preservation, store-address selection,
store-value publication, or another same-boundary backend rule.

## Execution Rules

- Start from the first wrong Hanoi state, not from the old stale reload.
- Compare semantic BIR, prepared BIR, and generated AArch64 around the first
  recursive move and first store/update that places disk `1`.
- Identify whether the wrong peg is introduced by argument/peg selection,
  recursive call state, store address, store value, or another concrete owner
  before editing.
- Add focused backend coverage for the localized owner shape when practical.
- If focused backend coverage is not practical, record why in `todo.md` and use
  stronger generated-code or runtime evidence.
- Keep the idea 363 stale join reload evidence stable.
- Keep the idea 362 index/scale repair evidence stable.
- Keep the idea 361 store writeback evidence present.
- Keep the idea 360 starting-state output correct.
- Treat named-case matching, output-expectation changes, timeout-policy
  changes, or expectation weakening as route failure.

## Step 1: Localize The First Wrong Hanoi Move Boundary

Goal: find the first semantic-BIR, prepared-BIR, or generated-AArch64 boundary
where the expected first move to `C` becomes an actual move to `B`.

Actions:

- Reproduce the focused subset including `00181`, `00170`, `00189`, and the
  backend contracts listed above.
- Inspect `00181.c`, semantic BIR, prepared BIR, and generated AArch64 around
  the first recursive move, peg arguments, and the first tower store/update.
- Record the expected peg identity/state and actual peg identity/state at the
  first divergent boundary.
- Classify whether the owner is argument/peg selection, recursive call state,
  store-address selection, store-value publication, or another concrete rule.
- Confirm the idea 363 stale scan-join reloads remain absent.

Completion check:

- `todo.md` names the first wrong boundary, expected value or peg, actual value
  or peg, producer path, consumer path, and owner classification.
- The stale scan-join reload owner remains outside this step unless fresh
  evidence shows the old reload returned.

## Step 2: Repair The Localized First-Move Owner

Goal: repair the general backend rule responsible for the wrong first move.

Actions:

- Update only the localized owner identified in Step 1.
- Ensure the repair applies to the general owner shape, not just `00181`,
  `Move`, one peg letter, one tower global, one block label, one stack offset,
  or one emitted register.
- Add focused backend coverage for the repaired shape when practical, or record
  why generated-code/runtime evidence is the stronger proof for this owner.
- Preserve the idea 363 stale select/condition join repair.
- Preserve pointer-derived load/address scaling from idea 362.
- Preserve materialized pointer-addressed store writeback from idea 361.
- Preserve direct `LoadGlobal` current-memory select-store handling from idea
  360.

Completion check:

- Focused backend coverage fails before the repair and passes after it when
  practical.
- Generated code or execution evidence shows the first move no longer places
  disk `1` on `B` when expected on `C`.
- `git diff --check` passes.

## Step 3: Prove Focused External Progress

Goal: prove the first-move repair advances `00181` without regressing nearby
repaired paths.

Actions:

- Run:
  `cmake --build --preset default && ctest --test-dir build -j10 --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_contract|backend_prepare_frame_stack_call_contract|backend_cli_dump_prepared_bir_local_arg_call_contract|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00170_c|c_testsuite_aarch64_backend_src_00189_c)$' | tee test_after.log`
- If `00181` still fails, classify the new first bad fact in `todo.md` and
  split it only if it is outside the localized first-move owner.
- Keep `00170`, `00189`, the idea 360 starting-state output, the idea 361
  store writeback evidence, the idea 362 address-scaling evidence, and the idea
  363 stale join reload absence stable.

Completion check:

- The delegated proof command is recorded in `todo.md`.
- `00181` advances beyond the first-move `B`-instead-of-`C` mismatch or has a
  newly localized out-of-scope first bad fact.
- Stability representatives remain passing.

## Step 4: Broader Backend Guard And Lifecycle Decision

Goal: decide whether the first-move residual is complete and safe to close or
whether a new first bad fact needs its own source idea.

Actions:

- Compare the final residual, if any, against the source idea's in-scope and
  out-of-scope lists.
- Ask the supervisor for a broader backend guard before closure when the repair
  touches shared argument, call-state, store, publication, stack-home, or
  AArch64 dispatch behavior.
- Split unrelated residuals into new `ideas/open/` source ideas instead of
  absorbing them into this plan.

Completion check:

- The first-move residual closes only when it is repaired, guarded, and not
  masking a separate owner.
- Any unrelated residual is split into a new source idea.
