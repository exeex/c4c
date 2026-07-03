# RV64 Pointer Arithmetic Lowering

Status: Open
Type: Capability repair
Parent: `ideas/open/570_rv64_unsupported_instruction_fragment_owner_diagnostics.md`
Owning Layer: RV64 object lowering for pointer-valued binary instructions

## Goal

Implement RV64 object-route lowering for pointer-valued BIR binary arithmetic
where a pointer base is combined with a scaled byte offset and the result is
published as a pointer owner.

## Why This Exists

The Step 3 diagnostics from the 570 runbook identified `src/20000819-1.c` as
a distinct pointer arithmetic owner family:

- `function=foo`
- `instruction_kind=BinaryInst`
- `owner=ptr %t4`
- first unsupported instruction is a pointer add of a loaded base plus a
  scaled byte offset.

This is a pointer/object-emission capability gap, not evidence for call,
inline asm, select, floating-point, prepared authority, or runtime comparison
work.

Evidence:

- `build/agent_state/570_unsupported_instruction_fragment_diagnostics/classification.tsv`
- `build/agent_state/570_unsupported_instruction_fragment_diagnostics/src_20000819-1.c/dump-prepared-bir.txt`
- `build/agent_state/570_unsupported_instruction_fragment_diagnostics/src_20000819-1.c/object-route.log`

## In Scope

- RV64 object emission for pointer-valued binary add/subtract forms that lower
  to integer address arithmetic.
- Correct publication of the resulting pointer owner for later memory uses.
- Focused tests for loaded-base plus scaled-offset pointer arithmetic.
- Precise diagnostics for unsupported pointer arithmetic forms.

## Out Of Scope

- General integer ALU expansion beyond what pointer address arithmetic needs.
- Same-module calls, inline asm carriers, select lowering, FP binary lowering,
  or runtime comparison work.
- BIR producer rewrites unless focused proof shows the pointer arithmetic
  representation is semantically wrong.
- Filename-specific handling for `src/20000819-1.c`.

## Acceptance Criteria

- The observed pointer add shape no longer reaches generic
  `unsupported_instruction_fragment`, or it fails with a narrower pointer
  arithmetic diagnostic naming the unsupported operation and operand types.
- Focused backend tests cover pointer base plus scaled integer offset and
  result publication to a pointer owner.
- Unsupported pointer arithmetic forms remain fail-closed with specific
  diagnostics.
- No unrelated RV64 lowering family is changed as part of the acceptance
  proof.

## Reviewer Reject Signals

- Reject lowering that handles only the exact `%t4` value, `foo` function, or
  `src/20000819-1.c` filename.
- Reject expectation rewrites, unsupported-marker edits, allowlist changes, or
  runtime-output changes claimed as pointer arithmetic progress.
- Reject a broad integer ALU rewrite that does not prove pointer owner
  publication and later memory-use compatibility.
- Reject mixing pointer arithmetic with call, inline asm, select, or FP binary
  fixes in one completion claim.
- Reject leaving the old generic fallback as the first failure for the
  loaded-base plus scaled-offset pointer add shape.
