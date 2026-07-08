Status: Active
Source Idea Path: ideas/open/586_uniform_target_register_identity_policy.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Current Target Register Identity Policy

# Current Packet

## Just Finished

Step 1 from `plan.md` audited the current target register identity policy
without implementation changes.

Shared helper surfaces found:
- `src/backend/prealloc/target_register_profile.hpp` declares the ABI register
  policy surface:
  `call_arg_destination_register_name(...)`,
  `call_arg_destination_register_placement(...)`,
  `call_result_destination_register_name(...)`,
  `call_result_destination_register_placement(...)`, and
  `target_register_identity_for_abi_register_placement(...)`.
- `src/backend/prealloc/target_register_profile.cpp` owns the target ABI
  register pools for x86-64 GPR/SSE (`rdi`..`r9`, `xmm0`..`xmm7`), AArch64
  GPR/FP-SIMD (`x0`..`x7`, `s/d/q0`..`s/d/q7`), RV64 GPR/FPR (`a0`..`a7`,
  `fa0`..`fa7`), caller/callee-saved pools, ABI name publication, and ABI
  placement publication.
- `PreparedRegisterPlacement` is the target-profile-relative allocation slot
  (`bank`, `pool`, `slot_index`, `contiguous_width`) in
  `src/backend/prealloc/frame.hpp`.
- `PreparedTargetRegisterIdentity` is the concrete physical identity
  (`target_arch`, `bank`, `register_class`, `physical_index`) in
  `src/backend/prealloc/regalloc.hpp`, carried on `PreparedValueHome` by
  `src/backend/prealloc/value_locations.hpp`.
- Current shared identity publication is RV64-only:
  `target_register_identity_for_abi_register_placement(...)` accepts only
  `CallArgument`/`CallResult` placements with nonzero width, only RV64
  `Gpr`/`Fpr`, only slots `< 8`, and maps slot `0..7` to physical index
  `10..17` (`a0..a7` / `fa0..fa7`).

Target-local consumers and rediscovery points found:
- `src/backend/prealloc/regalloc/value_homes.cpp` publishes formal parameter
  homes from ABI register names and then uses the shared identity helper for
  ABI placements; today that yields identity only for RV64.
- `src/backend/prealloc/regalloc/call_moves.cpp`,
  `src/backend/prealloc/regalloc.cpp`, and
  `src/backend/prealloc/call_plans.cpp` already compute ABI argument/result
  register placements for move bundles and call plans, but they mostly carry
  names/placements rather than target identity.
- RV64 object emission consumes the existing identity path directly in
  `src/backend/mir/riscv/codegen/object_emission.cpp`, especially FPR ABI
  move helpers such as `fpr_register_number_for_abi_placement(...)` and
  home identity checks.
- AArch64 currently rediscovers physical registers from placements and names
  through `src/backend/mir/aarch64/abi/abi.cpp`
  (`call_abi_register(...)`, `placement_register(...)`,
  `convert_prepared_register(...)`) and through call/return consumers in
  `src/backend/mir/aarch64/codegen/calls.cpp`,
  `alu.cpp`, `memory.cpp`, and `prologue.cpp`.
- x86 currently relies on prepared register names and module-rendering helpers:
  `src/backend/mir/x86/codegen/x86_codegen.hpp`
  `select_prepared_call_argument_abi_register_if_supported(...)` reads
  `destination_register_name` from prepared move bundles, while
  `src/backend/mir/x86/module/module.cpp` repeatedly falls back to
  `call_result_destination_register_name(...)` for return emission.
- Runtime helper binding builders in
  `src/backend/prealloc/i128_runtime_helpers.cpp` and
  `src/backend/prealloc/f128_runtime_helpers.cpp` already manufacture ABI
  names plus placements for helper argument/result bindings and are candidate
  consumers once the shared identity surface covers non-RV64 targets.

Candidate extension points:
- Extend only `target_register_identity_for_abi_register_placement(...)` in
  `target_register_profile.*` first, preserving the existing public helper
  surface unless implementation requires a narrow overload.
- AArch64 stable ABI identities can map `CallArgument`/`CallResult` `Gpr` and
  `AggregateAddress` placements to physical `xN` indexes and `Fpr`/`Vreg`
  placements to FP/SIMD physical indexes for slots `0..7`; `register_class`
  should remain the class implied by the placement bank.
- x86-64 stable ABI identities can map `CallArgument` `Gpr` slots `0..5` to
  SysV physical GPR indexes for `rdi`, `rsi`, `rdx`, `rcx`, `r8`, `r9`,
  `CallArgument` `Fpr` slots `0..7` to `xmm0`..`xmm7`, and `CallResult` GPR/FPR
  slot `0` to `rax`/`xmm0`.
- `I686`, stack-passed arguments/results, memory returns, AArch64 `x8` sret
  pointer special placement, unsupported aggregate-address publication where
  no stable register slot is represented, `None` banks, non-ABI pools,
  zero-width placements, out-of-range slots, and multi-register/contiguous
  aggregate lanes should remain identity-less or fail closed until explicitly
  proven.

## Suggested Next

Execute Step 2 from `plan.md`: extend
`target_register_identity_for_abi_register_placement(...)` in
`src/backend/prealloc/target_register_profile.*` for stable AArch64 and x86-64
ABI argument/result placements while preserving the existing RV64 mapping and
the fail-closed shapes listed above.

## Watchouts

- Do not change semantic ABI classification, value freshness authority,
  preservation fallback, move-bundle authority, broad backend lowering, test
  expectations, unsupported markers, allowlists, or runtime-comparison
  behavior under this idea.
- AArch64 `call_arg_destination_register_placement(...)` currently returns no
  placement for the `x8` sret pointer special case even though
  `call_arg_destination_register_name(...)` can name it; do not fabricate
  identity for that shape without first adding an explicit placement policy.
- x86-64 physical-index numbering must be chosen as a target identity contract,
  not inferred from ABI argument order where that differs from architectural
  register numbers.
- `AggregateAddress` is a placement/allocation bank, not always a distinct
  physical register class; map only the stable ABI-register forms and leave the
  rest identity-less.

## Proof

Audit-only packet per supervisor. No build, ctest, or `test_after.log` was
required or created.
