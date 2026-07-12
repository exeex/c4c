# AArch64 Named Handoff Materializer Cleanup

Status: Open
Type: AArch64 MIR consumer migration
Parent: `ideas/open/703_bir_mir_contract_abstraction_umbrella.md`
After: `ideas/open/706_common_mir_named_query_migration.md`

## Return-Chain Authority Gate (2026-07-12)

Step 2.1 proved that the existing traversal-attached common API does not own
the complete return-chain relation required by AArch64 ALU materialization.
The current view exposes one event's move bundle, source freshness, and
optional stack fan-in authority, but not the terminal return-ABI home, the
first successor ALU non-chain operand home, or their cross-instruction
move/scalar-producer relation. Reconstructing those facts in `alu.cpp` violates
this idea's consumer-only boundary.

The first Step 2.1 consumer attempt was rejected and fully reverted because
existing return-chain inputs classified `Stale`. Idea 727 has now repaired the
common publication seam and proved that representative production AArch64
inputs receive traversal-attached, fresh, attributed `Available` relations.

Idea 727 is closed, but the next Step 2.1 attempt proved a distinct production
shape gap. The real public multi-link external add/sub chain attaches
`PreparedObjectReturnChainStatus::StructurallyIncomplete` at its first ALU
event even though the synthetic one-/two-link contract and representative
one-link AArch64 builder report `Available`. The focused guard swaps in
`backend_cli_aarch64_asm_external_return_add_sub_chain_smoke` as a new failure.

Idea 709 remains open but is parked at Step 2.1 behind idea 728. Do not retry
consumer deletion until that decomposition initiative separates public
one-link publication, multi-link successor attribution/adjacency, terminal
`FunctionReturnAbi` binding, and traversal/classification parity and proves the
narrowest generic producer repair. Do not reopen 727 or rebuild authority in
AArch64.

## First Owner And Scope

First owning layer: AArch64 MIR materialization.  Migrate dispatch, calls,
globals, ALU, comparison, select, publication, and value materialization away
from direct route records and target-local route indexes.

First migrated consumer: `src/backend/mir/aarch64/codegen/dispatch.cpp`, then
the sibling AArch64 materializers under the same owner.

## Dependencies And Proof

- Depends on idea 706's common query contract.
- Proof surface: AArch64 instruction-dispatch, call-boundary, branch-control,
  current-block/join, scalar-ALU, and memory-operand tests.

## Retirement Guard

The umbrella guard must reach zero in semantic AArch64 codegen.  No AArch64
helper may rebuild route indexes or preserve a fallback on route absence.

## Acceptance Criteria

- Every materializer consumes common named/prepared views.
- Target code owns only legal instruction and ABI realization.
- Comparison/select/control and publication paths fail closed on unavailable
  authority and retain broad behavior proof.

## Reviewer Reject Signals

- One dispatch path migrates while sibling helpers retain semantic routes.
- Route/index recreation is renamed rather than removed.
- Testcase identity, assembly text alone, or expectation weakening is proof.
