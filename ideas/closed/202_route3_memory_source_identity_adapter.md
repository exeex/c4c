# Route 3 memory/source identity adapter

## Goal

Migrate one selected memory/source identity reader to Route 3 BIR records while
retaining prepared target-addressing fallback for policy-sensitive behavior.

## Why This Exists

The Phase B2 readiness audit classifies Route 3 schema as sufficient for
selected memory/source identity, but not for frame/global/TLS address
formation, addressing-mode legality, final operands, materialization, or other
target-addressing policy. Idea 190 also showed that prepared target-addressing
fallback remains required for non-regressive AArch64 behavior.

This idea is the bounded follow-up for the Phase B2 Route 3 candidate:
selected memory/source identity consumer, with idea 190 fallback discipline.

## In Scope

- Select exactly one direct memory/source identity reader.
- Use Route 3 records or indexes for memory access id, result or stored-value
  identity, semantic base/source identity, and block/instruction compatibility.
- Preserve prepared fallback for target-addressing, address formation,
  materialization, and policy-sensitive cases.
- Add or update proof for absent Route 3 facts, mismatched route/prepared
  facts, non-memory cases, alias/address ambiguity, and fallback behavior.
- Preserve baseline and string-authority discipline from ideas 190 and 199.

## Out Of Scope

- Replacing all `memory_accesses` or `PreparedMemoryAccessLookups`.
- Moving frame offsets, relocation spelling, concrete layout, addressing-mode
  legality, volatile/address-space lowering policy, materialization, or final
  operands into BIR schema.
- Retiring prepared memory/frame/stack or target-addressing oracles.
- Refreshing expected output or baselines as proof of ownership.

## Acceptance Criteria

- The selected reader obtains only Route 3 semantic identity from BIR.
- Target-addressing and policy-sensitive behavior still falls back to prepared
  lookup authority.
- Tests or route proof cover success, absence, mismatch, non-memory, ambiguity,
  prepared fallback, and non-regressive output/baseline behavior.
- The implementation names any retained prepared oracle or fallback that is
  still required after the adapter lands.

## Completion Notes

Closed after the selected Route 3 load-local adapter landed. The selected
reader now obtains semantic memory/source identity from BIR Route 3 while
retaining `PreparedMemoryAccessLookups` and prepared load-local source-home
fallback for address formation, materialization, addressing-mode legality,
final operands, and policy-sensitive source-home behavior.

Close proof used matching backend logs from:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > <log>`

`test_before.log` and `test_after.log` both recorded 180 passed, 0 failed. The
close-time regression guard passed in non-decreasing mode with no new failures
because this closure slice is lifecycle-only and the implementation validation
had already been committed.

Additional Route 3 readers remain intentionally out of scope for this idea and
should be opened as separate follow-up ideas if needed.

## Reviewer Reject Signals

- Reject a slice that moves address formation, frame/global/TLS relocation,
  stack/frame offsets, concrete layout, addressing-mode legality,
  materialization, final operands, or target-addressing fallback into BIR
  schema.
- Reject broad replacement of `memory_accesses`,
  `PreparedMemoryAccessLookups`, or memory/frame/stack helper groups.
- Reject happy-path-only proof without absent, mismatch, non-memory, ambiguity,
  and fallback coverage.
- Reject baseline refreshes, expected-string rewrites, unsupported downgrades,
  or guard weakening used as ownership evidence.
- Reject any abstraction rename that leaves the same prepared-only failure mode
  behind a new Route 3 wrapper.
