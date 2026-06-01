Status: Active
Source Idea Path: ideas/open/72_aarch64_special_carrier_prepared_policy_cleanup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Consume Prepared I128 Runtime Helper And ABI Policy

# Current Packet

## Just Finished

Completed `plan.md` Step 3 for the AArch64 i128 div/rem runtime-helper boundary
in `src/backend/mir/aarch64/codegen/i128_ops.cpp`.

Helper-boundary operand construction now routes through a local prepared
helper carrier/lane adapter that consumes the prepared runtime-helper lane
bindings as authority, cross-checks them against prepared i128 carriers, and
hands target-local record construction converted AArch64 register operands.
Runtime-helper record construction, selection validation, and printable
emission now share checks for prepared resource policy, ABI policy,
live-preservation, selected-call ownership, clobber policy, and the structured
marshal/unmarshal move plan. Final helper-boundary record construction and
`bl` emission remain AArch64-local.

## Suggested Next

Next coherent packet: supervisor should decide whether Step 4 should broaden
validation around AArch64 special-carrier helper boundaries or hand the active
plan to review/plan-owner for the next lifecycle decision.

## Watchouts

`review/idea69_step2_prepared_effect_review.md` and
`review/idea70_steps1_3_review.md` were already untracked and were not touched.
The marshal-plan validator intentionally rejects missing or mismatched
structured prepared ABI move facts instead of reconstructing a fallback call
sequence.

## Proof

Delegated proof passed:
`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_aarch64_target_instruction_records$' >> test_after.log 2>&1`.
Proof log: `test_after.log`.
