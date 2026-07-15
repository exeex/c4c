# LIR-To-New-BIR Remaining Coverage Umbrella

Status: Open (active after 734 Step 7.30 exhaustion)
Type: Umbrella triage and follow-up idea generator
Parent: `ideas/open/734_lir_to_new_bir_container_completeness.md`
Handoff Directory: `docs/lir_to_new_bir_remaining_coverage/`
Related:
- `ideas/open/792_lir_next_local_operation_receiver_handoff.md`
- `docs/lir_local_operation_authority/handoff_to_734.md`
- `ideas/open/761_lir_call_signature_type_mirror_convergence.md`
- `ideas/open/762_lir_module_declaration_type_shadow_convergence.md`
- `ideas/open/763_lir_composite_type_ref_model.md`

## Goal

Use the current post-Step 7.29/7.30 evidence from ideas 734 and 792 to
classify the remaining LIR-to-new-BIR coverage space, document what 734 and 792
do not cover, and generate ordered follow-up ideas after the current
792/734 local-operation work.

## Why This Exists

Idea 734 is the parent implementation route for lossless typed Raw-BIR receipt
of current typed LIR semantics, but repeated runbook exhaustion has shown that
remaining coverage cannot be safely stuffed into one broad implementation
route. Idea 792 is intentionally narrower: it selects and verifies one next
local-operation authority row after 734's accepted local-array GEP receipt.

This umbrella exists to create the next evidence-based queue after the current
792 handoff and corresponding 734 receiver packet. It must prevent route drift
where all remaining 734 gaps are collapsed into one oversized implementation
idea, where local-operation work absorbs unrelated memory/type/body/global
families, or where type/model prerequisites are skipped because a receiver row
is tempting.

## Current Evidence

- `ideas/open/734_lir_to_new_bir_container_completeness.md` is the paused
  parent source. Its post-Step 7.30 exhaustion records accepted receiver work
  through `2323afb91` for closed 792's selected VLA `LirStackSaveOp`, then
  rejects closure because the no-omission matrix, every-row typed receiver
  dispositions, complete dispatcher, whole-module proof, and documentation
  convergence remain unmet.
- Closed 792 selected and published that one VLA stack-save authority in
  `900a42bfd` and `89f4d7f85`; its lifecycle closure is `ecccb195f`. It does
  not authorize stack restore, dynamic VLA allocation, VLA GEP, or broad local
  conversion.
- `docs/lir_local_operation_authority/handoff_to_734.md` currently records
  the 792-selected VLA `LirStackSaveOp` row as a saved-stack-pointer result
  with native current-function `LirValueId`, local object, owner, pointer type,
  pointee type, and liveness authority. It explicitly rejects stack restore,
  dynamic VLA allocation, VLA GEP, nonselected local rows, and presentation
  recovery.
- Accepted 734 Step 7.30 evidence is a fresh build plus `^backend_` 5/5 and a
  matching non-decreasing 5/5-to-5/5 canonical guard. This post-792/post-7.30
  evidence supersedes the draft's older active-792 snapshot.
- Type/model prerequisites are likely related but separately owned:
  `ideas/open/763_lir_composite_type_ref_model.md` provides the composite
  `LirTypeRef` carrier, `ideas/open/761_lir_call_signature_type_mirror_convergence.md`
  converges call/signature type mirrors, and
  `ideas/open/762_lir_module_declaration_type_shadow_convergence.md` converges
  module declaration type shadows.

Fresh evidence produced by 792 closure and the later 734 Step 7.30 receiver
packet supersedes the draft's pre-closure snapshot for ordering decisions.

## In Scope

- Create or refresh docs under `docs/lir_to_new_bir_remaining_coverage/`.
- Classify what 734 already covers, what 792 covers, and what neither covers.
- Build an evidence-backed remaining-coverage matrix by first owning layer:
  LIR producer/schema/verifier, Raw-BIR container/importer/verifier,
  dispatcher/proof infrastructure, type model, or documentation convergence.
- Generate ordered follow-up ideas under `ideas/open/` after evidence
  classification, not inside this draft.
- Record dependency and sequencing rules so each follow-up idea names one
  owning layer and a bounded receiver or producer handoff surface.

## Out Of Scope

- Implementing fixes inside this umbrella idea.
- Editing Raw-BIR, LIR producer/schema/verifier code, importer dispatch,
  receiver verification, target lowering, MIR, or emission as part of 793.
- Duplicating 734's implementation contract or claiming 734 completion.
- Creating a single broad "finish all remaining 734 gaps" implementation
  route.
- Mixing producer authority publication, receiver consumption, type-model
  repair, and final proof convergence into one follow-up idea.
- Recovering identity or types from local names, `%t`, formatted operands,
  printer output, LLVM text, rendered type strings, or testcase identity.
