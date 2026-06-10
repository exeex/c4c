# 148 Same-block load-local stored-value owner

## Goal

Move same-block load-local stored-value source lookup toward addressing or
memory ownership.

## Why This Exists

Idea 141 found that `PreparedSameBlockLoadLocalStoredValueSource` crosses
same-block source producers, memory-access lookups, addressing, and stack
layout. It should move toward addressing/memory ownership with dependencies on
stack-layout and same-block source-producer owners.

## In Scope

- Move `PreparedSameBlockLoadLocalStoredValueSource`.
- Move `find_prepared_same_block_load_local_stored_value_source`.
- Use `src/backend/prealloc/addressing.hpp` and the existing addressing
  implementation owner selected by the plan.
- Depend on stack-layout ownership from idea 143 and same-block
  source-producer ownership from idea 144.

## Out Of Scope

- Rewriting prepared memory-access construction.
- Moving AArch64 load/store emission, register view, scratch, or extension
  handling into prealloc.
- Special-casing the current AArch64 call consumer.
- Absorbing unrelated same-block materialization APIs into addressing.

## Acceptance Criteria

- The load-local stored-value API has a memory/addressing owner and reuses
  prepared facts rather than duplicating scans.
- Behavior remains semantic and not shaped around one AArch64 call-site case.
- Proof includes `cmake --build --preset default` and
  `ctest --test-dir build -R '^backend_' --output-on-failure`.
- Escalate to full CTest if shared memory-access semantics change.

## Reviewer Reject Signals

- The move duplicates source-producer or stack-layout scans instead of using
  prepared facts.
- The implementation is shaped around only the AArch64 call-site case.
- Addressing ownership is used to absorb unrelated same-block materialization
  APIs.

## Completion Note

Closed on 2026-06-09 after the active runbook completed Step 4 and validation
was committed through `263575573`.

- `PreparedSameBlockLoadLocalStoredValueSource` and
  `find_prepared_same_block_load_local_stored_value_source` are declared from
  `src/backend/prealloc/addressing.hpp`.
- The direct AArch64 consumer includes the addressing owner header.
- The finder continues to reuse prepared source-producer, stack-layout, and
  memory-access facts; no duplicate scan or named call-site shortcut was
  introduced.
- Close-time backend regression guard passed with 179/179 backend tests before
  and after. The strict guard had no new failures but no pass-count increase;
  the non-decreasing maintenance policy was used for this lifecycle closeout.
- No separate residual follow-up was created. The remaining implementation body
  location is treated as the existing prepared lookup implementation owner, not
  a separate initiative required for this source idea.
