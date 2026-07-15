# LIR Aggregate SSA Producer Authority Publication

Status: Open
Type: bounded LIR aggregate producer/operand provenance prerequisite
Blocks: `ideas/open/754_lir_aggregate_vector_value_identity_convergence.md` Step 2

## Goal

Publish checked native `LirValueId` authority for the two selected non-call
aggregate SSA producer paths—local load and constructed `LirInsertValueOp`—so
754 can validate `LirExtractValueOp.agg` without display-text recovery or a
raw fallback.

## Why This Exists

754 and closed 798 accepted only a direct-composite-call provenance handoff.
Fresh focused failures show real extracts can instead consume a valid local
load or constructed aggregate result. The current verifier accepts structured
SSA extracts only when their ID selects a matching `LirCallOp` result; treating
these values as raw evades the new authority rule and cannot be accepted.

## In Scope

- Trace the exact local-load and constructed-`LirInsertValueOp` aggregate SSA
  producer paths reaching the failing extractvalue callers.
- Publish the smallest checked native producer/operand authority needed to
  carry each existing aggregate `LirValueId` to the downstream extract use.
- Define verifier checks for current-function producer ownership, aggregate
  type coherence, and compatibility-display mirroring for the selected paths.
- Add nearby positive and malformed coverage, then publish the exact 754
  handoff and focused proof route.

## Out Of Scope

- `LirExtractValueOp` result/schema, index, anonymous-layout, or selected
  element-type validation; 754 retains that row work after this handoff.
- Broad expression API redesign, generic producer-family conversion, other
  aggregate/vector rows, Raw-BIR, lowering, MIR, emission, or text recovery.
- Weakening verification or classifying valid structured aggregate SSA values
  as raw merely to restore existing tests.

## Acceptance Criteria

- The selected local-load and constructed-insertvalue aggregate values retain
  native, checked current-function `LirValueId` authority to their intended
  extractvalue use.
- Missing, foreign, stale, wrong-producer-kind, or type-incoherent authority
  rejects without consulting rendered text or accepting raw fallback.
- Fresh build plus focused positive/malformed aggregate proof yields a precise
  754-consumable handoff; unrelated producer paths remain unchanged or
  fail-closed.

## Reviewer Reject Signals

- Reject broad operand/expression or producer-family rewrites claimed as the
  two selected aggregate producer paths.
- Reject parsing `%t`, printer output, rendered LLVM, instruction order, or
  testcase names to recover authority.
- Reject verifier weakening, raw fallback for structured aggregate SSA values,
  expectation downgrades, or named-case-only shortcuts.
- Reject adding extractvalue row index/layout/result-type rules, Raw-BIR
  receipt, or unrelated aggregate/vector conversion.
