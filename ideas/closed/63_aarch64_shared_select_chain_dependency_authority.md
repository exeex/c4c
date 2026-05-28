# AArch64 Shared Select-Chain Dependency Authority

Ownership class: shared-authority migration plus target-owner split

## Goal

Move direct-global and select-chain dependency discovery into shared prepared
select-chain or call-plan authority, then relocate target select materialization
to a precise AArch64 local owner instead of broad dispatch-family code.

## Why This Exists

Idea 59 found direct-global select-chain dependency traversal split across
edge-copy, producer, call-argument, ALU, and value-materialization consumers.
The semantic dependency facts should be shared/prepared, while AArch64 target
materialization should remain local but live under a narrower owner than the
generic dispatch family.

## In Scope

- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`
  `emit_select_chain_value_to_register` and
  `materialize_direct_global_select_chain_call_argument`.
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
  `prepared_select_chain_contains_direct_global_load` and prepared select
  bridge fallback behavior.
- `src/backend/mir/aarch64/codegen/calls.cpp` call-argument consumers.
- ALU and value-materialization select-chain callers.
- The shared prepared select-chain or call-plan owner that should expose the
  dependency facts.

## Out Of Scope

- Moving AArch64 select emission, register spelling, or call-argument
  instruction sequencing into shared code.
- Solving same-block scalar recursion, edge fallback, or join-copy cache
  migration.
- Adding select-chain-shaped special cases for one direct-global pattern.
- Bulk moving all select handling into `dispatch.cpp`.

## Acceptance Criteria

- Direct-global and select-chain dependency discovery comes from shared
  prepared select-chain or call-plan facts.
- `PreparedCallPlan` or the shared select-chain fact owner covers existing
  call-argument cases before AArch64 local fallback traversal is removed.
- A precise target owner, not broad dispatch-family code, owns AArch64 select
  materialization and call-argument emission details.
- Focused proof covers non-edge select chains, direct-global select-chain call
  arguments, ALU select materialization, value publication through select
  chains, and equivalence with shared prealloc/publication-plan dependency
  coverage.

## Reviewer Reject Signals

- Duplicated direct-global traversal remains in AArch64 dispatch after the
  migration.
- `PreparedCallPlan` or shared select-chain facts do not cover existing
  call-argument cases.
- Non-edge callers are not proven.
- The result works only by adding select-chain-shaped special cases.
- The route changes target emission sequencing while claiming only shared
  dependency migration.

## Completion Note

Closed after the active runbook completed inventory, shared call-argument
facts, call-argument consumer migration, non-call select-chain fact migration,
target-local select materialization owner relocation, and full-suite Step 6
proof. Close validation used the full build plus full CTest suite with
3417/3417 tests passing before and after.
