# LIR To BIR Packed Bitfield Scalar Binop Semantics

Status: Closed
Type: Producer-side semantic lowering follow-up
Parent: `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`
Split From: `ideas/closed/409_prepared_packed_fp128_global_initializer_admission.md`

## Goal

Repair semantic LIR-to-BIR lowering for bitfield-style scalar operations over
packed aggregate fields so `20040709-2.c` can advance past the current
`scalar-binop semantic family` residual.

## Why This Exists

Idea 409 repaired the first supported packed integer aggregate global
initializer family. The old bootstrap global admission diagnostic for
`tests/c/external/gcc_torture/src/20040709-2.c` is gone.

The current first residual is before prepared handoff:

```text
semantic lir_to_bir function 'fn1A' failed in scalar-binop semantic family
```

The failing function performs bitfield-style scalar operations over packed
`%struct.A`: `load i16`, `lshr i16`, `and i16`, `zext i16 to i32`, `add i32`,
`trunc i32 to i16`, `shl i16`, `or i16`, and `store i16`. This is a semantic
function-lowering boundary after global initializer admission, not a remaining
packed/`fp128` global initializer problem and not an RV64 object-emission
packet.

## In Scope

- Classify the `fn1A` scalar-binop semantic LIR-to-BIR failure in
  `20040709-2.c`.
- Identify the missing support for the bitfield-style integer operation chain:
  logical shift, mask, zero-extension, add, truncation, shift-left, or OR over
  the packed field storage value.
- Repair semantic LIR-to-BIR lowering for the first supportable scalar-binop
  family without relying on testcase shape.
- Preserve width, signedness, truncation, and packed-field storage semantics.
- Prove `20040709-2.c` advances past the `fn1A` scalar-binop residual or
  routes the next residual precisely.

## Out Of Scope

- Reopening packed aggregate global initializer admission from idea 409.
- Reopening RV64 object-route scalar compare/trunc or instruction-fragment
  lowering unless fresh evidence reaches those target consumers.
- Teaching RV64 object emission to infer or repair semantic scalar-binop facts.
- Broad bitfield frontend rewrites outside the observed semantic LIR-to-BIR
  lowering boundary.
- Expectation rewrites, unsupported downgrades, allowlist filtering, or
  testcase-specific shortcuts.

## Acceptance

- `src/20040709-2.c` no longer stops first at
  `semantic lir_to_bir function 'fn1A' failed in scalar-binop semantic family`
  for the supportable bitfield-style scalar-binop chain.
- Focused semantic/backend coverage proves the repaired scalar-binop family
  preserves width, signedness, truncation, and packed-field storage behavior.
- Any later prepared, RV64 object-route, link, qemu, runtime, or unrelated
  semantic boundary is routed to an existing or new owner instead of being
  absorbed into this idea.
- Existing backend and semantic lowering coverage remains green.

## Reviewer Reject Signals

- Reject filename-specific handling for `20040709-2.c`, function-specific
  handling for `fn1A`, or matching exact `%struct.A` names.
- Reject changes that widen, drop, or reorder bitfield scalar operations while
  merely clearing the diagnostic.
- Reject RV64 object-emitter changes claimed as progress for this pre-prepared
  semantic residual.
- Reject broad scalar rewrites that do not directly prove the observed
  scalar-binop semantic family.
- Reject expectation downgrades, unsupported markers, allowlist filtering, or
  diagnostic-only churn claimed as capability progress.

## Lifecycle Notes

- 2026-06-27: Closed after semantic LIR-to-BIR repair in implementation commit
  `84f868c3` admitted I16 scalar-binop immediates for the packed bitfield
  operation chain without touching RV64 object emission.
- 2026-06-27: Closure proof for `20040709-2.c` reported `dump_bir_status=0`,
  `prepared_status=0`, and `c4cll_status=0`. The old
  `semantic lir_to_bir function 'fn1A' failed in scalar-binop semantic family`
  residual is absent, fresh semantic BIR preserves the expected `lshr`, `and`,
  `zext`, `add`, `trunc`, `shl`, and `or` packed-bitfield operations, and the
  prepared dump includes `prepared.summary @fn1A` / `prepared.func @fn1A`.
- 2026-06-27: The representative now succeeds through the RV64 object route
  with no later residual in the accepted Step 3 proof.
- 2026-06-27: Close gate passed with the backend regression guard over
  `ctest --test-dir build -j --output-on-failure -R '^backend_'`. The
  rolled-forward `test_before.log` and regenerated `test_after.log` both
  reported 326/326 passing backend tests with no new failures.
