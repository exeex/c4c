# Prepared Indirect Call String Argument Facts

Status: Open
Created: 2026-05-19

## Goal

Preserve string-constant pointer identity for indirect-call arguments in the
BIR/prepared handoff so target call lowering can consume semantic facts instead
of guessing from AArch64 assembly or c-testsuite source shape.

## Why This Exists

Idea 309's Step 2 AArch64 repair is blocked because the `00189.c` outer
indirect call loses the identity of its string argument before AArch64 lowering.
The LLVM/LIR input still contains `%t2 = getelementptr ... @.str1`, and the
outer indirect call passes `%t2`, but `--dump-bir` has no `%t2` producer and
`--dump-prepared-bir --mir-focus-function main` reports the argument only as an
ordinary register source with no prepared string-constant materialization for
`.str1`.

Direct-call string arguments already have a prepared string-constant argument
materialization path. Indirect-call arguments need the same semantic producer
fact before idea 309 can safely resume AArch64 call-boundary repair.

## In Scope

- Trace the existing direct-call string argument materialization path from LIR
  `getelementptr @.str*` through BIR and prepared addressing.
- Add the missing BIR/prepared producer or rewrite path for string-constant
  pointer arguments passed to indirect calls.
- Preserve `PreparedAddressMaterializationKind::StringConstant`, text identity,
  result value identity, byte offset, and address-space facts for the indirect
  call argument.
- Add focused prepare/handoff tests that prove indirect calls get the same
  semantic string argument facts that direct calls already get.
- Record proof sufficient for the supervisor to reactivate idea 309.

## Out of Scope

- AArch64 indirect callee register placement, call-register shuffling, final
  `blr` preservation, or assembly printing repairs from idea 309.
- Direct multi-argument shuffle bugs around `00181.c` and `00182.c`.
- Direct-call vararg aliasing or materialization bugs around `00200.c`.
- Address-of-local direct-call argument preparation around `00218.c`.
- Generic string/pointer/store/control materialization buckets that are not
  needed to publish indirect-call string argument facts.
- Expectation, allowlist, unsupported-classification, CTest registration,
  runner, timeout-policy, proof-log, or test-contract changes.

## Acceptance Criteria

- An indirect-call argument sourced from LIR `getelementptr @.str*` retains a
  semantic string-constant producer in BIR or in the prepared fact stream before
  target lowering.
- `--dump-prepared-bir --mir-focus-function main` for the focused `00189.c`
  shape publishes a prepared string-constant address materialization for the
  outer indirect call's `.str1` argument instead of only an ordinary register
  source.
- Focused tests cover indirect-call string argument publication without
  depending on the `00189.c` filename, `stdout`, `fprintfptr`, a fixed argument
  index, or a one-string-constant heuristic.
- Direct-call string argument materialization remains covered and
  non-regressed.
- Supervisor-selected proof is recorded before switching lifecycle state back
  to idea 309.

## Reviewer Reject Signals

Reject the route if it:

- repairs only `00189.c`, `stdout`, `fprintfptr`, `.str1`, a hard-coded
  argument index, or a single fixture shape instead of preserving indirect-call
  string argument facts semantically;
- teaches AArch64 lowering to infer string constants from assembly text,
  source text, nearby rodata labels, or missing prepared facts;
- claims progress through expectation, allowlist, unsupported-classification,
  CTest registration, runner behavior, timeout policy, proof-log, or
  test-contract changes;
- broadens into idea 309's AArch64 callee/register preservation, direct
  multi-argument shuffle, direct vararg aliasing, address-of-local direct-call
  preparation, or unrelated runtime buckets;
- breaks or bypasses the existing direct-call string argument materialization
  contract while adding the indirect-call path;
- hides the missing producer behind helper renames or diagnostics while
  prepared call arguments still lack string-constant identity.
