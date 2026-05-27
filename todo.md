# Current Packet

Status: Active
Source Idea Path: ideas/open/57_aarch64_variadic_va_arg_register_save_progression_repair.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Confirm aggregate register-save progression ordering

## Just Finished

Lifecycle switched from parked calls prepared authority work to the variadic
aggregate `va_arg` register-save progression owner.

## Suggested Next

Delegate Step 1 classification/implementation preparation for
`src/backend/mir/aarch64/codegen/variadic.cpp`. Confirm the helper or lowering
block that computes register-save source addresses from post-incremented
`gp_offset`/`fp_offset`, then prepare the Step 2 repair packet.

## Watchouts

The semantic contract is the AAPCS64 va_list register-save rule: the current
aggregate source address uses the old negative offset, while the va_list
progression field receives old offset plus the prepared stride.

Treat existing dirty `memory.cpp`, `dispatch_edge_copies.cpp`, and transient
`review/*` files as external context.

## Proof

Validation not run by plan-owner; lifecycle-only switch.

Latest inherited focused proof: build passed, focused subset `7/8` passed,
and only `c_testsuite_aarch64_backend_src_00204_c` failed with
`[RUNTIME_MISMATCH]`. Proof log path: `test_after.log`.
