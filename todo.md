# Current Packet

Status: Active
Source Idea Path: ideas/open/120_aarch64_calls_after_call_result_value_local_owner.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extract Local Owner Surface In Place

## Just Finished

Step 2 extracted the after-call result/value emission body into the in-place
AArch64-local `AfterCallMoveLocalOwner::instruction` surface in
`src/backend/mir/aarch64/codegen/calls.cpp`.

Changed files:

- `src/backend/mir/aarch64/codegen/calls.cpp`
- `todo.md`

The `lower_after_call_move` wrapper is now the prepared-input collection
surface. It gathers the destination home, call-boundary classification,
prepared f128 carrier table, and selected destination f128 carrier, then passes
those explicit inputs through `PreparedAfterCallMoveOwnerInputs` to the local
owner. The owner preserves the existing AArch64-local register/result view
selection, f128 q-register spelling, frame-slot store publication, record
construction, diagnostics, and fallback FPR path without recomputing prepared
destination homes, stack frame-slot facts, f128 carriers, or result
classification.

The synthetic register-to-stack result publication path in
`lower_after_call_moves` remains intact and still routes through
`lower_after_call_move`, so synthetic result publication receives the same
prepared-input wrapper and local-owner emission path as bundle moves.

## Suggested Next

Execute Step 3: decide whether to defer or perform a file split for
`AfterCallMoveLocalOwner`. The current extraction is in-place and still depends
on nearby private AArch64 register, f128, memory, and record-construction
helpers.

## Watchouts

- Do not reopen destination-home, stack frame-slot, or f128 carrier authority
  from closed ideas.
- Do not move target-specific ABI register spelling, register-view policy,
  q/f128 rendering, or memory operand spelling into shared prepared code.
- Treat expectation downgrades, unsupported-path rewrites, and named-case-only
  proof as route failures.
- Keep the synthetic register-to-stack result publication path in
  `lower_after_call_moves` intact; it is a real after-call result route, not
  dead compatibility code.
- The f128/vector-carrier register-to-register after-call result coverage gap
  is still present. Current proof covers f128 q-register result publication to
  a frame slot, while register-home after-call result coverage remains strongest
  for GPR and scalar FPR/HFA lanes.

## Proof

Ran exactly:

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_call_boundary_owner|backend_prepare_frame_stack_call_contract|backend_prealloc_call_boundary_classification|backend_aarch64_target_instruction_records)$' >> test_after.log 2>&1`

Result: passed, 5/5 tests.

Proof log: `test_after.log`.

Supervisor regression guard:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
passed with 5/5 before and 5/5 after, no new failures.
