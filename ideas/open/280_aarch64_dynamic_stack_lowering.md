# AArch64 Dynamic Stack Lowering

Status: Open
Created: 2026-05-18

## Intent

Implement real AArch64 lowering for prepared dynamic-stack operations that are
currently rejected with the truthful diagnostic:
`AArch64 dynamic-stack helper lowering is not implemented`.

## Why This Exists

Idea 279 repaired the backend-symbol authority bug by preventing unresolved
LLVM stack helper calls such as `llvm.stacksave`, `llvm.dynamic_alloca.i8`, and
`llvm.stackrestore` from reaching generated AArch64 output. The remaining
`00207.c` blocker is no longer an unresolved-symbol failure; it is a distinct
capability gap where AArch64 does not yet lower dynamic stack save, allocation,
and restore semantics.

## In Scope

- Trace the prepared dynamic-stack operation records consumed by AArch64
  backend lowering.
- Define the AArch64 machine lowering for stack save, dynamic allocation, and
  stack restore when the prepared representation has enough information.
- Preserve a truthful diagnostic for any dynamic-stack form that still lacks
  sufficient lowering facts.
- Prove the behavior with focused backend tests and the relevant AArch64
  c-testsuite backend subset, including `00207.c`.

## Out of Scope

- Reintroducing LLVM helper calls as external symbols in AArch64 output.
- Reopening completed call-boundary move, `.LBB...` label, or helper-symbol
  authority fixes unless their exact old diagnostics return.
- Configuring an AArch64 runtime runner or claiming runtime pass evidence.
- Weakening c-testsuite expectations, allowlists, unsupported classifications,
  or stage accounting.
- Broad frontend or ABI rewrites unless tracing proves the prepared
  dynamic-stack facts are insufficient before AArch64 lowering.

## Acceptance Criteria

- Prepared dynamic-stack operations lower to valid AArch64 machine output for
  the supported forms covered by the focused proof.
- Generated AArch64 output for the representative subset contains no unresolved
  `llvm.stacksave`, `llvm.dynamic_alloca.*`, or `llvm.stackrestore`
  references.
- The `00207.c` AArch64 backend c-testsuite path advances beyond the current
  dynamic-stack unsupported diagnostic to the next truthful owner layer.
- Any remaining unsupported dynamic-stack form fails before machine output with
  a specific owner-layer diagnostic.
- `[RUNTIME_UNAVAILABLE]` is not counted as pass evidence.

## Reviewer Reject Signals

Reject the route or slice if it:

- restores unresolved LLVM helper calls in generated AArch64 assembly or link
  inputs;
- matches only `00207.c` or one helper spelling instead of lowering the prepared
  dynamic-stack operation family;
- claims completion only by weakening diagnostics, expectations, allowlists, or
  unsupported classifications;
- treats missing AArch64 runtime execution as a pass;
- rewrites unrelated ABI, call-boundary, label, scalar-printer, or frontend
  paths to mask the dynamic-stack lowering gap;
- loses the truthful fail-closed behavior for dynamic-stack forms that are not
  actually supported yet.
