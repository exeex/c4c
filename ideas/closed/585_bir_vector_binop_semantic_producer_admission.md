# BIR Vector Binop Semantic Producer Admission

Status: Closed
Type: Producer implementation follow-up
Parent: `ideas/closed/562_bir_scalar_binop_semantic_producer_admission.md`
Owning Layer: BIR semantic producer

## Goal

Decide and repair, or explicitly fail-close, BIR vector arithmetic binary
operation producer admission after vector function signatures have already
been admitted.

## Why This Exists

The scalar-binop producer lane repaired the confirmed F128 scalar arithmetic
boundary and closed with `src/960513-1.c` advancing beyond the original
scalar-binop failure. During closure, `src/simd-6.c` remained as a vector
arithmetic residual after small-vector signature admission. That residual is
not scalar-binop work and is not the same owner decision as wide-vector ABI
signature representation in
`ideas/open/563_bir_wide_vector_abi_signature_representation_owner_decision.md`.

This idea records the separate vector-binop owner boundary so scalar-binop
closure does not silently absorb vector arithmetic work.

## In Scope

- Inspect the actual BIR producer boundary for vector arithmetic binary
  operations once function signature facts are available.
- Add focused BIR coverage for supported vector binary opcode and operand fact
  publication, or for an explicit fail-closed vector-binop owner boundary.
- Prove `src/simd-6.c`, or a stronger current vector-binop substitute, advances
  past the vector-binop semantic producer boundary only when the BIR facts are
  correct.
- Record any newly exposed scalar-cast, local-memory, ABI, or RV64 lowering
  failures as downstream owner boundaries.

## Out Of Scope

- Scalar-binop F128 arithmetic repair already covered by the closed parent
  idea.
- Wide-vector function signature ABI carrier decisions covered by
  `ideas/open/563_bir_wide_vector_abi_signature_representation_owner_decision.md`.
- Scalar-cast, scalar/local-memory, alloca local-memory, RV64 object lowering,
  expectation rewrites, unsupported downgrades, allowlist changes, or
  classification-only movement.
- Broad vector or ABI rewrites that leave the same vector-binop producer
  ambiguity in place.

## Acceptance Criteria

- Focused BIR coverage demonstrates the vector-binop producer behavior or the
  explicit fail-closed owner boundary.
- Supported vector binary opcode and operand facts are published at the real
  instruction-lowering boundary before claiming producer progress.
- `src/simd-6.c`, or a stronger substitute, passes or advances beyond the old
  vector-binop semantic admission diagnostic because vector-binop BIR facts are
  correct.
- Any residual failures are documented as downstream owner boundaries rather
  than counted against scalar-binop closure.

## Closure Notes

Closed after selecting the explicit fail-closed owner boundary for fixed-vector
`LirBinOp`: BIR scalar `BinaryInst` facts are not published for LLVM fixed
vectors until a lane-aware vector-binop fact shape exists. Focused coverage
exercises a representative `<8 x i8> mul` fixture and checks the
`vector-binop semantic family` diagnostic.

Representative `src/simd-6.c` evidence remains at the intentional
`vector-binop semantic family` boundary. This is accepted as the documented
producer-owner decision for this idea, not downstream vector ABI or RV64
lowering progress.

Close-time backend regression guard passed with matching canonical logs:
`test_before.log` and `test_after.log` both report `346/346` backend tests
passing, with zero new failures.

## Reviewer Reject Signals

- Reject named-case handling for only `src/simd-6.c` or one vector spelling
  instead of a semantic vector-binop opcode or operand rule.
- Reject routing wide-vector signature carrier gaps into this idea instead of
  the dedicated wide-vector ABI signature owner decision.
- Reject fixes that only change expectation files, unsupported markers,
  allowlists, row classification, or outer failure-note text.
- Reject claiming scalar-binop progress from vector arithmetic movement, or
  claiming vector-binop progress while the same vector-binop admission failure
  remains behind renamed helpers.
- Reject broad scalar, vector, or ABI rewrites not justified by vector-binop
  producer evidence.
