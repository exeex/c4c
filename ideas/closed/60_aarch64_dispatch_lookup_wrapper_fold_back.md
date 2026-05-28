# AArch64 Dispatch Lookup Wrapper Fold-Back

Ownership class: mechanical fold-back

## Goal

Fold thin AArch64 dispatch lookup and producer convenience wrappers back into
their direct prepared, BIR query, or publication-query owners where they no
longer carry target-specific policy.

## Why This Exists

Idea 59 classified several `dispatch_lookup.*`, `dispatch_producers.*`, and
publication result helpers as wrapper layers over existing prepared/shared
queries. Keeping those wrappers as standalone dispatch-family surface area makes
the AArch64 route look like it owns semantic lookup policy even when the real
authority is already elsewhere.

## In Scope

- `src/backend/mir/aarch64/codegen/dispatch_lookup.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_lookup.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_producers.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
- Thin prepared lookup wrappers including `prepared_named_value_id`,
  `prepared_value_id`, and `find_value_home`.
- Producer wrappers over `mir::query`, including
  `find_same_block_binary_producer`, `find_same_block_named_producer`, and
  `evaluate_same_block_integer_constant`.
- Publication result-value helpers including `instruction_result_value` and
  `instruction_result_value_ref`.

## Out Of Scope

- Moving semantic lookup, producer, or publication authority into AArch64 code.
- Changing recursive value materialization decisions.
- Changing edge-copy fallback behavior, select-chain discovery, or join-copy
  source reconstruction.
- Broadly merging unrelated dispatch files into `dispatch.cpp`.

## Acceptance Criteria

- Call sites use direct prepared/shared query surfaces or local inline
  convenience only where no target-specific policy is introduced.
- Any removed public dispatch lookup or producer declarations are proven unused
  or replaced by the correct owner API.
- Existing behavior for prepared value-home lookup, publication result
  extraction, comparison/FP materialization hooks, and dispatch hook consumers is
  preserved.
- The proving scope includes a build plus focused AArch64 backend coverage for
  the affected lookup and publication-result paths. Escalate to a broader
  regression guard if public headers are removed.

## Closure Note

Closed after the active runbook removed the mechanical public dispatch lookup,
producer, and publication-result forwarding helpers from the AArch64
dispatch-family surface. Remaining dispatch lookup exports were explicitly
identified as carrying AArch64 register/home or scalar-producer policy and are
not part of this mechanical fold-back idea.

Close proof:
- Backend guard: `test_before.log` vs `test_after.log`, both 169/169 passing,
  accepted with non-decreasing mode for this mechanical refactor slice.
- Full after-check: `/tmp/c4c_step5_full_ctest.log`, 3417/3417 passing.

## Reviewer Reject Signals

- New semantic lookup policy appears in AArch64 dispatch code while claiming
  mechanical fold-back.
- Call sites start reimplementing raw BIR scans instead of using prepared or
  shared query authority.
- Wrapper deletion requires unsupported expectation downgrades or weaker test
  contracts.
- Folded helpers become target-specific under a different name without evidence
  that AArch64 owns the behavior.
- The change is mostly helper renames or classification text while leaving the
  same old dispatch-family wrapper authority in place.
