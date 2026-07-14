# Production Member/Bitfield Rvalue Value Identity Publication

Status: Open (active upstream blocker for
`ideas/open/764_lir_production_computed_goto_addr_value_publication.md`)
Type: bounded production LIR rvalue authority repair
Predecessor: `ideas/open/764_lir_production_computed_goto_addr_value_publication.md`,
interrupted at Step 1

## Goal

Make the first production member/bitfield rvalue identity-loss seam reached by
`tests/c/external/gcc_torture/src/comp-goto-1.c` publish a structured,
current-function `LirValueId` for `insn.f1.offset`, so it can serve as the RHS
index of the computed-goto address pointer-add GEP.

## Why This Exists

The active 764 trace established that `IndirBrStmt` correctly copies
`emit_rval_operand(...).value_id()` into `LirIndirectBrOp.addr_value`. The
production target is `base_addr + insn.f1.offset`; its binary lowering routes
through `emit_indexed_gep`, but the RHS member/bitfield rvalue reaches that
seam without an ID. Therefore the GEP result cannot honestly become
authoritative. A GEP-only publication route was rejected because
`verify_authoritative_gep` correctly requires fully authoritative inputs and
the focused guard rejects the raw RHS index.

This is the first upstream producer seam confirmed by the production
expression. It is separate from 764's computed-goto carrier publication and
from 734's Raw-BIR receiver.

## In Scope

- Trace `comp-goto-1.c` only far enough to name the first member/bitfield
  rvalue producer that drops the structured value identity for
  `insn.f1.offset`.
- Repair that one confirmed owner so the emitted rvalue carries a valid
  current-function `LirValueId` into the RHS index of `emit_indexed_gep`.
- Preserve and exercise the existing authoritative-GEP rule: base and index
  must both be valid, current-function authority before the result receives an
  ID.
- Add focused producer coverage for this production member/bitfield rvalue
  seam and hand off the structured identity needed by 764.

## Out Of Scope

- `LirIndirectBrOp`, Raw-BIR/importer, 734's receiver, or 764's address-carrier
  publication work.
- Publishing a GEP ID from only its base, accepting a partial/raw index, or
  weakening `verify_authoritative_gep`.
- Parsing rendered operands, labels, printers, LLVM, or testcase text.
- General member, bitfield, rvalue, aggregate, local/object, pointer, memory,
  PHI, parameter, target-lowering, or emission redesign; stop after the first
  production owner confirmed by this expression.

## Acceptance Criteria

- The production `insn.f1.offset` member/bitfield rvalue for `comp-goto-1.c`
  reaches the pointer-add RHS with a valid current-function `LirValueId`.
- The GEP authority rule remains fail-closed for a missing, invalid, foreign,
  or raw index; no partial-authority result publication is introduced.
- Focused producer coverage proves both the positive production seam and the
  malformed-index rejection.
- The handoff identifies the confirmed owner, structured field/ID, proof, and
  exact return action: resume 764 Step 1 and reattempt only computed-goto
  address publication; do not resume 734 directly.

## Reviewer Reject Signals

- Reject a GEP-only ID, a verifier relaxation, raw-index acceptance, or any
  patch that retains the identity-less RHS behind a different abstraction.
- Reject parsing display text, labels, rendered LLVM/printer output, or
  testcase-specific logic to derive identity.
- Reject changes to Raw-BIR/importer, `IndirBrStmt`, computed-goto carrier
  handling, or the 734 receiver claimed as progress for this producer seam.
- Reject a broad member/bitfield or rvalue redesign, unrelated authority
  family work, named-case workaround, weakened expectation, or test-only
  classification change.
