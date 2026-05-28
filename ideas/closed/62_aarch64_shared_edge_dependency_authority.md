# AArch64 Shared Edge Dependency Authority

Ownership class: shared-authority migration

## Goal

Dispose of AArch64 edge-copy null-publication fallback producer discovery by
moving predecessor/successor edge source facts to shared prepared edge
dependency or publication-query authority.

## Why This Exists

Idea 59 classified the edge-copy fallback producer group as a remaining
duplicate semantic authority path. Edge copy emission, scratch registers,
hazard ordering, and prepared edge-publication consumption are target-local, but
fallback discovery of missing publication sources should not live in AArch64
dispatch.

## In Scope

- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`
  `find_edge_named_producer` and null-publication callers.
- Prepared edge-publication consumers in the AArch64 edge copy route.
- The shared edge dependency or prepared publication query owner that should
  expose successor, edge, and predecessor source facts.

## Out Of Scope

- Moving clobber-sensitive copy ordering, scratch selection, va-list carrier
  emission, or target instruction spelling to shared code.
- Reworking non-edge select-chain discovery.
- Rewriting branch-fusion or before-return publication sequencing.
- Deleting fallback code without proving its callers are covered by prepared
  edge facts.

## Acceptance Criteria

- Edge dependency source identity comes from shared/prepared authority rather
  than AArch64 predecessor-depth scans.
- AArch64 edge emission keeps local responsibility for scratch, hazards,
  clobbers, va-list carrier emission, and machine instruction ordering.
- Missing publication facts have a deliberate fail-closed behavior or diagnostic
  instead of null-publication fallback recovery.
- Focused proof covers prepared edge sources, missing publication facts,
  va-list carrier emission, predecessor/successor edge inputs, and
  clobber-sensitive copy ordering.

## Reviewer Reject Signals

- Predecessor-depth scans survive as durable AArch64 authority.
- Null-publication fallback deletion lacks caller proof.
- Clobber or scratch behavior regresses while moving semantic authority.
- Unsupported or error expectations are downgraded instead of proving prepared
  edge facts.
- The implementation claims shared migration while only renaming the fallback
  producer helper.

## Closure

Closed after the active runbook completed Step 5. The final audit recorded in
`todo.md` found that `find_edge_named_producer(...)` and
`unique_branch_predecessor_context(...)` were removed from the AArch64
edge-copy route without replacement by a renamed predecessor scan, named-case
shortcut, weakened expectation, or unsupported downgrade.

Target-local AArch64 behavior remains local for scratch choice,
clobber-sensitive block-entry ordering, register spelling, va-list carrier
emission, and machine instruction sequencing. Missing null-publication entry
paths fail closed before materialization instead of recovering producers through
AArch64-local semantic discovery.

Closure proof: `(cmake --build build -j && ctest --test-dir build -j --output-on-failure) > test_after.log 2>&1`
passed 3417/3417 tests, and `git diff --check` passed.
