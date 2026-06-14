Status: Active
Source Idea Path: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add the Agreement Boundary

# Current Packet

## Just Finished

Step 2: Add the Agreement Boundary confirmed the local prepared/BIR agreement
boundary for the `control_flow` branch-target identity packet.

Implementation status:

- No code change was needed.
- `src/backend/prealloc/control_flow.hpp` already isolates structured BIR
  branch-target identity in
  `detail::read_agreeing_bir_branch_target_labels(...)`.
- The private reader returns BIR-derived `PreparedBranchTargetLabels` only when
  the source BIR terminator is `CondBranch`, both structured `true_label_id`
  and `false_label_id` are valid, and both ids exactly match the prepared
  conditional branch row.
- The public BIR-aware overload first requires a valid prepared conditional
  row. Missing/invalid/non-conditional prepared rows fail closed to
  `std::nullopt`; absent, incomplete, invalid, non-conditional, mismatched, or
  otherwise non-agreeing BIR facts fall back to the prepared labels.
- Raw BIR label strings do not override prepared labels; only structured
  `BlockLabelId` agreement can supply the BIR-derived identity.

Focused coverage status:

- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
  `verify_control_flow_branch_target_labels_use_agreeing_structured_ids()`
  already covers the positive agreement path and fail-closed/fallback rows for
  absent BIR context, raw-name drift, absent prepared blocks, invalid
  structured ids, structured-id mismatch, private invalid/conflict rejection,
  non-conditional BIR fallback, and invalid prepared source labels.

## Suggested Next

Delegate Step 3 to an executor: add or confirm any remaining focused proof rows
for the branch-target identity candidate in
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp`, then decide
whether the candidate is ready for broader backend validation.

## Watchouts

- Do not reactivate completed module/names/control-flow call-preservation
  packets.
- Keep this runbook limited to prepared branch-target identity.
- Preserve prepared-label fallback and fail closed for non-conditional,
  absent, invalid, missing structured-id, mismatch, and non-agreement rows.
- The Step 2 boundary was confirmed in `control_flow.hpp`; further work should
  focus on proof sufficiency unless a real helper integration gap appears.
- Do not rewrite expectations, output strings, diagnostics, helper status
  names, or unrelated control-flow readers to claim progress.
- The active plan names `prepared_lookups.cpp` as the primary target, but the
  branch-target helper surface currently lives inline in `control_flow.hpp`.
  Treat that as an inventory finding, not permission to edit unrelated readers.

## Proof

Proof passed:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$'`

Result: build was up to date and `backend_prepared_lookup_helper` passed 1/1.

Proof log: `test_after.log`
