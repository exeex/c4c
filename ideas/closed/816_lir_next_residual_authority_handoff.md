# LIR Next Residual Authority Handoff

## Intent

Select and publish exactly one next residual LIR instruction, terminator, or
inline-assembly authority row after the accepted bounded cast-result work in
idea 796, so a later 734 Raw-BIR receiver packet has a checked structured
handoff.

## Why This Exists

The active 734 receiver route is paused after its accepted module/type and
aggregate/vector prerequisites. Its next remaining residual family requires a
producer-owned, one-row structured authority contract; it must not recover
identity or type facts from rendered operands, labels, templates, or LLVM
text. Open 796 records its selected cast route as complete and has no
executable next packet, so this successor owns a fresh, isolated selection.

## In Scope

- Audit residual instruction, terminator, and inline-assembly candidates only
  to select one row with already-native or narrowly publishable value, edge,
  object, and type authority.
- Publish and verify the selected row's current-function ownership, exact type
  facts, and malformed/foreign/missing-authority rejection.
- Record a precise handoff for 734 containing the selected row, allowed facts,
  rejected forms, focused proof, and exact receiver return point.

## Out Of Scope

- Raw-BIR destination/importer/verifier changes or 734 receiver work.
- A residual-family sweep, reimplementation of 796's accepted cast paths,
  CFG/PHI work, aggregate/vector authority, body parameters, module shadows,
  or parsing inline-assembly templates/constraints.
- Rendering changes, target lowering, MIR, test expectation weakening, or
  display-text-derived identity.

## Acceptance Criteria

- Exactly one explicit residual row has a structured producer/schema/verifier
  authority contract with focused positive and malformed-authority proof.
- The handoff names the one permitted 734 receiver row and preserves all
  nonselected residual forms as fail closed.
- The repository builds and the selected focused frontend/backend proof passes.

## Reviewer Reject Signals

- The change parses rendered text, labels, templates, or constraints to create
  semantic identity or type facts.
- More than one residual family is enabled, or the patch absorbs a Raw-BIR
  receiver, CFG/PHI, body-parameter, aggregate/vector, or module-shadow route.
- Tests weaken contracts, special-case a named testcase, or accept malformed,
  missing, foreign, or incoherent authority.
- The handoff omits the exact selected row, current-function ownership,
  rejected forms, focused proof, or 734 return point.

## Closure Decision

Close accepted — capability complete. This source's one-row producer/schema/
verifier authority contract is satisfied and is intentionally limited to the
following durable handoff:

- The sole permitted future receiver row is scalar-integer output-only
  `LirInlineAsmOp.ordinary_results[0]` emitted by
  `StmtEmitter::emit_inline_asm`.
- The receiver may consume only its fresh native value ID, the typed `Output`
  binding at index 0, and its `LirTypeRef`.
- The verifier authority is the exact tuple with a valid native ID owned by the
  current function. Missing, invalid, duplicate, role-mismatched,
  index-mismatched, type-mismatched, and foreign-authority forms reject.
- Every nonselected inline-assembly form remains fail closed. Compatibility
  `result` is presentation-only and cannot establish semantic authority.

Accepted evidence: Step 2 audit commit `839a97804`; a fresh
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir_interface$'`
passed; matching fresh `test_before.log` and `test_after.log` regression guard
outputs passed 1/1.

Successor and exact return point: no Raw-BIR receipt occurred in this source.
Return to existing open
`ideas/open/734_lir_to_new_bir_container_completeness.md` for one repaired,
bounded receiver packet that receives only the structured facts listed above.
That packet must add only the necessary typed Raw-BIR destination/importer/
reachable-verifier work and prove positive plus malformed-authority rejection;
it must not recover identity or type from `result` or any presentation text,
and it must leave all other inline-assembly forms fail closed.
