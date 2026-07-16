# LIR Global Policy and Symbol-Identity Evidence

Status: Open
Type: Research and architecture documentation
Parent: `ideas/open/813_lir_string_semantic_authority_completion_umbrella.md`
Related:
- `docs/lir_string_authority_remaining_routes/handoff_to_813.md`
- `docs/lir_string_semantic_authority_completion/input_validation.md`
- `ideas/open/844_lir_global_extern_initializer_family_facts.md`
- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- `ideas/open/797_lir_to_new_bir_final_coverage_convergence.md`
Owning Layer: global/module policy-field producer-to-verifier-to-new-BIR evidence
Routing Key: `global.policy-identity-evidence`

## Goal

Produce concrete evidence documents under `docs/lir_global_policy_identity_evidence/`
that answer whether every modeled global linkage, visibility, qualifier-policy,
and symbol-identity fact has a native producer-to-BIR-receiver route, without
inferring any fact from rendered declaration text.

## Why This Exists

The accepted 812/813 input establishes dedicated global policy fields and
`LinkNameId` where present, but does not establish complete policy-field
producer-to-receiver coverage. Open 844 owns global/extern type facts and
explicitly excludes this non-type routing question; 734 may receive only an
accepted typed receiver handoff, and 797 may converge only after such receipts.

## Research Questions And Required Answer Files

There is 1 research question. The delivery must contain exactly 1 numbered
question-answer Markdown file plus one `index.md`.

1. `01_global_policy_identity_route.md`

   Question: For every `LirGlobal` linkage, visibility, qualifier-policy, and
   symbol-identity field, which producer, verifier gate, and new-BIR receiver
   handles it, and which exact seams remain unproved?

   First diagnostic question: What does `rg` reveal when every `LirGlobal`
   policy/identity field is traced into globals lowering?

   Required answer shape:
   - a field-by-field producer, verifier, and receiver trace with concrete
     source locations;
   - positive evidence for each covered native field, plus malformed, foreign,
     or mismatch evidence where the field has an identity or consistency gate;
   - a disposition separating proven evidence, missing evidence, and a
     narrowly described later implementation candidate, if any;
   - an explicit conclusion that declaration rendering is never used to
     reconstruct policy or identity.

## Required Documentation Output

Create the research documents in:

```text
docs/lir_global_policy_identity_evidence/
```

Required files:

- `docs/lir_global_policy_identity_evidence/index.md`
- `docs/lir_global_policy_identity_evidence/01_global_policy_identity_route.md`

`index.md` must link to the numbered answer file and summarize the result. It
must not replace the required answer file.

## In Scope

- Documentation and evidence tracing for the exact routing key only.
- Native global policy and identity facts from producer through verifier to BIR
  receiver, including the named positive and malformed/foreign/mismatch proof
  expectations where applicable.
- A return record stating what 844 can reuse as global-family context, what 734
  can receive as an evidence-backed receiver handoff, and what 797 must await.

## Out Of Scope

- Implementation changes, receiver repairs, or activation into `plan.md`.
- `llvm_type`/declaration-shadow rows, absent-metadata compatibility,
  initializer scans, or existing global/extern type ownership under 844.
- Test expectation, unsupported-marker, allowlist, runtime-behavior, or
  lifecycle-state changes.
- Inferring linkage, visibility, qualifier policy, or identity from rendered
  declaration text.

## Acceptance Criteria

- The output directory contains `index.md` and exactly the one numbered answer
  file named above.
- The answer directly records the first diagnostic trace and concrete source
  evidence for every in-scope policy/identity field.
- Positive coverage and applicable malformed, foreign, or mismatch evidence
  expectations are explicit; unavailable evidence is reported as missing, not
  silently treated as coverage.
- The documents define a precise dependency/return relation to 844, 734, and
  797 without changing any of their scopes.
- No implementation files, test expectations, unsupported markers, allowlists,
  runtime behavior, active plan state, or lifecycle history are changed.

## 866 Reconciliation And 734 Return

Idea 866 reuses this as the ordered documentation successor for global policy
and symbol identity evidence. It may return to 734 only by naming one exact
evidence-backed typed handoff after the field trace is complete; otherwise 734
and 797 remain downstream and deferred.

## Reviewer Reject Signals

- Reject any implementation, test-contract, or lifecycle change presented as
  evidence research.
- Reject a route that derives semantic policy or identity from declaration
  rendering, display spelling, or link-name presentation.
- Reject a field inventory that omits a producer, verifier, receiver, or the
  positive and applicable malformed/foreign/mismatch boundary.
- Reject reuse of 844 as owner of non-type policy evidence, or a claimed 734/
  797 receipt before the evidence identifies an exact handoff.
- Reject testcase-shaped conclusions, expectation downgrades, helper renames,
  or classification-only changes claimed as capability progress.
