Status: Active
Source Idea Path: ideas/open/858_lir_next_body_parameter_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select and publish one next body-parameter authority row
你該做code review了
你該做test baseline review了

# Current Packet

## Just Finished

Completed `plan.md` Step 1 by selecting and proving exactly one next
producer-side body-parameter row: `LirBinOp.scalar_rhs_parameter_authority`
for a current-function `DirectScalar` floating parameter used as the RHS of
binary floating add `fadd`, with producer shape equivalent to
`return 2.0 + x;`.

The existing emitter already publishes the native RHS carrier. The verifier now
admits that tuple only for selected floating `fadd` or `fmul` RHS authority,
requires a nonselected scalar LHS, and rejects duplicate selected
floating-`fadd` RHS consumers in the current function.

Focused coverage asserts the positive producer tuple: original parameter
`LirValueId`, current `LirFunction.link_name_id` owner, parameter index,
matching floating `LirTypeRef`, `LirNativeBodyParameterAbi::DirectScalar`, and
explicit `LirScalarBinaryParameterRole::Rhs`. Malformed coverage rejects
omitted/missing, invalid, duplicate definition, foreign owner, wrong index,
wrong type, wrong ABI, wrong role, non-`fadd`, RHS mismatch, type mismatch,
selected-LHS incoherence, and duplicate selected consumer forms. Neighboring
`fmul` RHS malformed coverage now uses nonselected `fsub` as the rejected
opcode because `fadd` RHS is the newly selected row.

## Suggested Next

Record and close the producer handoff for 858, then return to 734 for a future
bounded Raw-BIR receiver packet that receives only the selected binary-`fadd`
RHS DirectScalar parameter-use row into typed Raw BIR. Do not start Raw-BIR
receiver implementation from this producer packet.

## Watchouts

Do not edit Raw-BIR/importer receiver code in this successor. Do not repeat or
reopen accepted 734 body-parameter receiver rows through Step 7.46. Floating
binary-`fsub`/`fdiv` rows, other floating binary parameter uses, presentation
fields, generic parameter sweeps, ABI-expanded or aggregate parameter
families, memory/VA, aggregate/vector, module/type/global/metadata, residual
instruction/terminator, inline-assembly, and any other family remain
nonselected and fail closed here.

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
