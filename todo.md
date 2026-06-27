Status: Active
Source Idea Path: ideas/open/409_prepared_packed_fp128_global_initializer_admission.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Classify Packed And FP128 Global Admission

# Current Packet

## Just Finished

Lifecycle review closed 399 as stale/passing for current RV64 object-route
global-data diagnostics. The remaining `20040709-2.c` evidence was split into
this producer-side packed aggregate and `fp128` global initializer admission
idea.

## Suggested Next

Execute Step 1: inspect the fresh `20040709-2.c` compile/prepared artifacts
and classify the producer admission rule blocking packed aggregate and
`fp128` global initializer facts before prepared handoff.

## Watchouts

- Do not reconstruct packed aggregate layout or `fp128` initializer bytes in
  RV64 object emission.
- Keep producer/global admission separate from target global-data lowering.
- Route unrelated direct-call, memset, or vector local alloca failures to their
  own owners.
- Do not use filename-specific handling, expectation rewrites, unsupported
  downgrades, or allowlist filtering as progress.

## Proof

Lifecycle close/switch proof:

- Close gate: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
- Regression guard:
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
- Result: PASS, 326/326 before and 326/326 after, no new failures.
