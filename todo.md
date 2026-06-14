Status: Active
Source Idea Path: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Call-Preservation Dominance Contract

# Current Packet

## Just Finished

Step 1: Inventory Call-Preservation Dominance Contract completed for idea
260's selected `control_flow` call-preservation dominance candidate.

Selected lookup row: `PreparedCallPlanLookups::prior_preserved_by_value`
indexed by `PreparedValueId`, populated with `PreparedPriorPreservedValueEntry`
rows for non-`Unknown` `PreparedCallPreservedValue` entries. The public lookup
under inventory is `find_unique_indexed_prior_preserved_value_source(...)` in
`src/backend/prealloc/prepared_lookups.cpp`.

Current behavior to preserve:

- Null `control_flow` context still allows only strictly earlier same-block
  rows; cross-block rows fail closed because dominance cannot be checked.
- Invalid or out-of-range value ids and empty prior lists return `NotFound`
  with null `entry` and `preserved`.
- Later same-block entries are rejected by strict
  `entry.instruction_index < current_call_plan.instruction_index`.
- Cross-block entries are accepted only when
  `prepared_block_dominates(control_flow, entry.block_index,
  current_call_plan.block_index)` is true.
- Non-dominating or unreachable blocks are skipped by the reachability check;
  if no other row reaches, the result is `NotFound`.
- Duplicate prior entries at the selected same block/instruction position
  return `Ambiguous`.
- Missing `selected->preserved` returns `NotFound`; an incomplete selected
  `PreparedCallPreservedValue` returns `InvalidPreservation`.
- Complete source requirements are route-specific:
  `CalleeSavedRegister` needs register name, bank, nonzero width, occupied
  names, and placement; `StackSlot` needs valid value name, slot id, stack
  offset, and nonzero stack size; `Unknown` is never indexed for prior lookup.
- The call-plan construction consumer in
  `src/backend/prealloc/call_plans.cpp` accepts only `Found` prior lookup
  results for `PreparedCallArgumentSourceSelectionKind::PriorPreservation`.

Candidate implementation files for the next packet:
`src/backend/prealloc/prepared_lookups.cpp`,
`src/backend/prealloc/calls.hpp`, and
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp`. Inspect
`src/backend/prealloc/call_plans.cpp` if source-selection wiring must be
revalidated, but keep the implementation centered on the prior lookup path.

## Suggested Next

Execute Step 2: define the dominance agreement boundary for
`find_unique_indexed_prior_preserved_value_source(...)`, keeping current
fail-closed behavior for null context, invalid ids, empty rows, later
same-block rows, non-dominating/unreachable rows, duplicate positions, and
missing or incomplete preserved rows. Focused next proof command:
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
- Current dominance is computed from prepared control-flow block ordering and
  terminator labels inside `prepared_lookups.cpp`; the selected idea asks for a
  route/BIR dominance agreement boundary, not trust in sorted prepared block
  indices alone.
- Existing focused rows live in
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`:
  `verify_linear_function_lookup()` covers same-block prior, cross-block
  dominating prior, `NotFound`, `Ambiguous`, and `InvalidPreservation`;
  `verify_diamond_function_lookup()` is the nearby non-linear fixture for
  non-dominating or unreachable proof rows.

## Proof

Delegated proof passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
with output preserved in `test_after.log`.

Result: build reported `ninja: no work to do`; CTest ran 180 matching backend
tests and all passed.