- Expectation rewrites, unsupported-marker edits, allowlist edits, verifier
  weakening, or runtime behavior changes as proof of progress.

## Priority Model

Follow-up ordering should prefer first owning layer and dependency unblock
value over named testcase pressure. Producer authority gaps must precede
Raw-BIR receiver work. Type/model gaps that block many families should be
ordered before receiver ideas that would otherwise rely on runtime text or
shadow fields. Receiver ideas should stay one bounded semantic family at a
time unless the evidence proves a shared importer/verifier seam is the true
first owner.

The expected ordering after the current 792/734 local-operation slice is:

1. Complete the selected VLA stack-save handoff in 792 and the corresponding
   one-row 734 receiver packet.
2. Reclassify the remaining local/VLA rows from fresh post-792/post-7.30
   evidence.
3. Unblock shared type/model prerequisites where they are the first owner for
   multiple remaining families.
4. Generate and order the remaining producer, receiver, and proof-convergence
   ideas by family and owning layer.

## Required Follow-Up Families

This umbrella must classify and generate/order follow-up idea families unless
fresh evidence proves a better split:

- Remaining local/VLA after 792: stack restore, dynamic VLA allocation, VLA
  GEP, nonselected local load/store/GEP/local-temporary rows, and any
  remaining local lifetime rows.
- Memory and `va_list`: memcpy-like residuals, va-start/va-arg/va-end/copy
  semantics, pointer/object lifetime, and memory-operation authority not
  already covered by accepted selected rows.
- Aggregate and vector: aggregate object/value identity, vector type/value
  identity, composite field/member access, and any dependent structured type
  carrier work.
- Body parameters: parameter value identity in function bodies, ABI-expanded
  parameter forms, byval/aggregate/HFA/vector parameter rows, variadic
  parameter surfaces, and body-use authority.
- Module/type/global/metadata: module declarations, extern/function/global
  type shadows, composite `LirTypeRef` prerequisites, struct declarations,
  global initializers, metadata, and compatibility text boundaries.
- Instruction, terminator, and inline-assembly residuals: remaining ordinary
  calls/casts/binary/unary/intrinsic rows, PHI and CFG-adjacent residuals,
  terminator forms not already received, and inline-assembly value/type
  bindings without parsing opaque templates or constraints.
- Final no-omission matrix, dispatcher, and proof convergence: explicit
  dispatcher completeness, neighboring positive/negative coverage,
  whole-module transactional proof, documentation-to-code convergence, and a
  final evidence table proving every valid current-LIR fact has a typed
  disposition.

## Acceptance Criteria

- `docs/lir_to_new_bir_remaining_coverage/` contains a current evidence
  summary, remaining-coverage classification, follow-up ordering plan, and
  closure trace.
- The documents agree on the same current evidence source and state whether
  they use pre- or post-792/post-7.30 evidence.
- The classification clearly distinguishes what 734 has accepted, what 792
  selected or excluded, and what neither idea covers.
- Ordered follow-up ideas are generated under `ideas/open/` only after the
  classification docs exist and each generated idea names its first owning
  layer.
- Each follow-up idea is bounded to one family or one first-owner prerequisite
  and avoids mixed producer/receiver/type/proof ownership.
- The umbrella does not change implementation, tests, expectations,
  unsupported markers, allowlists, runtime behavior, default harness
  contracts, Raw-BIR/LIR code, or importer/verifier behavior.

## Closure Note Requirements

The closure note must state which 734, 792, handoff, type-model, and proof
evidence was used; which docs were written under
`docs/lir_to_new_bir_remaining_coverage/`; which follow-up ideas were
generated under `ideas/open/`; how those ideas were ordered; which first-owner
dependencies drove the ordering; and what remains unassigned or intentionally
deferred.

## Reviewer Reject Signals

- Reject direct implementation, Raw-BIR edits, LIR producer/schema/verifier
  edits, importer changes, or test expectation changes inside this umbrella.
- Reject a draft or closure that duplicates 734's implementation route instead
  of generating an ordered successor queue.
- Reject output that treats 792's VLA `LirStackSaveOp` row as authorizing stack
  restore, dynamic VLA allocation, VLA GEP, or broad local conversion.
- Reject follow-up ideas that stuff all remaining 734 gaps into one route or
  mix producer authority, receiver consumption, type-model repair, and final
  proof convergence without an evidence-backed owning layer.
- Reject classification that ignores the related type/model prerequisites in
  ideas 761, 762, and 763 when composite/type-shadow authority is the first
  owner.
- Reject stale pre-792 or pre-7.30 evidence left as authoritative after fresh
  792 closure or 734 receiver evidence exists.
- Reject testcase-shaped shortcuts, expectation rewrites, unsupported
  downgrades, allowlist filtering, presentation-derived identity recovery, or
  weaker runtime/verifier checks as progress.
