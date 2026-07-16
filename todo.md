Status: Active
Source Idea Path: ideas/open/847_lir_universal_model_string_escape_hatch_deletion.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Remove expired adapters and prove compile-time separation

# Current Packet

## Just Finished

Completed the Step 4 memory projection residual classification packet.

Shared memory/addressing and local GEP projection helpers now document rendered
subobject type text as an explicit no-id fallback after the root address or
local aggregate has already been accepted. Metadata-bearing root callers must
use structured type-ref lookup and fail closed before those recursive
projection helpers are used.

## Suggested Next

Suggested Next: continue Step 4 residual classification with one non-call-ABI
family, likely memory/provenance, memory intrinsics/local slots, or raw
`TypeDeclMap` lookup. Do not mix families in one packet.

## Watchouts

- Keep this packet narrow. Memory/provenance, intrinsics, local slots, and raw
  `TypeDeclMap`
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
