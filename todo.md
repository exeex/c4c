Status: Active
Source Idea Path: ideas/open/628_fpr_abi_frame_policy_and_placement.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Narrow RV64 FPR Consumer Admission

# Current Packet

## Just Finished

Step 4 added narrow RV64 object-emission admission in `fragment_for_prepared_call(...)` for explicit prepared F32/F64 immediate/literal FPR call arguments and prepared FPR callee-saved register-preservation effects.

The consumer now accepts only FPR immediate call arguments with an immediate F32/F64 source literal matching the BIR argument type, value bank `Fpr`, explicit destination bank `Fpr`, width 1, occupied destination register names matching the named ABI destination, a `PreparedRegisterSlotPool::CallArgument` FPR placement, and a destination target-register identity that maps to the same RV64 FPR as the destination name and placement. It materializes the literal bits through `t0` and moves them into the destination FPR with `fmv.w.x`/`fmv.d.x`.

The consumer now accepts only register-to-register FPR callee-saved preservation effects with available classification, `CalleeSavedRegister` route, FPR register endpoints, matching occupied names, required target identities that agree with endpoint names, and a callee-saved FPR placement on the storage endpoint. It emits `fmv.d` before/after the call for the preservation population/republication path.

Added focused RV64 object-emission coverage for positive F32/F64 immediate FPR call arguments, positive FPR callee-saved preservation around a call, and fail-closed mutations for missing/mismatched FPR placement, missing/mismatched target identity bank/class/index, GPR/FPR destination confusion, wrong width, missing/non-F32/F64 literal, explicit GPR source-bank confusion, type mismatch, malformed preservation endpoint storage/banks/placement/identity, missing preservation endpoint target identity, and occupied-register mismatch. Existing scalar GPR and pointer stack-result call tests remain in the same backend subset.

## Suggested Next

Execute Step 5 by re-running the idea 628 representative-row probes and classifying the remaining failures. Decide whether the source idea is close-ready, needs another narrow FPR ABI/frame packet, or should split residual rows into separate owner initiatives.

## Watchouts

Dynamic FPR frame-slot publication, FPR stack results, FPR stack-slot arguments, `unsafe-fp-assoc-1.c` fan-in, floating comparisons/min/max/casts, local/global memory repair, runtime mismatch triage, variadic/library policy, unsupported markers, allowlists, timeouts, and `f128` remain outside this Step 4 slice. The FPR preservation consumer intentionally emits only register-to-register `fmv.d` for callee-saved FPR endpoints; it does not add FPR stack preservation or dynamic FPR frame-slot support.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`.

Result: passed, 347/347 backend tests. Proof log: `test_after.log`.
