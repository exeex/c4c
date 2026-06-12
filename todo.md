Status: Active
Source Idea Path: ideas/open/224_phase_e2_control_flow_branch_target_helper_private_pass_context.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Broader Validation And Closeout Readiness

# Current Packet

## Just Finished

Completed Step 4 - Add Focused Agreement And Fallback Proof by extending
`verify_control_flow_branch_target_labels_use_agreeing_structured_ids()` in
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp`.

Covered private boundary cases:

- positive agreement through
  `prepare::detail::BranchTargetIdentityPassContext` and
  `prepare::detail::read_agreeing_bir_branch_target_labels(...)`
- absent BIR context rejection
- raw label spelling drift with agreeing structured IDs still accepted
- invalid structured ID rejection
- conflicting/mismatched structured ID rejection
- non-conditional BIR terminator rejection

Retained public fallback coverage in the same focused test:

- prepared-only target lookup still succeeds
- absent prepared block still fails closed
- agreeing BIR IDs still return the expected targets through the public helper
- raw BIR spelling drift, invalid IDs, mismatched IDs, and non-conditional BIR
  terminators preserve prepared fallback
- invalid prepared source labels still fail closed

No helper-oracle statuses/strings, supported-path statuses, expected strings,
printer/debug output, wrapper output, or unrelated backend code were changed.

## Suggested Next

Proceed to Step 5: close out the private pass-context branch-target helper
slice, including any final lifecycle handoff or review the supervisor requests.

## Watchouts

- Step 5 should remain a closeout/lifecycle handoff for this idea, not aggregate
  API retirement or facade reshaping.
- Keep the compare resolver policy byte-stable; it is intentionally retained
  on the public/prepared surface.
- Keep prepared printer/debug rows, x86 joined-branch wrapper outputs,
  helper-oracle strings/statuses, and expected strings byte-stable.
- The focused Step 4 proof directly covers private agreement plus absent,
  invalid-id, conflict/mismatch, non-conditional, and same-feature public
  fallback behavior.

## Proof

Supervisor-selected proof command ran exactly:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_prepared_lookup_helper|backend_aarch64_branch_control_lowering' > test_after.log 2>&1
```

Result: passed. `test_after.log` records 2/2 passing:

- `backend_aarch64_branch_control_lowering`
- `backend_prepared_lookup_helper`
