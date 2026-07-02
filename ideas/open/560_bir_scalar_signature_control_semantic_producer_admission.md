# BIR Scalar Signature Control Semantic Producer Admission

Status: Open
Activation Priority: Deferred until higher-frequency BIR semantic lanes are routed, unless the supervisor explicitly selects this smaller lane.
Type: Producer implementation follow-up
Parent: `ideas/closed/545_bir_semantic_producer_admission_reconstruction.md`
Owning Layer: BIR semantic producer

## Goal

Repair or split BIR scalar, function-signature, and control-flow semantic
producer admission for the `20` exact current rows in this smaller lane.

## Why This Exists

The evidence reconstruction found `10` scalar-control-flow rows, `9`
function-signature rows, and `1` scalar-binop row. The lane is real but lower
frequency than local-memory, call metadata, and runtime/intrinsic memory
facts, so it is recorded as a separate deferred implementation idea instead of
being merged into larger routes.

## In Scope

- Inspect whether scalar-control-flow, function-signature, and scalar-binop
  admission failures share a BIR semantic producer boundary.
- Split this idea before implementation if producer inspection proves the
  three topics are independent.
- Add focused BIR tests for each retained topic.
- Prove with RV64 representatives including `src/20000314-3.c`,
  `src/20050316-3.c`, and `src/960513-1.c`, or current stronger substitutes.

## Out Of Scope

- Mixing this lane into local-memory because some cases later touch memory.
- Treating function-signature failures as ABI/RV64 lowering before BIR
  publication is proven.
- Claiming broad scalar progress from the single scalar-binop row alone.
- Expectation rewrites, unsupported downgrades, allowlist changes, or weaker
  semantic admission checks.

## Acceptance Criteria

- Producer inspection either proves a shared scalar/signature/control boundary
  or splits the lane into smaller source ideas.
- Each retained topic has focused BIR coverage.
- Representative RV64 rows advance because BIR facts are published correctly.
- The deferred activation state is resolved before implementation starts.

## Reviewer Reject Signals

- Reject named-case repair of only `src/960513-1.c` as scalar lane progress.
- Reject routing signature failures to ABI/RV64 lowering without proving BIR
  publication is already correct.
- Reject merging this lane into local-memory, call, or runtime/intrinsic work
  without row and code evidence of a shared producer boundary.
- Reject expectation or unsupported-marker downgrades as producer progress.
- Reject broad scalar rewrites that retain the same semantic admission failure.
