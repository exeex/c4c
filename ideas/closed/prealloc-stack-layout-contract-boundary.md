# Prealloc Stack Layout Contract Boundary

## Goal

Clarify the stack-layout public boundary and split coordinator internals only
where existing responsibilities already separate cleanly.

## Why This Exists

The responsibility map identifies stack and frame planning as a durable
prealloc family. Some stack-layout-facing declarations are still exposed
through aggregate module surfaces, and `stack_layout/coordinator.cpp` combines
object collection, slot assignment, orchestration, and address-publication
concerns.

## Target Files

- `src/backend/prealloc/module.hpp`
- `src/backend/prealloc/frame.hpp`
- `src/backend/prealloc/frame_plan.cpp`
- `src/backend/prealloc/frame_plan.hpp`
- `src/backend/prealloc/stack_layout/coordinator.cpp`
- `src/backend/prealloc/stack_layout/alloca_coalescing.cpp`
- `src/backend/prealloc/stack_layout/analysis.cpp`
- `src/backend/prealloc/stack_layout/copy_coalescing.cpp`
- `src/backend/prealloc/stack_layout/inline_asm.cpp`
- `src/backend/prealloc/stack_layout/regalloc_helpers.cpp`
- `src/backend/prealloc/stack_layout/slot_assignment.cpp`
- `src/backend/prealloc/stack_layout/stack_layout.hpp`
- `src/backend/prealloc/prepared_printer/frame.cpp`

Potential newly-created split files, if the coordinator boundary proves clear,
should be described in the implementation plan before creation. Names such as
`objects.*`, `slots.*`, or additional stack-layout implementation files are
possible outputs of the slice, not current target files.

## Slice Type

Header/data-contract boundary cleanup plus focused `.cpp` phase split.

## Durable Owner

Stack and frame planning.

## In Scope

- Move or document stack-layout-facing declarations currently exposed through
  `module.hpp` only when call sites can depend directly on a stack-layout
  contract.
- Split `stack_layout/coordinator.cpp` along object collection, slot assignment
  orchestration, and address-materialization/publication boundaries when those
  APIs are clear.
- Preserve `frame.hpp` and `frame_plan.hpp` as public frame contracts.
- Keep prepared-printer frame output aligned with any data-family naming
  changes.

## Out Of Scope

- Changing stack object authority, frame size, slot assignment, alignment,
  inline-asm stack facts, or dynamic address semantics.
- Moving final target instruction emission, register spelling, or target
  assembly address formation into prealloc.
- Creating a forwarding header while `module.hpp` remains the real dependency.
- Broad unrelated include rewrites outside stack/frame consumers.

## Expected Behavior-Preservation Proof

Run:

```sh
bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } 2>&1 | tee test_after.log'
```

Also run `git diff --check`.

## Acceptance Criteria

- Stack-layout ownership is visible from the relevant contract files and helper
  names.
- `module.hpp` either remains intentionally aggregate or sheds stack-layout
  declarations that direct consumers can include elsewhere.
- Backend tests remain green and prepared frame dump meaning is preserved.

## Reviewer Reject Signals

- Reject a new header that is only a forwarding shell while `module.hpp`
  remains the actual dependency.
- Reject fallback address cleanup that becomes target-emission logic or
  duplicates stack object authority.
- Reject changes to stack layout, frame sizing, slot assignment, or address
  semantics claimed as contract cleanup.
- Reject broad include churn not tied to stack/frame ownership.
- Reject printer changes that hide or drop frame/stack facts.

## Closure Notes

Closed after behavior-preserving stack-layout boundary cleanup:

- stack-layout phase hook declarations and `FunctionInlineAsmSummary` moved
  from aggregate `module.hpp` declarations to `stack_layout/stack_layout.hpp`
  while keeping `module.hpp` as an intentional aggregate include for existing
  consumers
- `stack_layout/coordinator.cpp` now has file-local object-planning and
  addressing-publication helpers for the clear coordinator boundaries
- prepared frame printing was audited and did not need terminology changes
  because public frame/stack data-family names and field meanings were
  preserved

Deferred follow-up candidates, if still useful, should remain separate open
ideas: deeper slot-assignment orchestration decomposition and broader
frame-plan naming cleanup.
