Status: Active
Source Idea Path: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Fail-Closed Proof Rows

# Current Packet

## Just Finished

Step 4: Add Fail-Closed Proof Rows completed for idea 260's selected
`control_flow` prior preserved-value lookup candidate.

Added focused helper-test assertions in
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp` documenting the
source-selection consumer gate for prior-preservation lookup results. The test
now explicitly proves only complete `Found` lookup results are consumer-eligible,
while `NotFound`, `Ambiguous`, `InvalidPreservation`, missing-source,
out-of-range, non-dominating, and unreachable lookup results all fall through
without being treated as a prior-preservation source.

No implementation files were changed, and the Step 3 fail-closed lookup rows
remain intact.

## Suggested Next

Ask the supervisor to decide whether Step 4 exhausts the current runbook slice
or whether idea 260 needs a plan-owner/reviewer pass before close, replacement,
or another selected candidate packet.

## Watchouts

- Keep this runbook limited to the selected `control_flow`
  call-preservation dominance candidate.
- Do not treat this activation as approval for `PreparedBirModule` deletion,
  privatization, wrapping, broad aggregate retirement, or another idea 260
  candidate.
- Preserve prepared call plans, prepared preserved-value rows, existing lookup
  vectors, public aggregate compatibility, and current null or unavailable
  behavior.
- Do not rewrite route-debug, target output, baselines, unsupported
  expectations, helper/oracle status names, printer/debug strings,
  branch-target identity, block-index label bridge, value-name lookup,
  value-home lookup, semantic resolver API, module, store-source publication,
  or backend lowering behavior to claim progress.
- The Step 4 assertion is intentionally a helper-test consumer boundary, not a
  new implementation route. The private `call_plans.cpp` selector remains
  unchanged and still gates prior-preservation selection on `Found` plus
  non-null lookup pointers.
- The Step 3 helper still requires both prepared dominance and reachability for
  cross-block rows. This intentionally rejects branch-local rows that merely
  reach a join but do not dominate it.

## Proof

Delegated Step 4 proof passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$'`
with output preserved in `test_after.log`.

Result: build completed, CTest ran `backend_prepared_lookup_helper`, and the
focused subset passed 1/1.
