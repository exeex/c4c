# LIR Stack-Restore Lifetime-Consumer Authority

Status: Open
Type: bounded LIR producer/schema/verifier authority publication
Predecessor: `ideas/open/794_lir_next_local_vla_authority_handoff.md`
Future consumer: `ideas/open/794_lir_next_local_vla_authority_handoff.md`

## Goal

Select, publish, and verify exactly one native `LirStackRestoreOp` authority
contract: the saved-pointer binding plus a structured lifetime-consumer
transition that makes this one operation admissible without deriving facts
from presentation.

## Why This Exists

794's accepted Step 1 evidence shows that stack restore already has a native
saved-pointer/object/owner/type/liveness binding, but lacks a selected-row
admission and structured lifetime-consumer/transition facts.  This missing
producer/schema/verifier contract must be resolved before 794 can publish a
one-row local/VLA handoff.

## In Scope

- Choose the single `LirStackRestoreOp` admission and its native structured
  saved-pointer, object, current-function owner, pointer/pointee type,
  liveness, and lifetime-consumer transition facts.
- Publish the minimum producer/schema representation and verifier checks that
  reject malformed, foreign, type-incoherent, non-live, or transition-invalid
  stack-restore authority.
- Provide focused same-feature positive and negative proof, then document one
  exact handoff to 794 without claiming Raw-BIR receipt or implementation
  beyond the selected authority contract.

## Out Of Scope

- Raw-BIR, importer, verifier receipt by 734, lowering, or any 734 packet.
- Dynamic VLA allocation or typed count work, VLA GEP, local load/store/GEP,
  local temporaries, other lifetime-consumer rows, or multiple local rows.
- Presentation-derived facts, rendered operands, LLVM text, testcase-shaped
  classification, and broad lifetime-model redesign.

## Acceptance Criteria

- One explicit `LirStackRestoreOp` admission binds its saved pointer to native
  current-function object/owner/type/liveness facts and structured
  lifetime-consumer transition facts.
- Focused same-feature producer/verifier positive and negative proof accepts
  the valid selected restore and rejects malformed, foreign, type-incoherent,
  non-live, or invalid-transition authority.
- The published handoff identifies the exact native fields, rejected forms,
  and proof so 794 can resume at Step 2; it does not authorize a 734 receipt.

## Reviewer Reject Signals

- Reject a selected restore inferred from operand spelling, rendered LIR or
  LLVM text, testcase identity, `monostate`, or an unstructured lifetime
  classification.
- Reject a change that merely renames existing saved-pointer authority while
  retaining no selected admission or no structured consumer/transition fact.
- Reject dynamic-VLA count work, VLA GEP, another local/lifetime row,
  Raw-BIR/importer/734 work, broad lifetime rewrites, weakened expectations,
  or named-case-only verifier shortcuts as stack-restore capability progress.
