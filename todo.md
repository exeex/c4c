# Current Packet

Status: Active
Source Idea Path: ideas/open/120_aarch64_calls_after_call_result_value_local_owner.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Acceptance Review Package

## Just Finished

Step 4 proved neighboring after-call result routes for the in-place
`AfterCallMoveLocalOwner`.

Changed files:

- `todo.md`
- `tests/backend/mir/backend_aarch64_call_boundary_owner_test.cpp`

The focused proof now covers the Step 4 route set:

- scalar integer result movement: existing
  `scalar_call_result_publishes_gpr_to_prepared_stack_home` owner fixture and
  `block_dispatch_lowers_prepared_register_result_move_after_direct_call`
  dispatch fixture cover ABI GPR result publication.
- scalar FP result movement: existing
  `hfa_lane0_call_result_publishes_fpr_to_prepared_stack_home_without_move_bundle`
  owner fixture and `block_dispatch_lowers_prepared_hfa_result_lanes_from_distinct_fprs`
  dispatch fixture cover ABI FPR after-call result publication.
- f128/vector-carrier result movement: added
  `f128_call_result_publishes_q_register_to_prepared_register_home` in
  `tests/backend/mir/backend_aarch64_call_boundary_owner_test.cpp`, covering
  ABI `q0` to prepared register home `q3` through prepared destination f128
  carrier authority. Existing
  `f128_hfa_lane0_call_result_publishes_q_register_to_prepared_stack_home`
  continues to cover q-register publication to a frame slot.
- stack/frame-slot result storage: existing owner fixtures cover GPR, scalar
  FPR, and f128 q-register stores into prepared frame-slot homes, and the
  dispatch subset covers HFA lane stores from ABI publication.

No implementation files, shared prepared/BIR/prealloc files, expectations, or
build metadata were changed.

## Suggested Next

Execute Step 5 acceptance review package. The known f128/vector-carrier
register-to-register after-call result coverage gap was closed by the new
focused owner fixture, so idea 120 appears ready for closure review if the
supervisor accepts the focused proof.

## Watchouts

- Do not reopen destination-home, stack frame-slot, or f128 carrier authority
  from closed ideas.
- Do not move target-specific ABI register spelling, register-view policy,
  q/f128 rendering, or memory operand spelling into shared prepared code.
- Treat expectation downgrades, unsupported-path rewrites, and named-case-only
  proof as route failures.
- Keep the synthetic register-to-stack result publication path in
  `lower_after_call_moves` intact; it is a real after-call result route, not
  dead compatibility code.
- The f128/vector-carrier register-to-register after-call result gap is now
  covered by a focused owner-level fixture, not by a named-case implementation
  shortcut.
- Revisit file splitting only if a later helper extraction creates a smaller
  target-local interface that does not require exporting the private calls
  diagnostic, operand, f128, memory, and call-boundary record helpers together.

## Proof

Ran exactly:

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_call_boundary_owner|backend_prepare_frame_stack_call_contract|backend_prealloc_call_boundary_classification|backend_aarch64_target_instruction_records)$' >> test_after.log 2>&1`

Result: passed, 5/5 tests.

Proof log: `test_after.log`.

Supervisor regression guard:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
passed with 5/5 before and 5/5 after, no new failures. The comparison is at
CTest test-binary granularity; the new focused fixture runs inside
`backend_aarch64_call_boundary_owner`.
