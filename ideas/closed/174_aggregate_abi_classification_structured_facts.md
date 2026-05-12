# Aggregate ABI Classification Structured Facts

Status: Closed
Created: 2026-05-12
Closed: 2026-05-12

Depends On:
- `ideas/open/172_type_identity_authority_audit.md`

## Goal

Make aggregate ABI classification consume durable structured size, alignment,
layout, and aggregate-kind facts instead of deriving ABI class primarily from
rendered type or signature strings.

## Why This Idea Exists

The audit found aggregate ABI classification as a high-risk boundary because
byval, sret, HFA, direct-GP, memory-value, stack, and return classification can
still depend on spelling-derived layout data. BIR already records scalar type
kinds and ABI info such as `CallArgAbiInfo`, `CallResultAbiInfo`, byval/sret
flags, size, and alignment, but aggregate classification remains tied to
text-keyed layout bridges before structured metadata fully stabilizes. The
AArch64 direct LIR route is the largest spelling-authority island.

## In Scope

- Choose one bounded aggregate ABI classification boundary, such as BIR
  aggregate call classification or the AArch64 direct LIR aggregate route.
- Feed that boundary from structured aggregate layout facts and explicit ABI
  metadata when available.
- Preserve final ABI spelling and printer output as output surfaces, not
  semantic authority.
- Fail closed or report a clear unsupported metadata gap when generated
  metadata is present but inconsistent.
- Prove with build proof plus focused aggregate parameter/return, byval/sret,
  or HFA route coverage.

## Out Of Scope

- Retiring every AArch64 direct LIR text parser in one pass.
- Replacing ABI printers or final LLVM textual syntax.
- Reclassifying unrelated scalar call ABI behavior.
- Weakening backend expected output to match current spelling-derived behavior.

## Acceptance Criteria

- The selected aggregate ABI path uses structured layout/ABI facts for
  metadata-rich aggregate inputs rather than reparsing rendered signatures or
  instruction type strings as the first authority.
- byval, sret, HFA/direct, memory, stack, and return decisions touched by the
  slice are traceable to structured facts or to an explicit legacy fallback.
- Existing aggregate ABI tests remain strong and include at least one focused
  route that would catch a spelling-only classification collision.
- Any legacy/no-metadata fallback is named as compatibility and does not hide a
  structured metadata mismatch.
- The proof includes a fresh build or compile check plus focused backend CTest
  or route coverage.

## Completion Summary

The completed runbook selected fixed aggregate byval call arguments as the
bounded ABI classification boundary. That path now publishes structured
aggregate identity through LIR type-ref mirrors while preserving rendered LLVM
ABI spelling as output. BIR classification consumes the typed mirror
(`StructNameId` / aggregate `LirTypeRef`) when metadata is available, with
legacy compatibility limited to existing mirrorless paths.

Fail-closed metadata mismatch coverage was added for byval argument type-ref
mirror mismatches, and frontend coverage records byval argument type-ref
mirrors. Focused validation passed 6/6 targeted tests, broader
`backend_|frontend_lir` validation passed 113 executed tests, and the
supervisor accepted the full-suite baseline at commit `878d88413` with
3137/3137 passing tests. A close-time non-mutating regression guard parse over
the existing canonical focused log also passed.

Remaining compatibility note: older or mirrorless paths may still use the
named legacy fallback. That fallback is not the preferred authority for
metadata-rich aggregate ABI classification and should be retired by a separate
source idea if broader AArch64/direct-LIR parser removal becomes the target.

## Reviewer Reject Signals

- The implementation adds a new rendered signature, type string, or
  instruction text parser and calls the result structured ABI classification.
- A metadata-rich aggregate with missing or mismatched structured facts still
  takes the old text-derived byval/sret/HFA path silently.
- Backend ABI tests are weakened, unsupported, or expectation-rewritten without
  actual classification capability improvement.
- The diff only changes classification labels, debug output, or notes while
  the same rendered spelling still decides ABI class.
- The slice mixes unrelated scalar ABI rewrites into the aggregate identity
  boundary without focused proof.
