# LIR Computed-Goto Address Value Identity Publication

Status: Open (active blocker for idea 734's next bounded computed-goto receiver)
Type: bounded LIR computed-goto producer authority repair
Predecessor: `ideas/open/734_lir_to_new_bir_container_completeness.md`

## Goal

Publish verifier-checked current-function `LirValueId` authority for each
active `LirIndirectBrOp` address so one later Raw-BIR computed-goto receiver
packet can consume it with the already typed ordered successors.

## Why This Exists

Closed idea 750 established ordered `LirIndirectBrOp.successors`, but `addr`
is only a `LirOperand` spelling. Idea 734 cannot reconstruct pointer identity
from that carrier, so its next valid computed-goto receiver row is blocked at
the LIR producer boundary.

## In Scope

- add the smallest typed address-value carrier to active `LirIndirectBrOp`
- populate it from existing current-function pointer authority without parsing
  operand text
- verify presence, validity, current-function ownership, and pointer
  suitability before downstream use
- retain `addr` only as a checked display mirror
- add focused positive and malformed-authority coverage plus an exact handoff
  to idea 734

## Out Of Scope

- Raw-BIR indirect-jump container, importer, verifier, or receiver work; that
  remains idea 734
- computed-goto successor changes already completed by idea 750
- legacy `LirIndirectBr`, `LirCondBr`, `LirSwitch`, PHI, local/object,
  memory/va, aggregate/vector, parameter, or other identity work
- operand/label/printer-text recovery, canonicalization, target lowering, MIR,
  or emission

## Acceptance Criteria

- Every active `LirIndirectBrOp` address controlling computed-goto flow has
  valid current-function typed pointer identity independent of `addr` text.
- Missing, invalid, foreign, or non-pointer address authority rejects before
  printing or downstream consumption, with no text fallback.
- Focused positive and negative tests prove misleading address display text
  cannot select or repair the semantic address.
- The handoff names the typed address field, existing ordered successor field,
  fail-closed boundaries, and proof required for one 734 receiver packet.

## Reviewer Reject Signals

- Reject parsing `addr`, rendered LLVM, printer output, labels, or testcase
  names to derive address identity or type.
- Reject a named-case-only fix, expectation downgrade, or display agreement
  claimed as semantic progress.
- Reject changes to Raw-BIR receiver code, computed-goto successor authority,
  conditional/switch authority, or unrelated LIR families under this bounded
  producer source.
- Reject a carrier that leaves missing, foreign, or non-pointer address failure
  reachable behind a renamed field or permits downstream fallback.
