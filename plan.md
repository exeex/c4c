# Frontend Source Representation for Unresolved-External Direct Calls Runbook

Status: Active
Source Idea: ideas/open/747_frontend_source_representation_for_unresolved_external_direct_calls.md
Activated from: blocked idea-746 runbook; idea 746 remains open and blocked pending this research

## Purpose

Answer the bounded source-language and frontend-representation question that
blocks idea 746: whether a legal production-facing source form can produce a
plain direct fixed-empty scalar call with native global/link identity while
`target_fn` is absent.

## Goal

Create exactly the required index and one evidence-bounded answer document
under `docs/frontend_unresolved_external_direct_call_representation/`, giving
idea 746 either a concrete compliant source/HIR carrier contract or evidence
that the supported language surface cannot express one.

## Core Rule

Research and documentation only. Derive conclusions from a documented
legal-source and parser/sema/HIR/LIR-ingress investigation; do not alter
implementation, tests, expectations, runtime behavior, unsupported markers,
allowlists, HIR, or lifecycle history.

## Read First

- `ideas/open/747_frontend_source_representation_for_unresolved_external_direct_calls.md`
- `ideas/open/746_lir_unresolved_external_direct_call_signature_result_authority.md`
- current call-target ingress rooted at `src/codegen/lir/hir_to_lir/call/target.cpp`
- parser, sema, and HIR declaration/call construction for legal direct-call
  source forms

## Scope

- enumerate relevant legal source declaration and direct-call forms
- trace declaration retention, `DeclRef`, `target_fn`, and native link identity
- classify candidates against the four required properties
- author only the two required research Markdown files

## Non-Goals

- implementation or test changes of any kind
- HIR fabrication, prototype relocation, parallel authority storage, or
  text/name/type/result recovery
- deciding that idea 746, its prerequisite, or idea 744 is complete,
  unblocked, or superseded
- ABI, indirect, variadic, receiver, new-BIR, or broad call-lowering work

## Execution Rules

1. Treat declared and undeclared fixtures as two observations, not a complete
   language-surface conclusion.
2. Every candidate must be traced through the production parser/sema/HIR and
   LIR ingress path before it is classified.
3. Keep the conclusion evidence-bounded: name the exact compliant contract, or
   enumerate the supported-surface evidence for its absence.
4. Write exactly `index.md` and `01_legal_source_to_hir_representation.md` in
   the required directory; do not create extra research deliverables.
5. Do not modify source, tests, existing ideas, or lifecycle artifacts while
   executing the documentation steps.

## Ordered Steps

### Step 1 - Collect and analyze legal source-to-HIR evidence

Goal: enumerate and trace every relevant legal source form before asserting a
positive route or an absence.

Primary targets:

- parser, sema, and HIR declaration/direct-call construction
- `src/codegen/lir/hir_to_lir/call/target.cpp`
- existing declared and undeclared source fixtures as starting observations

Actions:

- identify the supported declaration and direct-call forms that could preserve
  direct global/link identity while leaving `target_fn` absent
- trace each candidate through declaration retention, `DeclRef`, `target_fn`,
  link-identity construction, and fixed-empty declaration facts
- record concrete source/HIR/LIR ingress evidence and classify the declared
  and undeclared fixtures alongside any additional legal candidates
- decide only whether the evidence supports a compliant native carrier
  contract or a supported-surface absence conclusion

Completion check:

- a complete, evidence-cited candidate classification is ready for the
  required answer document, without relying only on the two original fixtures

### Step 2 - Author the required research delivery

Goal: publish the bounded conclusion in exactly the two required documents.

Primary targets:

- `docs/frontend_unresolved_external_direct_call_representation/index.md`
- `docs/frontend_unresolved_external_direct_call_representation/01_legal_source_to_hir_representation.md`

Actions:

- author the numbered answer with the source-to-HIR trace, classification
  table, cited ingress surfaces, and evidence-bounded conclusion
- state either the exact compliant source/HIR carrier contract that can let
  idea 746 resume or the supported-language evidence requiring its runbook to
  be retired or replaced
- author `index.md` linking to the numbered answer and summarizing only that
  overall result
- structurally verify that the directory contains the index and exactly one
  numbered answer file, and confirm the diff is documentation-only

Completion check:

- both required Markdown files exist, are internally linked, satisfy the
  answer shape, and leave all implementation and test surfaces unchanged

## Handoff

Completion of this research runbook does not itself resume, complete, unblock,
supersede, or close idea 746 or idea 744. Hand the evidence and conclusion to
the supervisor for a separate lifecycle decision on idea 746.
