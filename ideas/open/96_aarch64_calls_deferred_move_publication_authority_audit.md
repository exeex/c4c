# AArch64 Calls Deferred Move And Publication Authority Audit

## Goal

Audit the calls clusters that idea 92 explicitly deferred because they mix
prepared call authority, dispatch state, scalar publication, preservation, and
AArch64 emission. Produce implementation follow-up ideas only when a traced
authority boundary is clear.

## Why This Exists

The calls owner subresponsibility audit split the straightforward local-owner
work into ideas 93-95, and those routes are now closed. Idea 95 confirms that
no remaining open ideas from the direct calls subowner chain remain.

However, idea 92 also recorded deferred clusters that need shared-authority
evidence before implementation:

- before-call move bundle lowering
- after-call, return, value, and preservation lowering
- scalar producer dispatch bridge
- result recording and late publication

These are not simple local helper moves. They cross prepared move-bundle and
source facts, after-call result facts, preservation facts, dispatch scalar
state, scalar publication producers, and AArch64 materialization/record
emission. The next step should trace those facts before creating an
implementation route.

## Owned Files

- Audit/read:
  - `src/backend/mir/aarch64/codegen/calls.cpp`
  - `src/backend/mir/aarch64/codegen/calls.hpp`
  - `src/backend/mir/aarch64/codegen/dispatch*.cpp`
  - `src/backend/mir/aarch64/codegen/dispatch*.hpp`
  - relevant prepared call/publication/prealloc sources under
    `src/backend/prealloc/`
- Read-only historical boundary files:
  - `ideas/closed/69_aarch64_call_publication_prepared_authority_cleanup.md`
  - `ideas/closed/84_aarch64_prepared_consumer_wrapper_contraction.md`
  - `ideas/closed/92_aarch64_calls_owner_subresponsibility_audit.md`
  - `ideas/closed/93_aarch64_calls_stack_frame_slot_operand_owner.md`
  - `ideas/closed/94_aarch64_calls_f128_carrier_operand_owner.md`
  - `ideas/closed/95_aarch64_calls_immediate_scalar_argument_publication_owner.md`

## In Scope

- Trace before-call move bundle lowering from prepared move-bundle facts to
  AArch64 call-boundary records and emitted side effects.
- Trace after-call move, return move, value move, and preservation lowering
  through prepared result/preservation facts and dispatch state mutation.
- Trace the scalar producer dispatch bridge, including prepared call argument
  source producers, scalar materialization, and AArch64 address/materialization
  emission.
- Trace result recording and late publication, including call result source
  registers, FPR result store retargeting, missing frame-slot call arguments,
  and stack-preserved value publication.
- Classify each traced cluster as:
  - `target-local-calls-emission`
  - `prepared-fact-consumption`
  - `dispatch-state-mutation`
  - `candidate-local-subowner`
  - `candidate-shared-authority-gap`
  - `needs-narrow-implementation-idea`
  - `intentionally-retained`
- Produce follow-up ideas only for clusters with a narrow proofable boundary.

## Out Of Scope

- Direct implementation edits in calls, dispatch, or prealloc files.
- Moving AArch64 ABI construction, scratch choice, call spelling,
  materialization spelling, or machine-record construction into shared
  BIR/prealloc.
- Reopening completed local-owner routes from ideas 93-95.
- Re-deriving prepared call plans, move bundles, after-call result facts,
  preservation facts, aggregate transport facts, or publication facts under a
  new local helper name.
- One monolithic `calls.cpp` shrink route.

## Proof Expectations

- This is audit-only; no backend tests are required unless implementation files
  are accidentally touched.
- The close note must name any generated follow-up ideas and explicitly say
  which deferred clusters remain intentionally retained.
- Any generated implementation idea must name focused proof for the affected
  call forms, such as before-call move bundles, after-call/result moves,
  return/value moves, preservation/republication, scalar producer publication,
  indirect call materialization interactions, result recording, and rejection
  diagnostics.

## Reviewer Reject Signals

- The audit proposes implementation without tracing the prepared fact and
  dispatch-state path.
- It treats AArch64 materialization or scratch selection as shared authority.
- It reopens ideas 93-95 instead of focusing on the deferred clusters from
  idea 92.
- It proposes broad calls/dispatch/prealloc rewrites rather than scoped,
  proofable follow-up ideas.
- It claims line-count progress without preserving diagnostics, record
  construction, and prepared fact consumption.

