Status: Active
Source Idea Path: ideas/open/856_lir_next_body_parameter_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Record The 734 Handoff
你該做code review了
你該做test baseline review了

# Current Packet

## Just Finished

Completed `plan.md` Step 2 by proving the selected
`LirBinOp.scalar_rhs_parameter_authority` DirectScalar floating RHS row for a
binary `fmul` consumer. The existing emitter already publishes the native RHS
carrier for `return 2.0 * x;`, so no emitter churn was needed.

Focused coverage now asserts the positive producer tuple for the current-
function `DirectScalar` `double` parameter used as the RHS of `return 2.0 * x;`:
original parameter `LirValueId`, current `LirFunction.link_name_id` owner,
parameter index, matching floating `LirTypeRef`,
`LirNativeBodyParameterAbi::DirectScalar`, and explicit
`LirScalarBinaryParameterRole::Rhs`.

Malformed coverage rejects omitted/missing, invalid, duplicate, foreign,
owner/index/type/ABI/role, non-`fmul`, RHS mismatch, result/type mismatch,
selected-LHS incoherence, and duplicate selected RHS consumer forms. Existing
verifier constraints keep floating RHS authority limited to exactly one
selected `fmul` consumer whose `rhs` is the parameter SSA/value and whose `lhs`
is a nonselected scalar operand.

## Suggested Next

Execute `plan.md` Step 3: record the exact 734 handoff in the source idea and
close or switch lifecycle state as appropriate. The handoff row is
`LirBinOp.scalar_rhs_parameter_authority` for a current-function DirectScalar
floating parameter used as the RHS of binary `fmul`; future 734 work should
receive only this row into typed Raw BIR.

## Watchouts

Exclude accepted rows through 734 Step 7.44: DirectPointer GEP-base,
DirectScalar binary-LHS/RHS integer row, return-value, switch-selector,
truthiness-comparison LHS, fixed-direct-call arguments 0 and 1,
DirectPointer pointer-truthiness, DirectScalar unary-`fneg` LHS, and
DirectScalar binary-`fmul` LHS. The selected binary-`fmul` RHS row now has
focused native verifier coverage; keep later rows separate.

Later or nonselected candidates remain fail closed: memory/VA, aggregate/vector,
module/type/global/metadata, residual instruction/terminator, inline-assembly,
ABI-expanded or aggregate parameters, direct-call argument positions beyond the
bounded accepted slots, and generic parameter sweeps have no selected
one-row consumer handoff here. Do not derive authority from text, names,
rendered operands, signatures, diagnostics, compatibility mirrors, `monostate`,
or testcase shape.

## Proof

Focused proof:

```
( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_function_signature_type_ref$' ) > test_after.log 2>&1 && git diff --check
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

Focused and broader frontend-LIR proofs passed with no new or unresolved
failures. The broader shared-verifier guard used matching before/after runs:

```
( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_' ) > test_before.log 2>&1
( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_' ) > test_after.log 2>&1 && git diff --check
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```
