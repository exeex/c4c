# LIR Stack-Restore Lifetime-Consumer Authority

Status: Closed (capability complete)
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

## Closure Record

Disposition: capability complete. Exactly one selected `LirStackRestoreOp`
authority contract is now published: the native saved-pointer/object/owner/
pointer-type/pointee-type/live checkpoint binding plus the structured
`RestoreSavedVlaStackCheckpoint` transition. The verifier admits only the
selected native row and rejects malformed, foreign, type-incoherent, non-live,
or transition-invalid authority without presentation-derived recovery.

Accepted implementation and handoff commits are `cdeacb2cd` and `f5cfa52b7`;
`8bd881842` records the contract definition. Supervisor acceptance includes a
fresh build, focused `^frontend_lir_call_type_ref$` proof passing 1/1, its
non-decreasing 1/1 guard, and broader frontend smoke proof. The exact selected
fields, rejected forms, proof, and return boundary are durable in
`docs/lir_local_operation_authority/handoff_to_734.md`.

The predecessor returns to
`ideas/open/794_lir_next_local_vla_authority_handoff.md` at **Step 2 - Publish
and verify the selected producer authority**. It may consume only this exact
stack-restore handoff to complete its one-row authority process; it must not
perform Raw-BIR/importer/734 receipt, dynamic-VLA count work, VLA GEP, or any
other local/VLA row.
