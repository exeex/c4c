Status: Active
Source Idea Path: ideas/open/237_aarch64_binary128_softfloat_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reconcile Parent Binary128 Route With Constant Carrier

# Current Packet

## Just Finished

Step 1 `Reconcile Parent Binary128 Route With Constant Carrier` reconciled the
parent F128 helper mapping with the closed full-width constant carrier. F128
soft-float helper operands now resolve named values or semantic full-width
`F128Payload` immediates through prepared regalloc values, so a binary128
helper operand can point at the structured constant carrier instead of being
rejected as a non-named operand. Helper selection remains fail-closed because
the existing helper boundary still requires a full-width register carrier for
marshaling; partial/scalar-only F128 immediates still do not enter helper
mappings.

## Suggested Next

Run the Step 2 representative parent-route proof, including the already
selected semantic constant call-boundary path and the newly guarded helper
constant-carrier fail-closed path.

## Watchouts

- The helper route now sees full-width F128 constants, but it intentionally
  does not rematerialize them into helper ABI Q registers yet; diagnostics stay
  on `rhs_requires_full_width_f128_carrier` /
  `incomplete_prepared_f128_runtime_helper`.
- Partial or scalar-only F128 constants still fail before helper mapping
  through `f128_soft_float_helper_requires_prepared_value_id_for_result_lhs_rhs`.
- No AArch64 constant assembly printing was added.

## Proof

Command:

```sh
set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } 2>&1 | tee test_after.log
```

Result: passed, `139/139` backend tests green. Proof log: `test_after.log`.
