# Shared Select-Chain And Same-Block Dependency Queries

## Goal

Share target-neutral select-chain and same-block dependency queries that are
still queried locally around AArch64 producer and value-publication
materialization.

## Why This Exists

The post-contract dispatch-family audit found remaining local queries for
select-chain direct-global dependency and same-block scalar or constant
materialization checks. These are prepared fact relationships that should live
behind a shared query surface when they are needed by lowering.

This idea exists to generalize those relationships without moving AArch64
materialization emission.

## In Scope

- `src/backend/mir/aarch64/codegen/dispatch_producers.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`
- The selected shared MIR, prepared, or prealloc query owner
- Shared query coverage for select-chain direct-global dependency and
  same-block scalar or constant materialization relationships

## Out Of Scope

- AArch64 instruction emission, register-read hazard handling, scratch choice,
  or final materialization order
- Named-case select-chain shortcuts
- Direct-global special cases that do not generalize to the prepared fact model
- Broad dispatch-family file reshaping

## Acceptance Criteria

- Local select-chain or same-block dependency checks are replaced by a shared
  query that describes the prepared relationship directly.
- AArch64 materialization and register-read hazard behavior are preserved.
- The route proves representative nearby cases, not only one known failing
  shape.
- Public dispatch producer surface is narrowed only when callers have a clearer
  shared owner.

## Proof Route

- `cmake --build --preset default`
- `ctest --test-dir build -R '^backend_' --output-on-failure`

Use matching `test_before.log` and `test_after.log` because shared query code
is in scope.

## Completion Note

Closed after the same-block materialization and select-chain dependency
relationships were exposed through shared prepared query facades and the AArch64
call sites were routed through those shared surfaces while keeping target
emission, hazard, scratch, and materialization-order policy local. Backend
regression guard passed with matching `^backend_` scope at 179/179 passing
tests and no new failures.

## Reviewer Reject Signals

- The route adds named-case select-chain or direct-global shortcuts.
- The shared API mirrors AArch64 local helper shape without becoming a real
  prepared fact query.
- Proof covers only one current testcase while nearby dependency forms remain
  unsupported or unexamined.
- AArch64 emission or hazard policy moves into shared code.
- Expectations are weakened to claim progress.
