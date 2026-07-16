Status: Active
Source Idea Path: ideas/open/847_lir_universal_model_string_escape_hatch_deletion.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Remove expired adapters and prove compile-time separation

# Current Packet

## Just Finished

Completed additional Step 4 residual repairs and classification.

Since the previous Step 4 repair decision, aggregate copy layout validation,
copy-to-pointer/global leaf discovery, call aggregate-value alias layout, and
local aggregate store/source layout validation now use structured LIR type-ref
facts when metadata is present. Metadata-bearing paths fail closed on structured
misses; no-id aggregate fallbacks remain explicit. The aggregate.cpp rendered-
text helper and aggregate-parameter comments were classified as deliberate no-id
fallback documentation rather than selected valid-LIR authority.

Step 4 is not complete yet. The residual inventory still includes call ABI
bridges where raw legacy signature text or legacy `function.params` can select
aggregate return/byval layout without `signature_param_type_ref` or equivalent
structured metadata. Those paths are 847-owned because they are expired adapter
or dispatcher fallback surfaces that can re-enter semantic layout selection
after the universal model/string escape-hatch deletion.

## Suggested Next

Suggested Next: execute a bounded Step 4 call ABI residual packet.

Classify and repair only the call ABI no-id bridge family:
- `call_abi.cpp` legacy aggregate return/byval layout when `type_ref == nullptr`
- the legacy `function.params` route that lacks `signature_param_type_ref`
- the raw parsed call-signature byval route that still treats rendered byval
  text as the semantic carrier for aggregate ABI layout

For this packet, either route each metadata-bearing call ABI aggregate return or
byval parameter through structured/native type-ref facts and fail closed on
structured misses, or prove and document that a remaining branch is a deliberate
no-id fallback with no valid-LIR semantic authority. Do not include memory,
globals, local GEP, local slots, provenance, global initializer, or raw
TypeDeclMap residuals in the same packet; they can be classified after the call
ABI authority surface is resolved.

## Watchouts

- Do not advance to Step 5 while call ABI aggregate return/byval layout can
  select valid-LIR layout authority from raw signature text.
- Keep this packet narrow. Memory/provenance, addressing, intrinsics,
  `local_gep`, globals, local slots, global initializers, and raw `TypeDeclMap`
  residuals are not cleared by this decision; they remain later Step 4
  classification/repair candidates unless evidence proves them no-id only or
  separately owned.
- Do not silently expand 847 into unrelated owners. If a residual bridge is not
  an 847 deletion target, preserve the current Step 4 return point and ask
  plan-owner to create or switch to the correct separate initiative.
- Keep legitimate no-id text fallbacks explicit and narrow; do not replace the
  deleted universal model with a renamed compatibility bag.

## Proof

Ran exactly:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: passed. `test_after.log` contains `100% tests passed, 0 tests failed
out of 6`.

Supervisor regression guard:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
reported before 6/6, after 6/6, result PASS.
