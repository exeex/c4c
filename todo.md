# Current Packet

Status: Active
Source Idea Path: ideas/open/121_aarch64_calls_preservation_republication_visibility_contract.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Acceptance Review Package

## Just Finished

Step 4: Add Stack-To-Register Republication Visibility is complete.

Changed files:

- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`
- `todo.md`

Strengthened focused stack-to-register republication visibility in
`stack_preserved_home_feeds_later_non_call_scalar_after_clobber`:

- Gives the preserved value an explicit prepared stack-slot preservation
  destination endpoint at `PreparedFrameSlotId{77}` / offset `96`, distinct
  from the original preserved source slot `PreparedFrameSlotId{44}` / offset
  `32`.
- Verifies the emitted `CallBoundaryMoveInstructionRecord` for the later
  stack-to-register reload keeps the prepared consumer move reason and consumes
  a prepared frame-slot memory operand from the preservation endpoint,
  including endpoint slot id, prepared snapshot offset, size, alignment, and
  value id.
- Verifies the endpoint-driven reload seeds `x22`, and the printed route uses
  `ldr x22, [sp, #96]` before the scalar consumer rather than reselecting the
  original stack-preserved source slot.
- Keeps preservation ownership, endpoint storage ownership, shared prepared
  facts, implementation files, build metadata, and expectations unchanged.

## Suggested Next

Execute Step 5: acceptance review package for idea 121.

## Watchouts

- Do not extract a preservation owner before the visibility contract exists.
- Do not reopen stack-preserved source, endpoint stack storage, or frame-slot
  ownership from idea 93.
- Do not move AArch64 callee-saved register spelling, frame-slot store lines,
  preservation-home publication records, or ABI/local effect resources into
  shared code.
- Treat expectation downgrades, unsupported-path rewrites, and helper-renaming
  claims without new route-visible evidence as route failures.
- Step 4 did not change preservation ownership or lowering semantics; it only
  made stack-to-register republication through prepared preservation endpoint
  data route-visible.
- The stack-slot route keeps the prepared consumer move reason
  `consumer_stack_to_register`; the endpoint visibility is carried by the
  prepared memory operand facts on the `CallBoundaryMoveInstructionRecord`.
- Route-visible proof now covers preserve-effect publication enabled/disabled,
  prior stack-preserved argument consumption, and stack-to-register
  republication. Idea 121 appears ready for Step 5 acceptance review, subject
  to supervisor/reviewer confirmation and regression policy.

## Proof

Ran exactly:

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(backend_call_boundary_effect_plan|backend_aarch64_target_instruction_records|backend_aarch64_call_boundary_owner|backend_aarch64_instruction_dispatch)$' >> test_after.log 2>&1`

Result: passed, 4/4 tests.

Proof log: `test_after.log`

Supervisor regression guard:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
passed with 4/4 before and 4/4 after, no new failures.
