# X86 Named Handoff Materializer Cleanup

Status: Open
Type: x86 MIR consumer migration
Parent: `ideas/open/703_bir_mir_contract_abstraction_umbrella.md`
After: `ideas/open/706_common_mir_named_query_migration.md`
Unblocked By: `ideas/closed/716_prepared_call_plan_cursor_complete_production.md`

## Parked Execution Note

Step 2 is parked before implementation completion. The attempted x86 slice was
rejected and reverted after the supported direct-extern fixture exposed a
common producer gap: semantic calls exist at instruction cursors 0 and 1, but
preparation publishes the zero-argument `actual_function` call exactly at
cursor 0 and omits the argument-bearing `printf` call at cursor 1. The omitted
call has valid semantic operands but no optional argument-source relationships;
common production currently treats that absence as incomplete and drops the
call. Idea 716 owns semantic-operand-based, cursor-complete common production
with unique optional relationship refinement and fail-closed contradictory or
ambiguous evidence. Resume this idea's direct-call/scalar migration only after
that contract has positive and negative proof.

Idea 716 is now closed with its focused producer/lookup proof, independent
route-quality review, and matching full-suite regression comparison accepted.
Resume at Step 2; do not repeat the reverted x86 slice or reintroduce target
fallback for authority now supplied by cursor-exact prepared call plans.

## First Owner And Scope

First owning layer: x86 MIR materialization.  Remove direct route vocabulary
and route-derived fallback decisions from x86 module lowering while preserving
target instruction and ABI policy.

First migrated consumer: `src/backend/mir/x86/module/module.cpp`.

## Dependencies And Proof

- Depends on idea 706's common query contract.
- Proof surface: x86 direct-call and joined-branch handoff tests plus behavior
  coverage for scalar, memory, publication, and call materialization.

## Retirement Guard

The umbrella guard must reach zero in semantic x86 materialization files;
debug labels may remain only for idea 712 and cannot affect lowering.

## Acceptance Criteria

- X86 consumes common named/prepared views and retains only target-local
  instruction/ABI choices.
- Missing prepared placement authority fails closed without route fallback.
- Handoff behavior remains covered across more than one narrow fixture.

## Reviewer Reject Signals

- Route analysis is copied into x86 or hidden in a target helper.
- Debug route state continues to select codegen.
- The slice passes by weakening a handoff expectation.
