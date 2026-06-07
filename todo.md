# Current Packet

Status: Active
Source Idea Path: ideas/open/121_aarch64_calls_preservation_republication_visibility_contract.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Prior Stack-Preserved Argument Visibility

## Just Finished

Step 2: Add Preserve-Effect Publication Visibility is complete.

Changed files:

- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`
- `todo.md`

Added focused disabled-publication visibility in
`preserve_effect_publication_scope_suppresses_call_node_payload_only`:

- Dispatches the existing prepared direct-call fixture through real AArch64
  lowering under `ScopedPreparedCallPreserveEffectPublication(false)`.
- Verifies the prepared `source_call` still owns the callee-saved and
  stack-slot `preserved_values`.
- Verifies the emitted call-node `CallInstructionRecord::preserved_values` and
  `InstructionRecord::preserves` are absent only in disabled mode.
- Verifies ordinary call dispatch facts, clobbers, and call side effects remain
  intact.

Enabled publication remains covered by the neighboring
`block_dispatch_lowers_prepared_direct_call_without_reclassifying_abi` route
and by `backend_aarch64_target_instruction_records` preserved-value payload and
effect checks.

## Suggested Next

Execute Step 3: add focused prior stack-preserved argument visibility that
distinguishes prepared prior-preservation selection from local stack-home
reselection.

## Watchouts

- Do not extract a preservation owner before the visibility contract exists.
- Do not reopen stack-preserved source, endpoint stack storage, or frame-slot
  ownership from idea 93.
- Do not move AArch64 callee-saved register spelling, frame-slot store lines,
  preservation-home publication records, or ABI/local effect resources into
  shared code.
- Treat expectation downgrades, unsupported-path rewrites, and helper-renaming
  claims without new route-visible evidence as route failures.
- Step 2 did not change preservation ownership or lowering semantics; it only
  made the disabled preserve-effect publication scope route-visible.
- Step 3 should not reopen stack-source or frame-slot ownership from idea 93.

## Proof

Ran exactly:

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(backend_call_boundary_effect_plan|backend_aarch64_target_instruction_records|backend_aarch64_call_boundary_owner|backend_aarch64_instruction_dispatch)$' >> test_after.log 2>&1`

Result: passed, 4/4 tests.

Proof log: `test_after.log`

Supervisor regression guard:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
passed with 4/4 before and 4/4 after, no new failures. The comparison is at
CTest test-binary granularity; the new disabled-publication fixture runs inside
`backend_aarch64_instruction_dispatch`.
