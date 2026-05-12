# Direct Call No-Prototype Variadic Signature Mismatch

Status: Open
Created: 2026-05-12
Blocks:
- `ideas/open/188_lir_bir_freeze_closure_gate.md`

## Goal

Repair the structured direct-call callee signature path so old-style,
no-prototype, and variadic C calls accepted by the frontend no longer fail
with `LirCallOp.callee_signature: structured callee signature does not match
call arguments`.

## Why This Idea Exists

Idea 188 reached milestone validation with a green accepted `test_before.log`
baseline, but the full-suite `test_after.log` run introduced 9 failures. The
shared failure mode is a frontend verification error in structured direct-call
signature metadata. The freeze gate cannot be closed while this generated
direct-call path rejects C calls whose declarations intentionally do not have a
fixed prototype matching every call argument.

## In Scope

- Identify how structured direct-call callee signatures are built and checked
  for direct calls.
- Repair old-style/no-prototype C declaration handling so argument-count
  flexibility is represented intentionally instead of tripping the structured
  signature verifier.
- Repair variadic direct-call handling where fixed parameters and extra call
  arguments must coexist.
- Preserve structured direct-call metadata for normal prototype calls.
- Prove the fix against the known 9 failing tests and nearby direct-call
  signature coverage chosen by the supervisor.
- Return to the idea 188 milestone gate only after the blocker is fixed and
  regression proof is green under the delegated command.

## Out Of Scope

- Backend restart implementation.
- Rewriting target MIR, assemblers, linkers, object emission, or unrelated
  backend output layers.
- Downgrading supported tests to unsupported or weakening expectations to make
  the milestone pass.
- Replacing structured callee signature metadata with rendered callee text.
- Broad refactors of the call-lowering pipeline beyond what is needed for this
  signature compatibility repair.

## Acceptance Criteria

- Old-style/no-prototype direct calls that are valid C no longer produce the
  structured callee signature mismatch error.
- Variadic direct calls preserve fixed-parameter checking while accepting the
  variadic tail arguments expected by the source program.
- Prototype direct calls still keep structured callee signature metadata and
  still reject genuine signature mismatches.
- The 9 milestone-validation failures from idea 188 are green under the
  supervisor-delegated proof command.
- A broader regression check selected by the supervisor is green before the
  blocker is treated as resolved.

## Reviewer Reject Signals

- The route adds named-test or exact-callee shortcuts for the 9 failing tests
  instead of modeling no-prototype or variadic call semantics.
- The route suppresses `LirCallOp.callee_signature` verification broadly or
  removes structured direct-call metadata for ordinary prototype calls.
- The route downgrades tests, rewrites expectations, or changes labels instead
  of repairing frontend/lowering behavior.
- The route treats rendered function names or printed signatures as semantic
  authority for generated direct calls.
- The route proves only one known failing testcase while leaving nearby
  no-prototype, old-style, or variadic direct-call cases unexamined.
- The exact old failure mode remains reachable behind renamed helpers or a new
  compatibility wrapper.
