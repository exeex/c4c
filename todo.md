# Current Packet

Status: Active
Source Idea Path: ideas/open/121_aarch64_calls_preservation_republication_visibility_contract.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Stack-To-Register Republication Visibility

## Just Finished

Step 3: Add Prior Stack-Preserved Argument Visibility is complete.

Changed files:

- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`
- `todo.md`

Added focused prior stack-preserved argument visibility in
`stack_preserved_home_feeds_later_call_argument_after_clobber`:

- Gives the later call argument a conflicting local frame-slot source
  (`PreparedFrameSlotId{91}` at offset `120`) while its explicit
  `PriorPreservation` source selection names the prepared stack-preserved
  source (`PreparedFrameSlotId{44}` at offset `32`).
- Verifies the emitted `CallBoundaryMoveInstructionRecord` consumes a prepared
  frame-slot memory operand from the preserved selection, including prepared
  snapshot offset, size, alignment, and value id.
- Verifies the later call argument reload prints from `[sp, #32]` after the
  clobbering call and does not reload from the conflicting local stack source
  `[sp, #120]`.
- Keeps preservation ownership, stack-source selection, shared prepared facts,
  implementation files, and expectations unchanged.

## Suggested Next

Execute Step 4: add focused stack-to-register republication visibility through
prepared preservation endpoint data.

## Watchouts

- Do not extract a preservation owner before the visibility contract exists.
- Do not reopen stack-preserved source, endpoint stack storage, or frame-slot
  ownership from idea 93.
- Do not move AArch64 callee-saved register spelling, frame-slot store lines,
  preservation-home publication records, or ABI/local effect resources into
  shared code.
- Treat expectation downgrades, unsupported-path rewrites, and helper-renaming
  claims without new route-visible evidence as route failures.
- Step 3 did not change preservation ownership or lowering semantics; it only
  made the prior stack-preserved argument source route-visible.
- Step 4 should prove stack-to-register republication through prepared endpoint
  data without moving callee-saved register spelling, frame-slot store line
  spelling, or machine-record emission out of AArch64 lowering.

## Proof

Ran exactly:

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(backend_call_boundary_effect_plan|backend_aarch64_target_instruction_records|backend_aarch64_call_boundary_owner|backend_aarch64_instruction_dispatch)$' >> test_after.log 2>&1`

Result: passed, 4/4 tests.

Proof log: `test_after.log`

Supervisor regression guard:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
passed with 4/4 before and 4/4 after, no new failures.
