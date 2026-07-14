# Current Packet

Status: Active
Source Idea Path: ideas/open/743_lir_i686_long_width_policy_convergence.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove cross-target behavior and hand off

## Just Finished

- Plan Step 3 proof and handoff recording are complete. The accepted focused
  checkpoint proves I686 `long`/`unsigned long` as i32 and LP64 as i64 across
  LIR production and verification plus new-BIR parameter, return, and global
  receipt; malformed I686 i64 parameter, return, and global mirrors reject
  transactionally. Neighboring `int`, `long long`, floating, and pointer
  behavior remains covered.
- Bounded handoff: only the idea-734 `long`/`unsigned long` parameter rows may
  be re-evaluated in a later, separate lifecycle decision. This record does
  not activate, unblock, complete, supersede, retire, or close ideas 734, 744,
  or 746.

## Suggested Next

- Hand the accepted evidence to the supervisor for any separate lifecycle
  decision concerning the bounded idea-734 `long`/`unsigned long` parameter
  rows. No implementation packet is selected by this completed runbook step.

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

- Supervisor accepted the fresh `cmake --build --preset default`, focused
  `ctest --test-dir build -j --output-on-failure -R
  '^(frontend_lir_function_signature_type_ref|backend_lir_to_bir_interface)$'
  recorded in `test_after.log`, and the matching `test_before.log` versus
  `test_after.log` regression comparison with `--allow-non-decreasing-passed`.
  `git diff --check` remains required for this recording update.
