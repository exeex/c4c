Status: Active
Source Idea Path: ideas/open/562_bir_scalar_binop_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Scalar-Binop Evidence

# Current Packet

## Just Finished

Step 1 refreshed scalar-binop evidence for
`ideas/open/562_bir_scalar_binop_semantic_producer_admission.md`.

Representative outcomes:
- `src/960513-1.c`, function `f`: still fails at the original BIR
  `scalar-binop semantic family` boundary. Current LLVM has `fneg fp128`,
  repeated `fmul fp128`, and `fsub fp128`; the first owned producer gap is
  F128 scalar-binop operand/opcode admission rather than outer failure
  publication.
- `src/simd-6.c`, function `foo`: still reports `scalar-binop semantic
  family`, but the operation is `mul <8 x i8>` and should be treated as a
  vector-binop owner-boundary candidate, not proof of ordinary scalar-binop
  closure.
- `src/960327-1.c`, `src/960402-1.c`, `src/960608-1.c`: semantic BIR dump now
  succeeds; their RV64 object path failures are downstream object instruction
  fragment boundaries.
- `src/960521-1.c`: fails later in `store local-memory semantic family`.
- `src/pr60960.c`: remains downstream in `scalar/local-memory semantic family`.
- `src/20050316-3.c`: remains downstream in `scalar-cast semantic family`.

Existing focused BIR coverage already documents fail-closed F128 scalar
constant binops plus admitted I16 and F32 scalar-binop publication, so Step 1
does not prove closure readiness. It exposes a real Step 2 decision point for
F128 scalar-binop producer admission.

## Suggested Next

Execute Step 2 as a focused producer packet: decide whether F128 scalar-binop
admission belongs in this scalar producer lane, then either publish the missing
F128 operand/opcode facts with focused BIR coverage or record an explicit
fail-closed owner boundary for F128 arithmetic that is strong enough for a
plan-owner route decision.

## Watchouts

- Do not edit expectations, unsupported markers, allowlists, classifications,
  or the outer `latest function failure` note as evidence of progress.
- Do not route this packet into scalar-control-flow, function-signature, RV64
  lowering, or object-emission work before BIR scalar-binop publication is
  proven correct.
- `src/960513-1.c` is not a named-case fix target; any repair must be a general
  F128 scalar-binop operand/opcode rule or an explicit owner-boundary decision.
- `src/simd-6.c` is vector arithmetic after small-vector signature admission;
  do not use it to justify scalar-binop closure or scalar-only F128 work.

## Proof

Ran:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`

Result: backend subset passed; `test_after.log` contains the fresh proof.
