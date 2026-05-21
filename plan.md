# AArch64 Prepared Select Condition Join Stale Reload Runbook

Status: Active
Source Idea: ideas/open/363_aarch64_prepared_select_condition_join_stale_reload.md

## Purpose

Repair the AArch64 residual exposed after pointer-derived load/address scaling
was fixed: prepared false/zero scan exits can be overwritten by stale stack
reloads at a join.

## Goal

Make prepared select or condition publication preserve explicit false/zero edge
values across joins so `00181` advances beyond the current fast segmentation
fault without reopening the repaired address-scaling path.

## Core Rule

Localize and repair the general prepared select/condition join publication
boundary. Do not special-case `00181`, `Move`, Hanoi tower globals, block
labels, stack offsets, ABI registers, or one emitted instruction neighborhood.

## Read First

- `ideas/open/363_aarch64_prepared_select_condition_join_stale_reload.md`
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

- Do not reopen pointer-derived load/address scaling from idea 362 except to
  keep it stable.
- Do not reopen materialized pointer-addressed store writeback from idea 361
  except to keep it stable.
- Do not reopen the idea 360 direct `LoadGlobal` current-memory select-store
  repair except to keep it stable.
- Do not reopen recursive formal post-call repairs from ideas 357, 358, and
  359.
- Do not handle semantic-BIR dynamic pointer-derived string loads for `00173`.
- Do not handle frontend or semantic admission failure for `00005`.
- Do not change expectations, unsupported classifications, runner behavior,
  timeout policy, CTest registration, or proof-log policy.
- Do not broaden into ABI composite/byval/HFA/f128, variadic floating, dynamic
  stack, or unrelated load-address scaling residuals.

## Working Model

Commit `321031ce0` repaired immediate-scale multiply publication so generated
`Move` keeps the index and scale in distinct registers. `00181` now advances
from the old 5 second timeout to a fast runtime nonzero segmentation fault.
The new first bad fact is at a prepared select/condition join: the false scan
exit materializes `mov x13, #0`, but the join reloads a stale stack slot such
as `ldr x13, [sp, #64]`, overwriting the false value. The destination scan
shows the same shape.

## Execution Rules

- Use generated-code and prepared-BIR evidence to confirm the first stale join
  reload before editing.
- Compare semantic BIR, prepared BIR, and generated AArch64 before choosing the
  owner boundary.
- Prefer existing select, condition, local publication, stack home, and
  join-value helper patterns.
- Add focused backend coverage for the localized select/condition join shape
  before claiming capability progress.
- Keep the idea 362 index/scale repair evidence stable.
- Keep the idea 361 store writeback evidence present.
- Keep the idea 360 starting-state output correct.
- Treat named-case matching, timeout-policy changes, or expectation weakening
  as route failure.

## Step 1: Localize The Stale Join Reload Boundary

Goal: find the first prepared select or condition publication operation that
lets a false/zero edge value be overwritten by a stale stack reload.

Actions:

- Reproduce the focused subset including `00181`, `00170`, `00189`, and the
  backend contracts listed above.
- Inspect `00181.c`, semantic BIR, prepared BIR, and generated AArch64 around
  the source and destination scan joins in `Move`.
- Identify the false-edge producer, expected join-visible value, stale stack
  slot, emitted reload, and consuming path.
- Record the first bad fact and any adjacent same-shape examples in `todo.md`.

Completion check:

- `todo.md` names the source select/condition operation, false-edge producer,
  expected join value, stale reload, first wrong boundary, and consumer path.
- The idea 362 address-scaling repair remains outside this step unless fresh
  evidence contradicts the split.

## Step 2: Repair The Prepared Select Condition Join Rule

Goal: repair the general AArch64 prepared select/condition publication rule
that lets stale stack reloads override explicit false/zero edge values.

Actions:

- Update only the localized select/condition join publication boundary.
- Ensure the repair applies to the general join-visible value shape, not just
  `00181`, one stack offset, or one emitted register.
- Preserve pointer-derived load/address scaling from idea 362.
- Preserve materialized pointer-addressed store writeback from idea 361.
- Preserve direct `LoadGlobal` current-memory select-store handling from idea
  360.
- Add focused backend coverage for the repaired join publication shape.

Completion check:

- Focused backend coverage fails before the repair and passes after it.
- Generated code or execution evidence shows false/zero scan exits keep their
  expected join-visible value.
- `git diff --check` passes.

## Step 3: Prove Focused External Progress

Goal: prove the stale join reload repair advances `00181` without regressing
nearby repaired paths.

Actions:

- Run:
  `cmake --build --preset default && ctest --test-dir build -j10 --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_contract|backend_prepare_frame_stack_call_contract|backend_cli_dump_prepared_bir_local_arg_call_contract|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00170_c|c_testsuite_aarch64_backend_src_00189_c)$' | tee test_after.log`
- If `00181` still fails, classify the new first bad fact in `todo.md` and
  split it only if it is outside the prepared select/condition join owner.
- Keep `00170`, `00189`, the idea 360 starting-state output, the idea 361
  store writeback evidence, and the idea 362 address-scaling evidence stable.

Completion check:

- The delegated proof command is recorded in `todo.md`.
- `00181` advances beyond the fast segmentation fault or has a newly localized
  out-of-scope first bad fact.
- Stability representatives remain passing.

## Step 4: Broader Backend Guard And Lifecycle Decision

Goal: decide whether the residual is complete and safe to close or whether a
new first bad fact needs its own source idea.

Actions:

- Compare the final residual, if any, against the source idea's in-scope and
  out-of-scope lists.
- Ask the supervisor for a broader backend guard before closure because the
  repair may touch shared select, condition, local publication, or stack-home
  behavior.
- Split unrelated residuals into new `ideas/open/` source ideas instead of
  absorbing them into this plan.

Completion check:

- The prepared select/condition join residual closes only when it is repaired,
  guarded, and not masking a separate owner.
- Any unrelated residual is split into a new source idea.
