Status: Active
Source Idea Path: ideas/open/628_fpr_abi_frame_policy_and_placement.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Publish Or Verify Prepared FPR Facts

# Current Packet

## Just Finished

Step 3 verified prepared-layer FPR facts for the fixed-frame idea 628 family before RV64 lowering. Added focused coverage in `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`; no producer files changed because the existing carriers already publish the required facts.

The new `riscv_fpr_abi_frame_fact_contract` fixture proves:

- FPR immediate/literal call argument placement is explicit before object emission: `source_encoding=Immediate`, `source_literal=0x40800000` with `F32` type, `value_bank=Fpr`, destination `fa0`, destination bank `Fpr`, destination occupied names `fa0`, `PreparedRegisterSlotPool::CallArgument`, width 1, and RV64 target FPR identity physical index 10.
- FPR call-result authority remains explicit on the same focused call: result `value_bank=Fpr`, source storage `Register`, source bank/name `Fpr:fa0`, and `CallResult` placement.
- FPR callee-saved preservation across the preceding call is explicit: `PreparedCallPreservedValue` for `float.carry` uses `route=CalleeSavedRegister`, bank `Fpr`, `fs*` register placement in the callee-saved pool, width 1, occupied names matching the saved register, `callee_saved_save_index`, and register-to-register FPR preservation endpoints.
- Fixed-frame FPR saved-register slot placement is complete: matching `PreparedSavedRegister` bank/name/save index, complete `PreparedSavedRegisterSlotPlacement`, fixed location, 8-byte size/align, slot id, stack offset equal to the fixed frame size, and matching register placement.
- The prepared dump exposes the same authority through `prepared-call-plans` and `prepared-frame-plan` lines before object emission.

Structured prepared facts, not final object output, are the proof surface for this packet.

## Suggested Next

Execute Step 4 as a narrow RV64 consumer packet in `fragment_for_prepared_call(...)`: admit only prepared F32/F64 immediate/literal call arguments into explicit FPR ABI destinations and admit only prepared callee-saved FPR register-preservation effects before/after calls. Add focused positive and fail-closed object-emission coverage for missing/mismatched FPR bank, missing placement or target identity, wrong width, absent/non-F32/F64 source literal, malformed preservation endpoints, and GPR/FPR bank confusion.

## Watchouts

Do not collapse this into scalar GPR call lowering with `fa` spellings. The Step 3 test intentionally proves explicit prepared FPR facts before consumer work. Keep dynamic FPR frame-slot publication, FPR stack results, FPR stack-slot arguments, `unsafe-fp-assoc-1.c` fan-in, floating comparisons/min/max/casts, local/global memory repair, runtime mismatch triage, variadic/library policy, unsupported markers, allowlists, timeouts, and `f128` outside idea 628.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`.

Result: passed, 347/347 backend tests. Proof log: `test_after.log`.
