Status: Active
Source Idea Path: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove Positive and Fail-Closed Rows

# Current Packet

## Just Finished

Step 3: Prove Positive and Fail-Closed Rows completed focused helper proof for
the `control_flow` branch-target identity packet.

Focused coverage status:

- Added explicit focused rows in
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
  `verify_control_flow_branch_target_labels_use_agreeing_structured_ids()` for
  absent prepared control-flow rows, invalid prepared target labels, and public
  prepared fallback when a BIR conditional target is missing its structured
  label id.
- Existing rows in the same helper cover positive prepared/BIR agreement,
  absent private BIR context, raw-name drift/non-agreement, absent prepared
  blocks, invalid BIR structured ids, structured-id mismatch, private
  invalid/conflict rejection, non-conditional BIR fallback, and invalid
  prepared source labels.
- No implementation file changes were needed.

## Suggested Next

Delegate Step 4 broader backend validation for the branch-target identity
candidate with the supervisor-selected backend proof command, then decide
whether the active runbook is ready for plan-owner retirement review.

## Watchouts

- Do not reactivate completed module/names/control-flow call-preservation
  packets.
- Keep this runbook limited to prepared branch-target identity.
- Step 3 remains test-only; implementation already preserved prepared-label
  fallback and fail-closed behavior for non-conditional, absent, invalid,
  missing structured-id, mismatch, and non-agreement rows.
- The Step 2 boundary was confirmed in `control_flow.hpp`; any future code
  change should require a real helper integration gap.
- Do not rewrite expectations, output strings, diagnostics, helper status
  names, or unrelated control-flow readers to claim progress.
- The active plan names `prepared_lookups.cpp` as the primary target, but the
  branch-target helper surface currently lives inline in `control_flow.hpp`.
  Treat that as an inventory finding, not permission to edit unrelated readers.

## Proof

Proof passed:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$'`

Result: rebuilt `backend_prepared_lookup_helper_test` and
`backend_prepared_lookup_helper` passed 1/1.

Proof log: `test_after.log`
