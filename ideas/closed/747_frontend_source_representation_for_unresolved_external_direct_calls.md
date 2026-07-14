# Frontend Source Representation for Unresolved-External Direct Calls

Status: Closed (research delivery complete)
Type: Research and architecture documentation
Parent: `ideas/open/746_lir_unresolved_external_direct_call_signature_result_authority.md`
Related:
- `ideas/open/746_lir_unresolved_external_direct_call_signature_result_authority.md`
- `ideas/open/744_lir_remaining_ordinary_value_identity_publication.md`
- `src/codegen/lir/hir_to_lir/call/target.cpp`
Owning Layer: source language, frontend declaration/HIR representation, and
LIR call-target ingress

## Goal

Produce concrete research documents under
`docs/frontend_unresolved_external_direct_call_representation/` that determine
whether a legal production-facing source form can simultaneously preserve a
plain direct global/link identity and leave `target_fn` absent for a fixed-empty
scalar call, or whether the frontend representation makes that combination
unavailable.

## Why This Exists

Idea 746 requires a positive source-level probe and forbids HIR fabrication,
prototype relocation, and text recovery. Its Step 1 evidence shows that the
two attempted source forms divide the needed facts: a declared fixed-empty call
retains `target_fn`; an undeclared call reaches unresolved-external handling
without the native direct global/link identity. That is not enough to conclude
that no legal production-facing source representation exists. A bounded
frontend/source-representation investigation must resolve this uncertainty
before 746 can choose a valid producer route, retirement, or replacement.

## Research Questions And Required Answer Files

There is 1 research question. The delivery must contain exactly 1
question-answer Markdown file plus one `index.md`.

1. `01_legal_source_to_hir_representation.md`

   Question: Which legal source-level declaration/call forms can produce a
   plain direct call with native global/link identity while `target_fn` is
   absent, and, if none can, what parser/sema/HIR evidence proves that absence
   for the supported language surface?

   Required answer shape:
   - trace each candidate legal source form through declaration retention,
     `DeclRef`, `target_fn`, and link-identity construction
   - classify candidates in a table by native direct identity, `target_fn`
     presence, fixed-empty signature facts, and production eligibility
   - reach one evidence-bounded conclusion: a concrete compliant probe and
     carrier contract for idea 746, or a documented absence that requires
     retiring/replacing its active runbook

## Required Documentation Output

Create the research documents in:

```text
docs/frontend_unresolved_external_direct_call_representation/
```

Required files:

- `docs/frontend_unresolved_external_direct_call_representation/index.md`
- `docs/frontend_unresolved_external_direct_call_representation/01_legal_source_to_hir_representation.md`

`index.md` must link to the numbered answer file and summarize the overall
result. It must not replace the required answer file.

## In Scope

- source-language, parser, sema, and HIR evidence needed to enumerate legal
  declaration and direct-call forms relevant to the bounded idea-746 route
- a source-to-HIR trace and explicit classification of the two failed fixtures
  and any other legal candidates discovered
- documentation of the exact native carrier contract that would let idea 746
  resume, or evidence-bounded documentation that no such current source form
  exists

## Out Of Scope

- Implementation changes.
- Test expectation, unsupported-marker, allowlist, or runtime behavior changes.
- HIR mutation, prototype relocation, parallel source-authority tables, or
  text/name/type/result recovery.
- Activating this idea into `plan.md` unless explicitly requested later.
- Declaring idea 746, its prerequisite, or idea 744 complete, unblocked, or
  superseded.

## Acceptance Criteria

- `docs/frontend_unresolved_external_direct_call_representation/` contains one
  `index.md` and exactly one numbered answer file.
- The answer traces concrete frontend and LIR ingress surfaces and classifies
  every candidate against direct global/link identity, `target_fn`, and native
  fixed-empty declaration facts.
- Its conclusion is limited to the evidence: it either names a legal compliant
  positive route for 746 or explains why the currently supported source
  language cannot express one; it does not infer either conclusion from only
  the two original fixtures.
- No implementation files, test expectations, unsupported markers, allowlists,
  runtime behavior, active plan state, or lifecycle history are changed by
  executing this research idea.

## Completion Record

The required `index.md` and sole numbered answer file were delivered in
`docs/frontend_unresolved_external_direct_call_representation/` and committed
in `df18f32e8`. Their evidence-bounded conclusion is that the currently
supported source surface lacks a legal production-facing plain direct,
fixed-empty scalar-call carrier with native global/link identity and absent
`target_fn`.

This completes the documentation research only. It retires the then-active
idea-746 runbook through a separate lifecycle decision; it does not complete,
unblock, supersede, or close ideas 746 or 744.

## Reviewer Reject Signals

- Reject a conclusion based only on the declared and undeclared fixtures rather
  than a documented legal-source and frontend/HIR search.
- Reject HIR fabrication, prototype relocation, parallel authority storage, or
  text/name/type/result recovery presented as a production-facing source route.
- Reject implementation, test-expectation, unsupported-marker, allowlist, or
  runtime behavior changes disguised as research.
- Reject a recommendation that advances idea 746 without a concrete native
  carrier contract, or retires it without evidence that the supported source
  language lacks a legal representation.
- Reject claims that the work completes or unblocks idea 744, or that it
  expands into ABI, indirect, variadic, receiver, new-BIR, or broad call
  lowering work.
