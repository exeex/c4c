Status: Active
Source Idea Path: ideas/open/410_lir_to_bir_packed_bitfield_scalar_binop_semantics.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Classify The Fn1A Scalar Binop Residual

# Current Packet

## Just Finished

Lifecycle review closed 409 after packed integer aggregate global initializer
admission was repaired. The remaining `20040709-2.c` residual was split into
this semantic LIR-to-BIR scalar-binop idea.

## Suggested Next

Execute Step 1: inspect the fresh `20040709-2.c` failure and classify the
first unsupported scalar-binop operation in `fn1A` over packed `%struct.A`
bitfield-style storage.

## Watchouts

- Do not reopen packed aggregate global initializer admission from 409.
- Do not patch RV64 object emission for this pre-prepared semantic residual.
- Preserve width, signedness, truncation, shift, mask, OR, and packed-field
  storage semantics.
- Do not use filename-specific handling, expectation rewrites, unsupported
  downgrades, or allowlist filtering as progress.

## Proof

Lifecycle close/switch proof:

- Close gate: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
- Regression guard:
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
- Result: PASS, 326/326 before and 326/326 after, no new failures.
