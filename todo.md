# Current Packet

Status: Active
Source Idea Path: ideas/open/743_lir_i686_long_width_policy_convergence.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish the structured target-aware width policy

## Just Finished

- Plan Step 2 is complete. `c4c::long_width_bits(const TargetProfile&)` in
  `src/target_profile.hpp` is now the shared structured authority: I686 is 32
  bits and every other supported LP64 profile is 64. Shared LLVM production
  (`llvm_base`, therefore `llvm_ty`/`llvm_value_ty`) uses the active structured
  profile; LIR verification passes `LirModule::target_profile` into
  `exact_plain_scalar_mirror`; and new-BIR uses the helper in
  `lower_constant_type` while admitting `TB_LONG`/`TB_ULONG` through the
  existing plain parameter receipt path. Signature, return, and global receipt
  therefore compare the same structured width rule without text or name
  recovery.
- Focused coverage now proves I686 i32 and LP64 i64 long/unsigned-long
  parameter mirrors in frontend LIR production and verification; new-BIR
  accepts I686 and LP64 parameter, return, and global receipt at the matching
  widths, and rejects malformed I686 i64 parameter, return, and global mirrors
  transactionally. Existing `int`, `long long`, float/double, and pointer
  neighbor assertions remain in the same test families.

## Suggested Next

- Execute Step 3 only after supervisor acceptance: run the requested
  cross-target proof/checkpoint and publish the exact bounded idea-734 handoff
  for the `long`/`unsigned long` parameter rows. Do not treat this packet alone
  as an idea-734 unblock or closure decision.

## Watchouts

- Do not use rendered type text, target-triple spelling, names, or testcase
  identity as width authority. Preserve I686 C `long` at 32 bits and LP64 at
  64 bits; do not absorb unrelated idea-734 receiver work.
- Other open candidates remain ineligible for this activation: 732 and
  737-740 are parked pending post-734 order, 734 is blocked on 744, 744 is
  blocked on open idea 746, and 746 has no supported legal source carrier.
- Later proof must build the focused targets with `cmake --build build --target
  frontend_lir_function_signature_type_ref_test backend_lir_to_bir_interface_test`
  and run `ctest --test-dir build --output-on-failure -R
  '^(frontend_lir_function_signature_type_ref|backend_lir_to_bir_interface)$'`.
  Extend those families for I686 and one LP64 positive plus malformed mirror
  rejection across parameter, return, and global receipt; retain their current
  `int`, `long long`, floating, and pointer checks. Relevant backend proof is
  the same `backend_lir_to_bir_interface` CTest because it lowers both Raw and
  Canonical BIR transactionally.

## Proof

- Passed: `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^(frontend_lir_function_signature_type_ref|backend_lir_to_bir_interface)$'
  with CTest output in `test_after.log`; both focused tests passed. `git diff
  --check` also passed. The supervisor still owns the Step 3 regression
  checkpoint and handoff decision.
