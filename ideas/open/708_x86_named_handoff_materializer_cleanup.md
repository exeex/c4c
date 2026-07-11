# X86 Named Handoff Materializer Cleanup

Status: Open
Type: x86 MIR consumer migration
Parent: `ideas/open/703_bir_mir_contract_abstraction_umbrella.md`
After: `ideas/open/706_common_mir_named_query_migration.md`

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
