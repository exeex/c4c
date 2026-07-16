Status: Active
Source Idea Path: ideas/open/857_lir_next_body_parameter_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Record the handoff back to 734
你該做code review了
你該做test baseline review了

# Current Packet

## Just Finished

Completed `plan.md` Step 2 by proving the selected
`LirBinOp.scalar_lhs_parameter_authority` DirectScalar floating LHS row for a
binary `fadd` consumer.

The existing emitter already publishes the native LHS carrier for a producer
shape equivalent to `return x + 2.0;`. The verifier now admits that tuple only
for selected floating `fadd` LHS authority, requires a nonselected scalar RHS,
and rejects duplicate selected floating-`fadd` LHS consumers in the current
function.

Focused coverage asserts the positive producer tuple: original parameter
`LirValueId`, current `LirFunction.link_name_id` owner, parameter index,
matching floating `LirTypeRef`, `LirNativeBodyParameterAbi::DirectScalar`, and
explicit `LirScalarBinaryParameterRole::Lhs`. Malformed coverage rejects
omitted/missing, invalid, duplicate definition, foreign owner, wrong index,
wrong type, wrong ABI, wrong role, non-`fadd`, LHS mismatch, type mismatch,
selected-RHS incoherence, and duplicate selected consumer forms. Neighboring
`fmul` malformed coverage now uses nonselected `fsub` as the rejected opcode
because `fadd` is the newly selected row.

## Suggested Next

Execute `plan.md` Step 3: record the exact 734 handoff in the source idea and
close or switch lifecycle state as appropriate. The handoff row is
`LirBinOp.scalar_lhs_parameter_authority` for a current-function DirectScalar
floating parameter used as the LHS of binary `fadd`; future 734 work should
receive only this row into typed Raw BIR.

## Watchouts

Exclude accepted rows through 734 Step 7.45: DirectPointer GEP-base,
DirectScalar binary-LHS/RHS integer row, return-value, switch-selector,
truthiness-comparison LHS, fixed-direct-call arguments 0 and 1, DirectPointer
pointer-truthiness, DirectScalar unary-`fneg` LHS, DirectScalar binary-`fmul`
LHS, and DirectScalar binary-`fmul` RHS. The selected row may reuse the
existing LHS carrier schema, but the handoff must be only the distinct
binary-`fadd` LHS consumer row.

Later or nonselected candidates remain fail closed: floating binary-`fadd` RHS,
other floating binary op parameter uses, memory/VA, aggregate/vector,
module/type/global/metadata, residual instruction/terminator, inline-assembly,
ABI-expanded or aggregate parameters, direct-call argument positions beyond
the bounded accepted slots, and generic parameter sweeps have no selected
one-row consumer handoff here. Do not derive authority from text, names,
rendered operands, signatures, diagnostics, compatibility mirrors, `monostate`,
or testcase shape.

## Proof

Focused proof and broader frontend-LIR guard:

```
( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_function_signature_type_ref$' ) > test_after.log 2>&1 && git diff --check
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed

( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_' ) > test_before.log 2>&1
( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_' ) > test_after.log 2>&1 && git diff --check
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

Both matching before/after guards passed with no new or unresolved failures.
