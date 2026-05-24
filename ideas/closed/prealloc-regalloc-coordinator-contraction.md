# Prealloc Regalloc Coordinator Contraction

## Goal

Contract `regalloc.cpp` coordinator internals along existing
`src/backend/prealloc/regalloc/` helper-family boundaries while keeping
allocation behavior unchanged.

## Why This Exists

The responsibility map treats liveness and allocation planning as a durable
family. `regalloc.cpp` remains a high-value contraction target, but the package
already has helper files under `regalloc/`; cleanup should strengthen those
boundaries instead of inventing a new allocator architecture.

## Target Files

- `src/backend/prealloc/regalloc.cpp`
- `src/backend/prealloc/regalloc.hpp`
- `src/backend/prealloc/regalloc/`
- `src/backend/prealloc/regalloc_placement_identity.cpp`
- `src/backend/prealloc/regalloc_placement_identity.hpp`
- `src/backend/prealloc/liveness.cpp`
- `src/backend/prealloc/liveness.hpp`
- `src/backend/prealloc/value_locations.hpp`
- `src/backend/prealloc/prepared_printer/regalloc.cpp`
- `src/backend/prealloc/prepared_printer/value_locations.cpp`

## Slice Type

`.cpp` phase contraction and helper relocation or renaming.

## Durable Owner

Liveness and allocation planning.

## In Scope

- Extract or relocate coordinator internals only when they align with existing
  `regalloc/` helper families.
- Rename helper functions so placement, spill/reload, interval, and ABI fact
  responsibilities are visible.
- Keep `regalloc.hpp` as the aggregate public allocation-plan contract until
  consumers prove smaller stable contracts.
- Preserve prepared-printer alignment for any renamed allocation-plan concepts.

## Out Of Scope

- Changing register allocation decisions, spill/reload behavior, interval
  construction, value homes, call-return ABI facts, or liveness semantics.
- Replacing the allocator architecture.
- Splitting `regalloc.hpp` before implementation helper boundaries are stable.
- Moving concrete target register profile ownership out of
  `target_register_profile.*`.

## Expected Behavior-Preservation Proof

Run:

```sh
bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } 2>&1 | tee test_after.log'
```

Also run `git diff --check`.

## Acceptance Criteria

- `regalloc.cpp` exposes less unrelated coordinator detail or has clearer
  internal helper boundaries.
- Extracted helpers do not depend on broad mutable coordinator state through
  unclear APIs.
- Backend tests remain green and allocation-plan dump meaning is preserved.

## Closure Note

Closed after the active runbook completed two behavior-preserving helper
contractions: assignment expiry now lives under `regalloc/assignment.*`, and
stack-slot frame seed/publication helpers now live under
`regalloc/stack_slots.*`. Closure audit found no allocation, spill/reload,
interval, liveness, ABI, public contract, dump-meaning, or testcase-overfit
drift. Remaining possible call-ABI binding, prepared value-location bundle, or
`regalloc.hpp` fragmentation work is a separate initiative, not leftover scope
for this idea.

## Reviewer Reject Signals

- Reject extracted helpers that require wide access to private mutable
  coordinator state without a clearer API.
- Reject behavior changes to allocation, spill/reload, interval, liveness, or
  call-return ABI facts under a contraction label.
- Reject target-shaped shortcuts or named-case allocation rules.
- Reject `regalloc.hpp` fragmentation that forces many includes for one
  allocation plan.
- Reject renames that mask concrete target policy that belongs in
  `target_register_profile.*`.
