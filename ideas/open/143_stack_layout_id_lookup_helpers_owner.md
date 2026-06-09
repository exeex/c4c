# 143 Stack-layout id lookup helpers owner

## Goal

Move stack-layout id lookup helpers from `prepared_lookups.*` to stack-layout
ownership.

## Why This Exists

Idea 141 found that `find_frame_slot_by_id` and `find_stack_object_by_id` are
simple scans over `PreparedStackLayout`. Their semantic owner is stack layout,
not the residual prepared lookup facade.

## In Scope

- Move declarations for `find_frame_slot_by_id` and `find_stack_object_by_id`
  to `src/backend/prealloc/stack_layout/stack_layout.hpp`.
- Move implementations to the narrowest existing stack-layout source file, or
  introduce one only if the repo pattern supports it.
- Update consumers to include the stack-layout owner.

## Out Of Scope

- Rewriting stack layout construction.
- Changing frame-address, addressing, memory retargeting, or comparison
  semantics.
- Moving AArch64 frame-address wrapper policy into prealloc.

## Acceptance Criteria

- Stack id helper declarations no longer require `prepared_lookups.hpp`.
- Behavior is unchanged and remains target-independent.
- Proof includes `cmake --build --preset default` and
  `ctest --test-dir build -R '^backend_' --output-on-failure`.

## Reviewer Reject Signals

- Helper movement causes broader prepared lookup dependency churn without
  reducing the facade.
- AArch64 wrapper behavior changes instead of only moving ownership.
- Stack id lookup becomes target-specific.
