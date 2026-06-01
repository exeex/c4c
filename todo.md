Status: Active
Source Idea Path: ideas/open/72_aarch64_special_carrier_prepared_policy_cleanup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Consume Prepared F128 Carrier And Memory-Backed Facts

# Current Packet

## Just Finished

Completed `plan.md` Step 4 for AArch64 f128 transport lowering in
`src/backend/mir/aarch64/codegen/f128.cpp`.

Transport record construction now routes full-width q-register carriers and
memory-backed carrier homes through local prepared-fact adapters before
target-local record construction. Memory-backed carrier address recovery now
cross-checks the record against the original prepared carrier and annotates the
derived carrier frame-slot memory with the prepared value-home identity before
the AArch64-local printer chooses `ldr`/`str` spelling.

## Suggested Next

Next coherent packet: supervisor should decide whether the active plan is ready
for review/plan-owner lifecycle handling or needs broader validation around
AArch64 special-carrier transport/helper boundaries.

## Watchouts

`review/idea69_step2_prepared_effect_review.md` and
`review/idea70_steps1_3_review.md` were already untracked and were not touched.
The f128 transport printer still keeps q/vector register conversion, memory
address spelling, `ldr`/`str` choice, and machine-record construction
AArch64-local; the new adapters only validate and carry prepared authority into
those local decisions.

## Proof

Delegated proof passed:
`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_aarch64_target_instruction_records$' >> test_after.log 2>&1`.
Proof log: `test_after.log`.
