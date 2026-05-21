# AArch64 Recursive Stack-Preserved Pointer Formal Post-Call Overwrite Runbook

Status: Active
Source Idea: ideas/open/359_aarch64_recursive_stack_preserved_pointer_formal_post_call_overwrite.md

## Purpose

Repair AArch64 lowering for recursive or same-module paths where a pointer
formal's stack-preserved home remains live after a call and must not be
overwritten from a clobbered ABI argument register before later recursive or
helper-call consumers reload it.

## Goal

Make post-call stack-preserved pointer formal handling keep the prepared home
valid across clobbering calls, with `00181` advancing or passing and prior
scalar-formal and callee-saved pointer-formal preservation coverage remaining
stable.

## Core Rule

Fix the general stack-preserved pointer formal post-call boundary. Do not
special-case `00181`, `Hanoi`, `Move`, `%p.spare`, one register, one stack
offset, or one emitted instruction neighborhood.

## Read First

- `ideas/open/359_aarch64_recursive_stack_preserved_pointer_formal_post_call_overwrite.md`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`
- Recent idea 357 closure notes under `ideas/closed/357_aarch64_recursive_pointer_formal_home_publication.md`
- Parked idea 358 notes under `ideas/open/358_aarch64_recursive_scalar_formal_post_call_preservation.md`

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

The current `00181` residual is past both pointer-formal callee-saved home
publication and scalar `%p.n` post-call reload. Generated code now preserves
and reloads `%p.n` correctly, but a later recursive path overwrites the
stack-preserved pointer formal `%p.spare` from clobbered `x3` after a call and
then reloads that corrupted stack slot for a later recursive call. The owner is
the stack-preserved pointer formal post-call overwrite/reload boundary.

## Execution Rules

- Use generated-code and prepared-BIR evidence to confirm the first bad fact
  before editing.
- Prefer existing prepared-home/publication helpers and local codegen patterns.
- Add focused backend coverage that proves a stack-preserved pointer formal's
  ABI register is clobbered by an intervening call and later consumers use the
  valid preserved home, not a republished clobbered value.
- Keep scalar-formal post-call reload behavior from idea 358 stable.
- Keep pointer-formal callee-saved home behavior from idea 357 stable.
- Treat named-case matching or expectation weakening as route failure.

## Step 1: Localize The Stack-Preserved Pointer Formal Boundary

Goal: confirm the exact producer, prepared stack home, clobbering call, and
later consumer for the pointer formal residual.

Actions:

- Reproduce the focused subset including `00181`, `00170`, `00189`, and the
  backend contracts listed above.
- Inspect prepared BIR and generated AArch64 for `%p.spare` on the recursive
  path.
- Identify where lowering overwrites the stack-preserved home from stale `x3`
  after an intervening call.
- Record the first bad fact and any adjacent same-shape examples in `todo.md`.

Completion check:

- `todo.md` names the pointer formal, prepared stack home, clobbering call
  boundary, overwrite site, and consuming lowered call path.
- Scalar `%p.n` reloads and callee-saved pointer homes remain outside this step
  unless fresh evidence contradicts the idea split.

## Step 2: Repair Stack-Preserved Pointer Formal Post-Call Handling

Goal: prevent stack-preserved pointer formal homes from being overwritten from
clobbered ABI argument registers after calls, and make later consumers reload
the valid preserved home.

Actions:

- Update AArch64 lowering at the localized boundary only.
- Ensure the repair applies to stack-preserved pointer formals generally, not
  just `%p.spare`, `Hanoi`, or `Move`.
- Preserve existing scalar-formal reload behavior.
- Preserve existing pointer-formal callee-saved home publication behavior.
- Add focused backend dispatch coverage for a stack-preserved pointer formal
  consumed after an intervening call that clobbers the original ABI register.

Completion check:

- Focused backend coverage fails before the repair and passes after it.
- Generated code for the representative no longer overwrites `%p.spare`'s
  stack home from clobbered `x3` after a call.
- `git diff --check` passes.

## Step 3: Prove Focused External Progress

Goal: prove the stack-preserved pointer formal repair advances or resolves the
external residual without regressing nearby repaired paths.

Actions:

- Run:
  `cmake --build --preset default && ctest --test-dir build -j10 --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_contract|backend_prepare_frame_stack_call_contract|backend_cli_dump_prepared_bir_local_arg_call_contract|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00170_c|c_testsuite_aarch64_backend_src_00189_c)$' | tee test_after.log`
- If `00181` still fails, classify the new first bad fact in `todo.md` and
  split it only if it is outside stack-preserved pointer formal post-call
  handling.
- Keep `00170` and `00189` passing.

Completion check:

- The delegated proof command is recorded in `todo.md`.
- `00181` passes or has a newly localized out-of-scope first bad fact.
- Stability representatives remain passing.

## Step 4: Close Or Split Residual Scope

Goal: decide whether idea 359 is complete or whether a fresh residual belongs
to a separate source idea.

Actions:

- Compare the final residual, if any, against the source idea's in-scope and
  out-of-scope lists.
- Request plan-owner closure only after stack-preserved pointer formal
  post-call handling is proven complete.
- Before closure, supervisor should run an appropriate broader backend guard
  if the code diff touches shared publication or call-lowering behavior.

Completion check:

- Idea 359 closes only when stack-preserved pointer formal post-call handling
  is repaired and guarded.
- Any unrelated residual is split into a new `ideas/open/` source idea instead
  of being absorbed into this plan.
