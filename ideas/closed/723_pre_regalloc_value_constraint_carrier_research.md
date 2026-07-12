# Pre-Regalloc Value Constraint Carrier Research

Status: Completed
Type: Research and architecture documentation
Parent: `ideas/open/722_direct_edge_publication_available_move_contract.md`
Related:
- `src/backend/prealloc/regalloc.cpp`
- `src/backend/prealloc/regalloc.hpp`
- `ideas/open/722_direct_edge_publication_available_move_contract.md`
- `ideas/open/708_x86_named_handoff_materializer_cleanup.md`
Owning Layer: semantic BIR, liveness, and common pre-regalloc constraint production

## Completion Note (2026-07-12)

Completed and accepted. The four required documents under
`docs/pre_regalloc_value_constraints/` establish the existing flow, choose
prepared semantic ingress plus normalized regalloc enforcement, and define a
deterministic explicit-register inline-assembly proof with a narrow follow-up
and stop condition. The matching six-test regression guard passed with no new
failures. Follow-up implementation is tracked separately in
`ideas/open/724_prepared_inline_asm_explicit_register_allocation_constraints.md`.

## Goal

Produce concrete research documents under `docs/pre_regalloc_value_constraints/`
that identify the correct producer-owned semantic input for general fixed or
preferred value-register constraints and define a narrow implementation boundary.

## Why This Exists

Idea 722 needs a genuine direct-edge move whose source and phi destination are
assigned distinct registers before prepared publication is built. Current
common regalloc constraints contain fixed and preferred register fields, but
their inputs are derived internally from liveness and ABI policy; no supported
general semantic value constraint was found. Implementing a fixture control or
target fallback would overfit the blocked case, while adding a new schema
without ownership evidence could distort the common compiler contract.

## Research Questions And Required Answer Files

There are three research questions. Delivery must contain exactly three
numbered question-answer Markdown files plus one `index.md`.

1. `01_existing_constraint_flow.md`

   Question: Which semantic, ABI, liveness, and target facts currently populate
   `PreparedAllocationConstraint`, and where are fixed/preferred fields consumed?

   Required answer shape:
   - an end-to-end producer/consumer trace with concrete symbols and files
   - a classification of supported inputs versus derived allocation policy
   - the earliest missing input boundary for a general named BIR value

2. `02_semantic_owner_and_schema.md`

   Question: Which common layer should own a general fixed/preferred value
   constraint, if such a carrier is valid?

   Required answer shape:
   - comparison of plausible BIR, prepared semantic, liveness, and regalloc owners
   - invariants for identity, register class/width, target validation, and failure
   - a recommended schema or an evidence-backed decision not to add one

3. `03_proof_and_followup_boundary.md`

   Question: What minimal semantic program and proof matrix would demonstrate
   the capability without testcase-shaped allocation control?

   Required answer shape:
   - a route-independent positive program shape and adjacent negative states
   - deterministic proof requirements that do not depend on incidental allocator order
   - a narrowly scoped follow-up implementation idea outline, or a stop decision

## Required Documentation Output

Create exactly:

- `docs/pre_regalloc_value_constraints/index.md`
- `docs/pre_regalloc_value_constraints/01_existing_constraint_flow.md`
- `docs/pre_regalloc_value_constraints/02_semantic_owner_and_schema.md`
- `docs/pre_regalloc_value_constraints/03_proof_and_followup_boundary.md`

## In Scope

- AST-backed tracing of current constraint producers and consumers.
- Architecture comparison and an explicit ownership decision.
- A deterministic, semantic proof design for any recommended capability.
- Separation of documentation conclusions from later implementation work.

## Out Of Scope

- Implementation or test changes.
- Fixture-only allocation controls or named-case register selection.
- X86 emission, Route 3/Route 5, ABI-policy, or target scheduling changes.
- Closing or weakening idea 722 acceptance criteria.

## Acceptance Criteria

- The documentation directory contains one index plus exactly the three named
  answer files.
- Every answer cites concrete code surfaces and directly answers its assigned
  question.
- The result names the semantic owner and earliest input boundary, or concludes
  with evidence that no general carrier should be added.
- Any proposed follow-up is narrow, deterministic, common-layer owned, and
  clearly separated from this documentation-only idea.
- No implementation, test expectation, runtime behavior, or unrelated
  lifecycle history is changed.

## Reviewer Reject Signals

- Reject a fixture flag, block/value-name match, forced register pair, or other
  joined-branch-shaped control presented as a general semantic carrier.
- Reject expectation downgrades, unsupported classification, helper renames, or
  status relabeling claimed as capability progress.
- Reject a schema recommendation that omits value identity, register
  class/width, target legality, conflict behavior, or deterministic proof.
- Reject x86-local synthesis, post-prepare home mutation, or allocator-output
  rewriting behind a new abstraction.
- Reject broad ABI, target scheduling, publication, or emission rewrites outside
  the researched input boundary.
- Reject retaining the exact absence of a producer-owned pre-regalloc input
  while describing incidental allocator ordering as semantic authority.
