# X86 Defined-Function Prepared-Core Completion

Status: Open
Type: x86 prepared-core producer-contract repair
Discovered by: `ideas/open/718_block_entry_publication_identity_completion.md`

## Goal

Ensure every defined function entering x86 module emission has the common
prepared-core function view required by the emitter, without weakening or
bypassing that readiness invariant.

## Why This Exists

`backend_prepare_frame_stack_call_contract` reaches x86 module emission and
throws `x86::module::emit requires prepared core facts for every defined
function`. This occurs after the block-entry identity assertion and is separate
from idea 718's prepared-to-BIR publication identity contract. Existing idea
716 owns prepared call-plan cursor completeness, not generic prepared-core
availability for every defined function.

## In Scope

- Localize the exact defined function that lacks a prepared-core view and the
  earliest producer boundary where that common record should have been
  published.
- Repair the general preparation-to-x86-emission contract so all eligible
  defined functions carry the required prepared-core facts.
- Add focused coverage for the failing function shape and at least one nearby
  defined-function shape, including a precise fail-closed case for genuinely
  absent preparation.
- Prove the repaired focused contract and a supervisor-selected broader x86 or
  backend comparison without expectation changes.

## Out Of Scope

- Block-entry publication identity, proof-claim attribution, or idea 718's MIR
  adapter.
- Prepared call-plan cursor semantics owned by idea 716 unless localization
  proves the missing common record is specifically caused by that contract;
  such ownership evidence requires a plan-owner route decision before scope
  changes.
- Bypassing the emitter invariant, treating defined functions as declarations,
  or synthesizing target-emission facts in fixtures.
- ABI policy, register spelling, move ordering, or unrelated target lowering.

## Acceptance Criteria

- The failing defined function and earliest missing prepared-core producer fact
  are documented before repair.
- Every eligible defined function in the focused coverage reaches x86 emission
  with the genuine common prepared-core view.
- Genuinely absent or inconsistent preparation remains a precise failure; the
  emitter invariant is not weakened.
- Focused and broader matching proof are green without expectation downgrades
  or fixture-only readiness injection.

## Reviewer Reject Signals

- A named-fixture, function-name, or single-test conditional that only makes
  `backend_prepare_frame_stack_call_contract` pass.
- Removing, weakening, catching, or routing around the x86 prepared-core
  invariant instead of repairing the producer contract.
- Marking the fixture unsupported, changing expected results, treating a
  definition as a declaration, or manually injecting readiness in the test.
- Helper renames, status reclassification, or expectation rewrites claimed as
  prepared-core capability progress.
- Broad changes to idea 718 attribution, idea 716 call-plan semantics, ABI
  policy, or unrelated x86 emission without localized necessity and a
  lifecycle route decision.
- Retaining the same missing prepared-core failure behind a new abstraction or
  later emission checkpoint.
