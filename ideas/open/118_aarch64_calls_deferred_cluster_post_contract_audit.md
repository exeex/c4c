# 118 AArch64 Calls Deferred Cluster Post-Contract Audit

## Goal

Revisit the deferred `calls.cpp` clusters from idea 92 after ideas 114, 116,
and 117 added shared prepared contract evidence, then produce focused
follow-up ideas for any calls-side ownership that can now be safely extracted
or contracted.

This idea is analysis-only. It should not directly edit `calls.cpp`.

## Why This Exists

`src/backend/mir/aarch64/codegen/calls.cpp` remains the largest AArch64 codegen
owner at 8541 lines. Earlier calls-owner work already completed the stable
local subowner chain:

- idea 93: stack/frame-slot operand owner;
- idea 94: f128 carrier operand owner;
- idea 95: immediate scalar argument publication owner.

Idea 92 intentionally deferred several clusters because they lacked enough
shared-authority evidence at the time:

- before-call move bundle lowering;
- after-call, return, value, and preservation lowering;
- scalar producer dispatch bridge;
- result recording and late publication.

Since then, newer shared-contract work has landed:

- idea 114: prepared outgoing stack argument area contract;
- idea 116: dispatch prepared producer/current-block/edge-publication contract
  surface;
- idea 117: comparison fused-compare/current-block publication contract.

Those contracts may make some formerly deferred calls clusters ready for a
bounded extraction, or may prove that they should remain target-local emission
glue. This audit should make that decision with current evidence instead of
creating a monolithic "shrink calls.cpp" route.

## In Scope

- Read the closure notes for ideas 92, 93, 94, 95, 114, 116, and 117.
- Build a current function-level map for the deferred `calls.cpp` clusters:
  - before-call move bundle lowering;
  - after-call result/value lowering;
  - preservation and republication lowering;
  - scalar producer dispatch bridge;
  - result recording and late publication;
  - ordinary call-boundary move/binding record construction where it directly
    couples to the deferred clusters.
- Classify each cluster as:
  - `ready-local-owner`: can be extracted as an AArch64-local owner consuming
    existing prepared facts;
  - `move-forward-needed`: still owns target-neutral facts that should move to
    shared BIR/prealloc first;
  - `keep-in-calls`: still fundamentally call instruction/ABI record emission;
  - `contract-needed`: requires focused tests or dump visibility before code
    movement;
  - `no-new-idea`: already covered by a closed route.
- Produce follow-up ideas only for bounded, non-duplicative slices.

## Out Of Scope

- Direct implementation changes in `src/backend/mir/aarch64/codegen/calls.cpp`.
- Reopening completed local subowners from ideas 93, 94, or 95.
- Reworking outgoing stack area authority already covered by idea 114.
- Reopening dispatch prepared producer or current-block publication contracts
  already covered by ideas 116 and 117.
- Broad call ABI rewrite, phoenix rebuild, or file splitting without a
  concrete cluster boundary.
- x86 or RISC-V codegen implementation.

## Expected Outputs

The closure note should include:

- a concise map from idea-92 deferred cluster to current owner disposition;
- explicit references to any 114/116/117 shared facts that make a cluster ready
  or still blocked;
- follow-up `ideas/open/` files for each actionable cluster;
- explicit "no new idea" entries for clusters that remain intentionally local
  or are already covered.

Good follow-up idea shapes include:

- a local owner for after-call result/preservation record emission if it now
  consumes prepared result and preservation facts cleanly;
- a shared prealloc fact/query idea if calls still reconstructs target-neutral
  result or republication authority;
- a contract-visibility idea if a cluster cannot be moved safely because dump
  or route tests do not expose the prepared fact boundary;
- a narrow local owner for before-call move bundle emission if the prepared
  move-bundle facts are already complete.

## Acceptance And Completion Criteria

- The audit creates no implementation, test, or build metadata changes.
- Each generated follow-up idea names likely files, owner boundary, proof route,
  and testcase-overfit reject signals.
- No generated idea duplicates ideas 93, 94, 95, 114, 116, or 117.
- The audit does not claim line-count reduction as progress; it identifies
  concrete ownership boundaries that enable later safe contraction.

## Reviewer Reject Signals

- The route proposes one monolithic `calls.cpp` shrink implementation.
- It treats a cluster as ready only because the file is large, without naming
  the prepared/shared facts consumed by the proposed owner.
- It reopens completed stack/frame-slot, f128 carrier, immediate scalar,
  outgoing stack area, dispatch producer, or comparison publication work without
  new evidence.
- It proposes moving AArch64-specific ABI spelling, scratch register policy, or
  machine-record emission into shared BIR/prealloc.
- It creates vague follow-up ideas without a bounded proof route.
