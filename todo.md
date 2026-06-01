Status: Active
Source Idea Path: ideas/open/72_aarch64_special_carrier_prepared_policy_cleanup.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Consume Prepared F128 Runtime Helper And Result Policy

# Current Packet

## Just Finished

Completed `plan.md` Step 5 for AArch64 f128 helper-boundary lowering in
`src/backend/mir/aarch64/codegen/f128.cpp`.

Helper-boundary record construction, machine-node selection, and printing now
consume prepared f128 runtime-helper resource, ABI/result ownership, clobber,
selected-call ownership, live-preservation, marshal/unmarshal, scalar result,
and comparison-consumption facts as authority. The AArch64-local path still
owns helper-boundary record construction, q/vector/scalar register rendering,
comparison `cmp`/`cset` spelling, final `bl`, and machine-record emission.

## Suggested Next

Next coherent packet: supervisor should decide whether Step 5 completion is
ready for reviewer/plan-owner lifecycle handling or needs broader validation
around AArch64 special-carrier transport/helper boundaries.

## Watchouts

`review/idea69_step2_prepared_effect_review.md` and
`review/idea70_steps1_3_review.md` were already untracked and were not touched.
The f128 helper printer still keeps q/vector/scalar register conversion,
compare `cmp`/`cset`, terminal `bl`, and machine-record construction
AArch64-local. The new source-policy checks intentionally validate copied
record facts against the prepared helper pointer before printing or selecting.

## Proof

Delegated proof passed:
`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_aarch64_target_instruction_records$' >> test_after.log 2>&1`.
Proof log: `test_after.log`.
