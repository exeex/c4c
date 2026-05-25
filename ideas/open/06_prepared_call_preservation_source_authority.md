# Prepared Call Preservation Source Authority

## Goal

Move authority for selecting prior preserved call values into shared prepared
lookups and call-plan records.

## Why This Exists

AArch64 `calls_moves.cpp` currently participates in deciding which prior
preserved value feeds a later call argument. That decision belongs to prepared
planning, where function-wide call order, block order, domination, and
preservation routes are already known.

## In Scope

- Define prepared lookup helpers for prior-preservation source queries.
- Replace target-local scans with shared prepared query results.
- Preserve indexed lookup behavior for same-block and cross-block cases.
- Make call-plan source selections reference the chosen preservation source
  explicitly.
- Add tests for same-block, cross-block, and no-valid-prior-source cases.

## Out Of Scope

- Changing AAPCS64 preservation policy.
- Mechanical calls file merging.
- Dispatch publication/edge-copy cleanup.
- Fixing unrelated c_testsuite families.

## Acceptance Criteria

- Target-local AArch64 code no longer owns prior-preserved-value selection.
- Prepared lookups expose a single, tested source of truth.
- Existing prior-preservation runtime cases remain green.
- Incomplete or ambiguous preservation choices are rejected before emission.

## Reviewer Reject Signals

- A patch moves the scan into a differently named AArch64 helper.
- A patch ignores domination/block-order constraints.
- A patch accepts ambiguous prior preserved values without diagnostics or a
  stable tie-breaking contract.
- A patch weakens tests to hide missing prepared lookup coverage.
