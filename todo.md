Status: Active
Source Idea Path: ideas/open/853_lir_next_body_parameter_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish DirectPointer Truthiness Parameter Authority

# Current Packet

## Just Finished

Completed Step 1 trace. Existing accepted body-parameter authority rows cover:
direct pointer GEP base; DirectScalar binary LHS/RHS; DirectScalar return
value; DirectScalar switch selector; DirectScalar truthiness-comparison LHS;
and DirectScalar fixed direct-call arguments 0 and 1.

The first non-duplicate remaining candidate is a DirectPointer current-function
parameter used directly for truthiness. `StmtEmitter::to_bool_operand` handles
pointer values by emitting `PtrToInt` followed by `icmp ne i64 <as_int>, 0`.
The current native definition exists through
`LirCurrentFunctionBodyParameterDefinition` with
`LirNativeBodyParameterAbi::DirectPointer`, but no consumer authority survives
onto the comparison: `LirTruthinessComparisonLhsParameterAuthority` is
DirectScalar-only and the comparison LHS is the `PtrToInt` result, not the
original parameter value. Therefore this is a producer/schema/verifier packet,
not a 734 receiver row.

## Active Packet

Publish only the selected DirectPointer truthiness parameter authority for the
pointer `to_bool_operand` route, preserving the existing `PtrToInt` lowering
shape as a checked consumer relation.

## Work Items

- Add a bounded native LIR carrier for a DirectPointer parameter used as
  truthiness-comparison input. It must preserve the original parameter
  `LirValueId`, owner, parameter index, pointer type, `DirectPointer` ABI, and
  an explicit pointer-truthiness role.
- Populate that carrier only in `StmtEmitter::to_bool_operand` when the input
  value is a native current-function DirectPointer parameter and the emitted
  consumer is the existing `PtrToInt` plus `icmp ne i64 <ptr-int>, 0` route.
- Add verifier checks that reject missing, invalid, duplicate, foreign,
  owner/index/type/ABI/role, and consumer-incoherent authority. The verifier
  must not infer the original parameter from the rendered pointer name, the
  `PtrToInt` text, or the comparison spelling.
- Add focused same-feature positive and malformed-authority coverage near the
  existing frontend body-parameter authority tests.

## Proof

Run a fresh build plus focused producer proof:

`( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$' ) > test_after.log 2>&1 && git diff --check`
