# Current Packet

Status: Active
Source Idea Path: ideas/open/119_aarch64_calls_before_call_move_bundle_local_owner.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Split File Boundary Only If The Owner Is Stable

## Just Finished

Step 3 completed: reviewed the in-place
`BeforeCallMoveLocalOwner::instruction` boundary and deferred a separate helper
file split.

Changed files:

- `todo.md`

No code file was changed. The extracted owner is stable as an in-place
AArch64-local boundary: the wrapper remains the prepared-input collection
surface, and the owner consumes only the explicit
`PreparedBeforeCallMoveOwnerInputs` plus the prepared call/move/effect records.
However, a file split would currently require exporting or relocating a broad
private target-local helper surface, including stack-frame call operand helpers,
f128 carrier helpers, immediate scalar publication, byval source/destination
materialization, register-authority construction, diagnostics, and
machine-record construction. That declaration churn would increase coupling and
expand the packet beyond a narrow boundary split.

## Suggested Next

Execute Step 4 by proving neighboring before-call routes across register,
stack, immediate, FP/f128, byval, and outgoing-stack-area cases. Treat the
deferred file split as intentional unless the proof pass exposes a smaller,
more stable target-local helper boundary.

## Watchouts

- `lower_before_call_immediate_binding` remains adjacent and intentionally
  outside this owner extraction; immediate publication was not moved.
- The wrapper still computes the prepared-input bundle once per move, including
  `outgoing_stack_argument_bytes(call_plan)`, so future packets should avoid
  reintroducing that derivation inside `BeforeCallMoveLocalOwner`.
- The in-place owner still depends on multiple private AArch64 helpers in
  `calls.cpp`; moving it before those helper boundaries are smaller would add a
  broad local declaration layer without improving prepared/shared ownership.

## Proof

No build or test was required for this packet because no code changed and the
Step 3 decision was to defer the file split. The existing `test_after.log` from
Step 2 was not regenerated.
