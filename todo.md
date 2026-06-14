Status: Active
Source Idea Path: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Locate the Branch-Target Helper Surface

# Current Packet

## Just Finished

Step 1: Locate the Branch-Target Helper Surface completed the inventory for
the `control_flow` branch-target identity packet.

Selected helper row:

- `src/backend/prealloc/control_flow.hpp`
- `PreparedControlFlowFunction::blocks` conditional-branch row read by
  `find_prepared_control_flow_branch_target_labels(function_cf, block_label)`
  and the `find_prepared_control_flow_branch_target_labels(function_cf,
  block_label, source_block)` overload.
- BIR structured identity is currently isolated in
  `detail::read_agreeing_bir_branch_target_labels(...)` through
  `detail::BranchTargetIdentityPassContext`.

Current contract:

- Prepared-only lookup returns the prepared conditional branch `true_label` and
  `false_label`.
- Absent prepared block, invalid prepared source label, non-conditional
  prepared block, or invalid prepared true/false labels return `std::nullopt`.
- The public BIR-aware overload first requires a valid prepared conditional row.
  If that row is absent, it returns `std::nullopt` rather than using BIR.
- For present prepared rows, BIR conditional structured ids are used only when
  both `true_label_id` and `false_label_id` are valid and exactly match the
  prepared row.
- Absent BIR context, non-conditional BIR terminators, invalid/missing
  structured ids, mismatched ids, and other non-agreement rows fall back to the
  prepared labels.
  Raw BIR label strings do not override prepared labels when structured ids
  agree.

Nearby focused test row:

- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
- `verify_control_flow_branch_target_labels_use_agreeing_structured_ids()`
  already exercises prepared-only lookup, private agreement acceptance,
  absent context rejection, raw-name drift, absent prepared block, public
  agreement, invalid structured id fallback, mismatch fallback,
  private invalid/conflict rejection, non-conditional BIR fallback, and invalid
  prepared source rejection.

## Suggested Next

Delegate Step 2 to an executor: tighten or confirm the local prepared/BIR
agreement boundary for branch-target identity in
`src/backend/prealloc/control_flow.hpp`, with focused coverage in
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp`.

Minimal Step 2 file scope:

- Implementation: `src/backend/prealloc/control_flow.hpp`
- Tests: `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
- `src/backend/prealloc/prepared_lookups.cpp` and
  `src/backend/prealloc/calls.hpp` do not appear necessary for this selected
  helper row unless Step 2 discovers a call-site integration gap.

## Watchouts

- Do not reactivate completed module/names/control-flow call-preservation
  packets.
- Keep this runbook limited to prepared branch-target identity.
- Preserve prepared-label fallback and fail closed for non-conditional,
  absent, invalid, missing structured-id, mismatch, and non-agreement rows.
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
