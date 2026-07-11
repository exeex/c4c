# AArch64 Named Handoff Materializer Cleanup

Status: Open
Type: AArch64 MIR consumer migration
Parent: `ideas/open/703_bir_mir_contract_abstraction_umbrella.md`
After: `ideas/open/706_common_mir_named_query_migration.md`

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
