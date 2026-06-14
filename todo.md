Status: Active
Source Idea Path: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Narrow Dominance Bridge

# Current Packet

## Just Finished

Step 3: Implement Narrow Dominance Bridge completed for idea 260's selected
`control_flow` prior preserved-value lookup candidate.

Implemented a private local agreement bridge in
`src/backend/prealloc/prepared_lookups.cpp` for
`find_unique_indexed_prior_preserved_value_source(...)`. The lookup now builds
prepared dominance and reachability facts when `control_flow` is available,
keeps strictly earlier same-block rows compatible without CFG context, rejects
cross-block rows without explicit prepared CFG agreement, and scans candidate
rows through that agreement boundary instead of using prepared block/instruction
ordering as the eligibility gate.

Focused tests in `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
now cover same-block null-context compatibility, cross-block null-context
rejection, later same-block rejection, invalid and empty rows, duplicate
selected-position ambiguity, missing preserved pointers, incomplete preserved
metadata, out-of-range CFG rows, a dominating reachable diamond source, and a
reachable but non-dominating diamond source. No public `calls.hpp` declaration
was needed.

## Suggested Next

Execute Step 4: add or confirm the remaining fail-closed proof rows for the
selected lookup candidate, especially consumer fallthrough for non-`Found`
prior-preservation lookup statuses if the supervisor wants that asserted
outside the direct helper tests.

Suggested ownership: keep the next packet limited to
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp`, optional
`src/backend/prealloc/call_plans.cpp` read-only inspection for source-selection
fallthrough, and `todo.md`. Reuse the focused proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$'`.

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
- The Step 3 helper requires both prepared dominance and reachability for
  cross-block rows. This intentionally rejects branch-local rows that merely
  reach a join but do not dominate it.
- `clang-format` is not installed in the current container; no formatting tool
  was run.
- The direct helper tests now cover most Step 4 fail-closed rows. The main
  residual proof question is whether to add an explicit call-plan
  source-selection fallthrough assertion for non-`Found` statuses.

## Proof

Delegated Step 3 proof passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$'`
with output preserved in `test_after.log`.

Result: build completed, CTest ran `backend_prepared_lookup_helper`, and the
focused subset passed 1/1.
