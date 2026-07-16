Status: Active
Source Idea Path: ideas/open/853_lir_next_body_parameter_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish DirectPointer Truthiness Parameter Authority

# Current Packet

## Completed Trace

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

## Just Finished

Completed Step 2. LIR now publishes a native DirectPointer truthiness
parameter authority tuple for the selected `StmtEmitter::to_bool_operand`
pointer route. The accepted tuple preserves the original current-function
pointer parameter `LirValueId`, owner, parameter index, pointer type,
`DirectPointer` ABI, and explicit `PointerTruthiness` role while the consumer
remains the existing `PtrToInt` plus `icmp ne i64 <ptr-int>, 0` lowering.

The verifier rejects missing, invalid, duplicate, foreign,
owner/index/type/ABI/role, non-`PtrToInt`, nonzero-RHS, non-`ne`, and
consumer-incoherent authority without recovering the original parameter from
rendered pointer text, cast spelling, or comparison text.

## Suggested Next

Return to the 853 source completion gate. If accepted, record the exact
one-row handoff to 734 for a future typed Raw-BIR receiver. Do not edit Raw
BIR/importer code in 853.

## Proof

Passed matching focused regression proof:

`( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$' ) > test_before.log 2>&1`

`( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$' ) > test_after.log 2>&1 && git diff --check`

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: before 1/1, after 1/1, no new failures; `git diff --check` passed.
