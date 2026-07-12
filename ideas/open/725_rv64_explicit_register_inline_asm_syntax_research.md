# RV64 Explicit-Register Inline-Assembly Syntax Research

Status: Open
Type: Research and architecture documentation
Parent: `ideas/open/724_prepared_inline_asm_explicit_register_allocation_constraints.md`
Related:
- `src/backend/bir/lir_to_bir/calling.cpp`
- `src/backend/prealloc/inline_asm.cpp`
- `src/codegen/lir/verify.cpp`
- `docs/pre_regalloc_value_constraints/`
Owning Layer: frontend/LIR inline-assembly syntax and semantic BIR metadata

## Goal

Produce concrete research documents under
`docs/rv64_explicit_register_inline_asm/` that determine whether c4c has or can
narrowly support a legitimate RV64 explicit-register operand syntax before
prepared allocation.

## Why This Exists

Idea 724 assumed one explicit-register operand family was already representable.
Tracing disproved that premise: physical operand tokens fall into
`unsupported_constraint`, while physical-name parsing occurs only during
post-regalloc carrier validation. Syntax ownership and source-to-LIR preservation
must be established before implementation can safely resume.

## Research Questions And Required Answer Files

There are two research questions. Delivery must contain exactly two numbered
answer files plus one `index.md`.

1. `01_source_to_bir_syntax_flow.md`

   Question: Which source, LIR, verifier, and BIR constraint syntaxes are
   currently accepted, and where exactly is explicit-register meaning lost or
   rejected?

   Required answer shape:
   - end-to-end source/LIR/BIR trace with concrete symbols and tests
   - accepted versus rejected token classification, including clobber semantics
   - exact earliest ownership/discontinuity boundary

2. `02_narrow_support_decision.md`

   Question: Can one legitimate RV64 explicit-register operand family be added
   without broad parser or inline-assembly redesign?

   Required answer shape:
   - comparison of plausible syntax and ownership options
   - required structured metadata, validation, malformed-input behavior, and proof
   - a narrow implementation-idea outline or evidence-backed stop decision

## Required Documentation Output

Create exactly:

- `docs/rv64_explicit_register_inline_asm/index.md`
- `docs/rv64_explicit_register_inline_asm/01_source_to_bir_syntax_flow.md`
- `docs/rv64_explicit_register_inline_asm/02_narrow_support_decision.md`

## In Scope

- Source/LIR/verifier/BIR inline-assembly constraint tracing.
- RV64 explicit-register operand syntax and structured semantic metadata.
- Distinguishing operand constraints from clobbers and post-regalloc homes.
- A narrow support-or-stop decision with deterministic proof boundaries.

## Out Of Scope

- Implementation or test expectation changes.
- Regalloc constraint admission/enforcement, publication, or target emission.
- Arbitrary value-name register controls.
- Multi-target syntax expansion or broad inline-assembly redesign.

## Acceptance Criteria

- The documentation directory contains one index plus exactly the two named
  answer files.
- Current accepted/rejected constraint flow and the earliest discontinuity are
  evidenced with concrete code and tests.
- The result chooses one narrow legitimate syntax/owner or concludes with
  evidence that idea 724 must remain parked.
- Clobbers are not represented as named-value allocation constraints.
- No implementation, expectation, runtime behavior, or unrelated lifecycle
  history is changed.

## Reviewer Reject Signals

- Reject `{xN}` support asserted solely from the later home-validation parser
  without proving source/LIR semantic preservation.
- Reject clobber `~{xN}` repurposed as an input/output value constraint.
- Reject fixture flags, named values, known registers, allocator pressure, or
  manually injected BIR metadata claimed as language capability.
- Reject expectation downgrades, unsupported relabeling, helper renames, or
  classification-only changes claimed as semantic support.
- Reject broad GCC/LLVM syntax, multi-target, emission, ABI, regalloc, or
  publication changes outside the research boundary.
- Reject retaining the exact `unsupported_constraint` discontinuity behind a
  new metadata field name.
