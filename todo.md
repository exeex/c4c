# Current Packet

Status: Active
Source Idea Path: ideas/open/828_lir_direct_scalar_unary_fneg_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove the narrow route and return to 827

## Just Finished

- Completed plan Step 2: unary floating-minus now publishes `LirScalarBinaryLhsParameterAuthority` only when its direct `fneg` operand structurally matches one current-function native `DirectScalar` definition by value, exact `LirTypeRef`, owner, and ABI, with role `Lhs`. The verifier now rejects populated `fneg` rhs operands while retaining the existing non-`FNeg` empty-rhs rejection. Nearby coverage proves the published tuple plus omitted-authority, mismatched-tuple, and populated-rhs failures. `cmake --build --preset default` passed; `ctest --test-dir build -j --output-on-failure -R '^frontend_lir_function_signature_type_ref$'` passed and is recorded in `test_after.log`.

## Suggested Next

- Execute plan Step 3 only: use the fresh build and focused CTest already recorded in `test_after.log` as this idea's narrow acceptance evidence, then return the result to 827 for its explicit repair/close lifecycle decision; do not credit the parent composite CTest or dirty selector/call-type work.

## Watchouts

- Preserve the dirty 821/822/825-related worktree changes. The Step 2 patch intentionally does not touch binary producers, switch selectors, Raw-BIR/importer, schemas, or non-unary-fneg rows.

## Proof

- Build: `cmake --build --preset default` — passed (existing deprecation warnings only).
- Focused proof: `ctest --test-dir build -j --output-on-failure -R '^frontend_lir_function_signature_type_ref$'` — passed; canonical output: `test_after.log`.
