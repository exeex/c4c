# Current Packet

Status: Active
Source Idea Path: ideas/open/119_aarch64_calls_before_call_move_bundle_local_owner.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extract Local Owner Surface In Place

## Just Finished

Step 2 completed: extracted the before-call move translation body into an
in-place AArch64-local `BeforeCallMoveLocalOwner::instruction` surface in
`src/backend/mir/aarch64/codegen/calls.cpp`.

Changed files:

- `src/backend/mir/aarch64/codegen/calls.cpp`
- `todo.md`

The public `lower_before_call_move` wrapper now gathers the prepared inputs and
passes them through `PreparedBeforeCallMoveOwnerInputs`: indexed prepared source
home, call-boundary classification, prepared f128 carrier facts, selected
source f128 carrier, and prepared outgoing-stack argument byte count. The local
owner consumes those explicit facts and keeps AArch64-specific register,
memory, machine-record, diagnostic, and stack-reservation emission decisions
local.

## Suggested Next

Execute Step 3 by reviewing the extracted owner boundary against the source
idea: confirm no shared prepared authority moved into AArch64 code, no
testcase-shaped matching was introduced, and the remaining wrapper lookups are
only the intended prepared-input collection surface.

## Watchouts

- `lower_before_call_immediate_binding` remains adjacent and intentionally
  outside this owner extraction; immediate publication was not moved.
- The wrapper still computes the prepared-input bundle once per move, including
  `outgoing_stack_argument_bytes(call_plan)`, so future packets should avoid
  reintroducing that derivation inside `BeforeCallMoveLocalOwner`.
- AST checks before the edit showed the only direct caller of
  `lower_before_call_move` was `lower_before_call_moves`; no header declaration
  change was needed.

## Proof

Ran exactly:

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_return_lowering|backend_prepare_frame_stack_call_contract|backend_prealloc_call_boundary_classification|backend_aarch64_target_instruction_records|backend_aarch64_call_boundary_owner)$' >> test_after.log 2>&1`

Result: passed. Build completed and the targeted CTest subset passed 6/6.
Proof log: `test_after.log`.

Supervisor regression guard:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
passed with 6/6 before and 6/6 after, no new failures. The default strict
increase mode was not used for acceptance because this packet is a
behavior-preserving refactor of an existing covered path, not a test-addition
slice.
