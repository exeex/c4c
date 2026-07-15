# LIR Operand Provenance Authority Publication

Status: Closed — capability complete
Type: bounded LIR operand/expression provenance prerequisite
Blocks: `ideas/open/754_lir_aggregate_vector_value_identity_convergence.md`

## Goal

Preserve native SSA `LirValueId` provenance when surrounding HIR expression
APIs carry an SSA operand through to LIR construction, so a downstream bounded
producer can consume structured use identity without recovering it from a
`std::string` display spelling.

## Why This Exists

754 selected `LirExtractValueOp`, but its real HIR `extractvalue` producers
pass the aggregate SSA value as `std::string` before operation construction.
That loses the originating `LirValueId`; adding only a row-local field cannot
publish a checked aggregate use. The shared operand/expression boundary must
retain opt-in native provenance first.

## In Scope

- Trace the minimal surrounding expression APIs used by real HIR
  `extractvalue` producers and identify the narrowest opt-in provenance carrier
  that can preserve an existing SSA `LirValueId` alongside compatibility
  rendering.
- Propagate and validate that carrier through the necessary LIR operand and
  construction seams, rejecting missing, unknown, foreign, or incoherent SSA
  provenance.
- Add nearby same-feature positive and malformed proof for provenance
  preservation and rejection, then publish an exact handoff contract for 754.

## Out Of Scope

- `LirExtractValueOp` result/schema publication, aggregate index/type
  validation, other aggregate/vector rows, Raw-BIR receipt, target lowering,
  MIR, or emission.
- General expression API redesign, broad operand-family conversion, or any
  identity reconstruction from strings, printer output, LLVM text, instruction
  order, or testcase names.

## Acceptance Criteria

- The real HIR `extractvalue` SSA aggregate path retains a checked native
  `LirValueId` through the required expression/operand construction boundary.
- Missing, foreign, stale, or type-incoherent provenance rejects without
  consulting compatibility display text.
- Focused positive and malformed same-feature proof supports a precise 754
  producer handoff; unrelated operand/expression paths remain unchanged or
  fail closed.

## Completion Disposition

Capability complete and accepted for the bounded prerequisite only. Step 1
trace is `4c6865e62`; Step 2 implementation is `f9fa478fb`; Step 3
proof/handoff is `2c231e342`.

Published contract for 754: a selected direct composite call creates
`LirOperand::ssa(display, LirValueId)`; unary real/imag forwards that exact
operand into `LirExtractValueOp.agg`. Verification requires a valid
current-function call-result ID, structured return/aggregate type equality,
and display-mirror equality. Missing, unknown/foreign, stale-display, and
type-incoherent forms reject. Compatibility text neither recovers nor repairs
authority. This idea did not publish extractvalue result/index schema work.

Accepted proof: a fresh build plus `ctest --test-dir build -j
--output-on-failure -R '^(backend_|frontend_hir_tests$)'` passed 6/6 before
and after; the monotonic guard accepted the equal 6/6 result. The satisfied
parent return point is Step 2 of
`ideas/open/754_lir_aggregate_vector_value_identity_convergence.md`.

## Reviewer Reject Signals

- Reject adding a `LirExtractValueOp` result/use field, aggregate index rules,
  or Raw-BIR receiver work here; those belong to 754 or its future consumer.
- Reject a catch-all expression/operand rewrite, text parsing, testcase-shaped
  shortcut, expectation downgrade, or weaker verifier/test contract.
- Reject carrying only a new abstraction name while the actual HIR
  `extractvalue` SSA path still discards `LirValueId` into `std::string`.
