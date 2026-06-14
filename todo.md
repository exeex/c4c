Status: Active
Source Idea Path: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Broader Validation and Closure Readiness

# Current Packet

## Just Finished

Step 5: Broader Validation and Closure Readiness completed for idea 260's
selected `control_flow` prior preserved-value lookup candidate.

Closure-readiness validation found the selected candidate complete under idea
260's one-candidate acceptance criteria: the work remained limited to the
prior preserved-value lookup/helper row, retained prepared call plans,
prepared preserved-value records, existing lookup vectors, and public aggregate
compatibility, and proved the nearby fail-closed rows without expectation,
baseline, status-name, printer/debug, route-debug, helper-output, or target
output rewrites.

Focused evidence from the prior packet remains green for
`backend_prepared_lookup_helper`: it covers valid same-block and cross-block
prior-preservation selection plus null context, invalid ids, empty prior
lists, later same-block entries, non-dominating or unreachable rows, duplicate
candidate positions, missing preserved pointers, incomplete preservation
metadata, and non-`Found` consumer fallthrough.

Broader backend validation is also green: the delegated backend subset passed
180/180 tests.

## Suggested Next

Ask the plan owner for closure review of this active runbook. Remaining idea
260 candidates should stay in the source idea for future one-candidate
runbooks rather than being absorbed into this plan.

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
- Step 5 is validation/status only. It did not change implementation files,
  tests, baselines, plan text, or source idea text.

## Proof

Focused proof from Step 4 passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$'`
with output preserved in `test_after.log` at the time of that packet.

Result: build completed, CTest ran `backend_prepared_lookup_helper`, and the
focused subset passed 1/1.

Delegated Step 5 broader proof passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
with output preserved in `test_after.log`.

Result: build completed, CTest ran the backend subset, and all backend tests
passed 180/180.
