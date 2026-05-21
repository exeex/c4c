# AArch64 Recursive Scalar Formal Post-Call Preservation Runbook

Status: Active
Source Idea: ideas/open/358_aarch64_recursive_scalar_formal_post_call_preservation.md

## Purpose

Repair AArch64 lowering for recursive or same-module paths where a scalar
formal remains live after a call and must be reloaded from its prepared
preserved home instead of reused from a clobbered ABI argument register.

## Goal

Make post-call scalar formal uses publish or reload the prepared preserved home
on the consuming path, with `00181` advancing or passing and prior pointer-home
publication coverage remaining stable.

## Core Rule

Fix the general scalar-formal preserved-home boundary. Do not special-case
`00181`, `Hanoi`, one source formal, one register, one stack offset, or one
emitted instruction neighborhood.

## Read First

- `ideas/open/358_aarch64_recursive_scalar_formal_post_call_preservation.md`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`
- Recent idea 357 closure notes under `ideas/closed/357_aarch64_recursive_pointer_formal_home_publication.md`

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

- Do not reopen pointer-formal callee-saved home publication from idea 357.
- Do not reopen address-valued memory or call-argument publication from idea 355.
- Do not handle semantic-BIR dynamic pointer-derived string loads for `00173`.
- Do not handle frontend or semantic admission failure for `00005`.
- Do not change expectations, unsupported classifications, runner behavior,
  timeout policy, CTest registration, or proof-log policy.
- Do not broaden into ABI composite/byval/HFA/f128, variadic floating,
  dynamic stack, or unrelated scalar compare/select residuals.

## Working Model

The current `00181` residual is past pointer-formal publication. Generated code
now publishes pointer formals into their callee-saved homes, but a later
recursive path computes `n - 1` from clobbered `w0` after calls instead of
reloading `%p.n` from the prepared preserved home. The owner is the scalar
formal preserved-home/post-call reuse boundary.

## Execution Rules

- Use generated-code and prepared-BIR evidence to confirm the first bad fact
  before editing.
- Prefer existing prepared-home/publication helpers and local codegen patterns.
- Add focused backend coverage that proves a scalar formal's ABI register is
  clobbered by an intervening call and the post-call use comes from the
  preserved home.
- Keep pointer-formal callee-saved home behavior from idea 357 stable.
- Treat named-case matching or expectation weakening as route failure.

## Step 1: Localize The Scalar Formal Boundary

Goal: confirm the exact producer, prepared home, and post-call consumer for the
scalar formal residual.

Actions:

- Reproduce the focused subset including `00181`, `00170`, `00189`, and the
  backend contracts listed above.
- Inspect prepared BIR and generated AArch64 for `%p.n` on the recursive path.
- Identify where lowering chooses stale `w0` instead of the prepared preserved
  home after an intervening call.
- Record the first bad fact and any adjacent same-shape examples in `todo.md`.

Completion check:

- `todo.md` names the scalar formal, prepared home, clobbering call boundary,
  and consuming lowered instruction path.
- Pointer-formal publication remains outside this step unless fresh evidence
  contradicts the idea split.

## Step 2: Repair Post-Call Scalar Formal Reload

Goal: make post-call scalar formal consumers reload or publish from the
prepared preserved home when the ABI argument register is no longer reliable.

Actions:

- Update AArch64 lowering at the localized boundary only.
- Ensure the repair applies to scalar formals generally, not just `%p.n` or
  `Hanoi`.
- Preserve existing pointer-formal callee-saved home publication behavior.
- Add focused backend dispatch coverage for a scalar formal consumed after an
  intervening call that clobbers the original ABI register.

Completion check:

- Focused backend coverage fails before the repair and passes after it.
- Generated code for the representative no longer computes the post-call
  scalar use from clobbered `w0`.
- `git diff --check` passes.

## Step 3: Prove Focused External Progress

Goal: prove the scalar formal preservation repair advances or resolves the
external residual without regressing nearby repaired paths.

Actions:

- Run:
  `cmake --build --preset default && ctest --test-dir build -j10 --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_contract|backend_prepare_frame_stack_call_contract|backend_cli_dump_prepared_bir_local_arg_call_contract|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00170_c|c_testsuite_aarch64_backend_src_00189_c)$' | tee test_after.log`
- If `00181` still fails, classify the new first bad fact in `todo.md` and
  split it only if it is outside scalar formal post-call preservation.
- Keep `00170` and `00189` passing.

Completion check:

- The delegated proof command is recorded in `todo.md`.
- `00181` passes or has a newly localized out-of-scope first bad fact.
- Stability representatives remain passing.

## Step 4: Close Or Split Residual Scope

Goal: decide whether idea 358 is complete or whether a fresh residual belongs
to a separate source idea.

Actions:

- Compare the final residual, if any, against the source idea's in-scope and
  out-of-scope lists.
- Request plan-owner closure only after scalar formal post-call preservation is
  proven complete.
- Before closure, supervisor should run an appropriate broader backend guard
  if the code diff touches shared publication or call-lowering behavior.

Completion check:

- Idea 358 closes only when scalar formal post-call preservation is repaired
  and guarded.
- Any unrelated residual is split into a new `ideas/open/` source idea instead
  of being absorbed into this plan.
