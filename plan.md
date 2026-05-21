# AArch64 Hanoi Starting-State Output Mismatch Runbook

Status: Active
Source Idea: ideas/open/360_aarch64_hanoi_starting_state_output_mismatch.md

## Purpose

Repair the AArch64 lowering residual where `00181` prints the wrong initial
Tower of Hanoi source-tower state before the previously repaired recursive
post-call pointer-formal path is observable.

## Goal

Make the initial `00181` output print `A: 1 2 3 4` instead of
`A: 1 2 0 4`, with the fix tied to the localized value-flow rule and prior
recursive-call preservation repairs remaining stable.

## Core Rule

Localize and repair the general value-flow boundary that loses or reads the
third source-tower element as zero. Do not special-case `00181`, `Hanoi`, the
literal value `3`, a tower name, one stack offset, one ABI register, or one
instruction neighborhood.

## Read First

- `ideas/open/360_aarch64_hanoi_starting_state_output_mismatch.md`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `tests/c/external/c-testsuite/src/00181.c`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`
- Recent idea 357 closure notes under
  `ideas/closed/357_aarch64_recursive_pointer_formal_home_publication.md`
- Parked idea 358 notes under
  `ideas/open/358_aarch64_recursive_scalar_formal_post_call_preservation.md`
- Parked idea 359 notes under
  `ideas/open/359_aarch64_recursive_stack_preserved_pointer_formal_post_call_overwrite.md`

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

- Do not reopen stack-preserved pointer formal post-call overwrite handling
  from idea 359.
- Do not reopen scalar formal post-call reloads from idea 358.
- Do not reopen pointer-formal callee-saved home publication from idea 357.
- Do not reopen address-valued memory or call-argument publication from idea 355.
- Do not handle semantic-BIR dynamic pointer-derived string loads for `00173`.
- Do not handle frontend or semantic admission failure for `00005`.
- Do not change expectations, unsupported classifications, runner behavior,
  timeout policy, CTest registration, or proof-log policy.
- Do not broaden into ABI composite/byval/HFA/f128, variadic floating,
  dynamic stack, or unrelated scalar compare/select residuals.

## Working Model

After idea 359, generated `Hanoi` no longer has the bad post-call
`str x3, [sp, #8]` before the final `%p.spare` reload/call sequence. `00181`
now fails earlier and visibly as a starting-state mismatch: expected
`A: 1 2 3 4`, actual `A: 1 2 0 4`. The owner is whatever pre-recursion
initialization, store, reload, or print value-flow boundary first loses that
third source-tower element.

## Execution Rules

- Use generated-code and prepared-BIR evidence to confirm the first bad fact
  before editing.
- Compare semantic BIR, prepared BIR, and generated AArch64 before choosing the
  owner boundary.
- Prefer existing memory, local, GEP, and publication helpers and local
  codegen patterns.
- Add focused backend coverage for the localized value-flow shape once known.
- Keep stack-preserved pointer formal post-call behavior from idea 359 stable.
- Keep scalar-formal post-call reload behavior from idea 358 stable.
- Keep pointer-formal callee-saved home behavior from idea 357 stable.
- Treat named-case matching or expectation weakening as route failure.

## Step 1: Localize The Starting-State Value-Flow Boundary

Goal: find the first boundary where the source-tower element expected to print
as `3` is missing, zeroed, stored incorrectly, reloaded incorrectly, or passed
incorrectly to the print path.

Actions:

- Reproduce the focused subset including `00181`, `00170`, `00189`, and the
  backend contracts listed above.
- Inspect `00181.c`, semantic BIR, prepared BIR, and generated AArch64 for the
  initial tower population and first printed starting-state rows.
- Identify whether the bad `0` is introduced during initialization, stack or
  memory publication, address/index lowering, reload, call argument setup, or
  print-time value movement.
- Record the first bad fact and any adjacent same-shape examples in `todo.md`.

Completion check:

- `todo.md` names the producing source operation, expected value, lowered
  storage or register location, first wrong boundary, and consumer path.
- Prior recursive-call preservation repairs remain outside this step unless
  fresh evidence contradicts the idea split.

## Step 2: Repair The Localized Starting-State Rule

Goal: repair the general lowering rule that causes the initial source-tower
element to become zero before the starting-state print completes.

Actions:

- Update the localized AArch64 lowering boundary only.
- Ensure the repair applies to the general value-flow shape, not just `00181`,
  `Hanoi`, the literal value `3`, or one emitted offset.
- Preserve stack-preserved pointer formal post-call behavior from idea 359.
- Preserve scalar-formal reload behavior from idea 358.
- Preserve pointer-formal callee-saved home publication behavior from idea 357.
- Add focused backend coverage for the repaired value-flow shape.

Completion check:

- Focused backend coverage fails before the repair and passes after it.
- Generated code or execution evidence for the representative no longer loses
  the third source-tower element before recursion.
- `git diff --check` passes.

## Step 3: Prove Focused External Progress

Goal: prove the starting-state repair advances or resolves `00181` without
regressing nearby repaired paths.

Actions:

- Run:
  `cmake --build --preset default && ctest --test-dir build -j10 --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_contract|backend_prepare_frame_stack_call_contract|backend_cli_dump_prepared_bir_local_arg_call_contract|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00170_c|c_testsuite_aarch64_backend_src_00189_c)$' | tee test_after.log`
- If `00181` still fails, classify the new first bad fact in `todo.md` and
  split it only if it is outside the starting-state value-flow owner.
- Keep `00170` and `00189` passing.

Completion check:

- The delegated proof command is recorded in `todo.md`.
- `00181` passes or has a newly localized out-of-scope first bad fact.
- Stability representatives remain passing.

## Step 4: Close Or Split Residual Scope

Goal: decide whether idea 360 is complete or whether a fresh residual belongs
to a separate source idea.

Actions:

- Compare the final residual, if any, against the source idea's in-scope and
  out-of-scope lists.
- Request plan-owner closure only after the starting-state value-flow owner is
  repaired and guarded.
- Before closure, supervisor should run an appropriate broader backend guard
  if the code diff touches shared memory, publication, or call-lowering
  behavior.

Completion check:

- Idea 360 closes only when the starting-state value-flow residual is repaired
  and guarded.
- Any unrelated residual is split into a new `ideas/open/` source idea instead
  of being absorbed into this plan.
