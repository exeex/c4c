# LIR Intrinsic and Inline-Assembly Binding Evidence

Status: Open
Type: Research and architecture documentation
Parent: `ideas/open/813_lir_string_semantic_authority_completion_umbrella.md`
Related:
- `docs/lir_string_authority_remaining_routes/handoff_to_813.md`
- `docs/lir_string_semantic_authority_completion/input_validation.md`
- `ideas/open/796_lir_instruction_terminator_residual_authority_handoff.md`
- `ideas/open/846_lir_family_overloaded_verifier_dispatch_printer.md`
- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- `ideas/open/797_lir_to_new_bir_final_coverage_convergence.md`
Owning Layer: intrinsic and inline-assembly binding producer/lowerer evidence
Routing Key: `instruction.intrinsic-binding-evidence`

## Goal

Produce concrete evidence documents under `docs/lir_intrinsic_binding_evidence/`
that establish the complete producer/lowerer audit boundary for intrinsic
operands and `LirInlineAsmValueBinding`, while keeping inline-assembly templates
and constraints opaque outward payload.

## Why This Exists

Ordinary inline-assembly values require `LirInlineAsmValueBinding`, but other
intrinsic forms vary and the accepted input lacks a complete producer/lowerer
binding audit. 796 owns only one selected residual instruction family at a time;
846 retains verifier/dispatch ownership for named forms. Neither source proves
the full binding inventory. Any accepted later handoff returns first to the
appropriate 796/846 owner, then may become a 734 receipt and eventual 797 input.

## Research Questions And Required Answer Files

There is 1 research question. The delivery must contain exactly 1 numbered
question-answer Markdown file plus one `index.md`.

1. `01_intrinsic_inline_asm_binding_route.md`

   Question: Which intrinsic and inline-assembly binding producers reach which
   verifier and lowering consumers, and where are native binding facts missing
   or unproved?

   First diagnostic question: Which `LirInlineAsmValueBinding` producers and
   intrinsic lowerers exist, and what malformed-binding rejection do they have?

   Required answer shape:
   - a complete producer-to-verifier-to-lowerer inventory, classified by
     intrinsic form and binding representation;
   - positive evidence for modeled bindings and malformed, foreign, and
     mismatch binding expectations where applicable;
   - an explicit proof that templates and constraints are never parsed for
     binding, type, value, ABI, or dispatch facts;
   - a conclusion assigning any future narrow repair to 796 or 846 only when
     its exact family and return path are proved.

## Required Documentation Output

Create the research documents in:

```text
docs/lir_intrinsic_binding_evidence/
```

Required files:

- `docs/lir_intrinsic_binding_evidence/index.md`
- `docs/lir_intrinsic_binding_evidence/01_intrinsic_inline_asm_binding_route.md`

`index.md` must link to the numbered answer file and summarize the result. It
must not replace the required answer file.

## In Scope

- Documentation and evidence tracing for the exact routing key only.
- Enumeration of `LirInlineAsmValueBinding` producers and intrinsic lowerers,
  including native binding paths and their verifier/receiver boundaries.
- Positive plus malformed, foreign, and mismatch evidence expectations where
  applicable, and return/dependency relations to 796, 846, 734, and 797.

## Out Of Scope

- Implementation changes, a combined residual instruction sweep, or activation.
- Parsing inline-assembly templates or constraints for binding, type, value,
  ABI, or dispatch facts.
- Accepted 796 cast-result/pointer-subtraction work, opaque asm payload policy,
  or unrelated verifier/dispatch migration under 846.
- Test expectation, unsupported-marker, allowlist, runtime-behavior, or
  lifecycle-state changes.

## Acceptance Criteria

- The output directory contains `index.md` and exactly the one numbered answer
  file named above.
- The answer records every discovered binding producer and lowering consumer
  with concrete evidence and reports unknown coverage as missing evidence.
- Positive binding evidence and applicable malformed, foreign, and mismatch
  rejection expectations are explicit.
- The documents prove the template/constraint opacity boundary and name an
  exact 796 or 846 return relation before mentioning a future repair; 734 and
  797 remain downstream receivers, not research owners.
- No implementation files, test expectations, unsupported markers, allowlists,
  runtime behavior, active plan state, or lifecycle history are changed.

## Reviewer Reject Signals

- Reject template/constraint parsing, text-derived binding recovery, or an
  opacity exception disguised as an intrinsic audit.
- Reject a combined residual sweep or a future repair recommendation without a
  singular family, native fact, and 796/846 return relation.
- Reject inventories without concrete producer/lowerer evidence or applicable
  positive and malformed/foreign/mismatch boundaries.
- Reject implementation, expectation downgrades, helper renames, or
  classification-only changes claimed as evidence/capability progress.
- Reject a claimed 734 receipt or 797 convergence before an exact accepted
  producer/verifier handoff exists.
