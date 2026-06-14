Status: Active
Source Idea Path: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Design the Dominance Agreement Boundary

# Current Packet

## Just Finished

Step 2: Design the Dominance Agreement Boundary completed for idea 260's
selected `control_flow` prior preserved-value lookup candidate.

Accepted boundary: keep the agreement local to the prior preserved-value lookup
path in `src/backend/prealloc/prepared_lookups.cpp`, preferably as a small
helper or adapter used by `find_unique_indexed_prior_preserved_value_source(...)`
and any immediately adjacent prior-preserved lookup helper that must share the
same check. Do not add a broad public API, do not move ownership out of
prepared call-plan lookups, and do not treat this as a `PreparedBirModule`
`control_flow` field exit.

Accepted rows:

- The queried `PreparedValueId` must index an existing nonempty
  `PreparedCallPlanLookups::prior_preserved_by_value` row.
- The selected prior entry must have a non-null preserved pointer with complete
  route metadata: `CalleeSavedRegister` requires register name, bank, nonzero
  width, occupied names, and placement; `StackSlot` requires valid value name,
  slot id, stack offset, and nonzero stack size; `Unknown` is unavailable.
- A same-block prior row is eligible only when
  `entry.instruction_index < current_call_plan.instruction_index`.
- A cross-block prior row is eligible only when non-null prepared
  `control_flow` plus the route/BIR-derived prepared CFG relationship proves
  `entry.block_index` dominates or reaches `current_call_plan.block_index`.
- If multiple eligible rows exist at the same selected block/instruction
  position, the lookup must return `Ambiguous` instead of choosing one.

Rejected boundary:

- Null `control_flow` rejects cross-block rows; it may still allow strictly
  earlier same-block rows because no CFG fact is needed.
- Invalid or out-of-range value ids, empty prior rows, missing preserved
  pointers, and incomplete preserved metadata fail closed as current `NotFound`
  or `InvalidPreservation` behavior dictates.
- Later same-block entries, non-dominating blocks, unreachable blocks,
  out-of-range block indices, route/BIR disagreement, and ambiguous agreement
  rows must not be accepted because prepared block indices happen to sort
  earlier.
- The call-plan consumer remains compatibility authority: only a `Found` result
  may produce `PreparedCallArgumentSourceSelectionKind::PriorPreservation`;
  unavailable, ambiguous, or invalid lookup statuses continue to fall through.

Rejected implementation locations: do not put this boundary in
`call_plans.cpp` source-selection policy, `control_flow.hpp` broad helpers, or
new aggregate wrappers unless Step 3 proves the lookup path cannot express the
agreement locally. The selected candidate is one helper-row bridge, not a
route-debug, target-output, branch-target, block-label, move-bundle, module,
names, store-source, or backend lowering migration.

## Suggested Next

Execute Step 3: implement the narrow dominance bridge for
`find_unique_indexed_prior_preserved_value_source(...)`.

Explicit implementation packet ownership:

- Owned files:
  `src/backend/prealloc/prepared_lookups.cpp`,
  `src/backend/prealloc/calls.hpp`, and
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`.
- Optional read/revalidation file: `src/backend/prealloc/call_plans.cpp`, only
  to confirm the existing `Found`-only consumer behavior.
- Keep the helper local to the selected prior preserved-value lookup path unless
  a declaration in `calls.hpp` is needed for tests or adjacent lookup sharing.
- Preserve current compatibility and fail-closed behavior for null context,
  invalid ids, empty rows, later same-block rows,
  non-dominating/unreachable/out-of-range rows, duplicate selected positions,
  missing preserved pointers, incomplete preservation metadata, and non-`Found`
  source-selection fallthrough.
- Focused next proof command:
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
- Current dominance is computed from prepared control-flow successor labels in
  `prepared_lookups.cpp`; Step 3 should make the route/BIR agreement boundary
  explicit at the selected lookup helper instead of trusting sorted prepared
  block indices alone.
- Preserve same-block behavior for null `control_flow`; the fail-closed
  requirement applies to cross-block dominance facts that need CFG agreement.
- Existing focused rows live in
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`:
  `verify_linear_function_lookup()` covers same-block prior, cross-block
  dominating prior, `NotFound`, `Ambiguous`, and `InvalidPreservation`;
  `verify_diamond_function_lookup()` is the nearby non-linear fixture for
  non-dominating or unreachable proof rows.

## Proof

Delegated Step 2 proof passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$'`
with output preserved in `test_after.log`.

Result: build reported `ninja: no work to do`; CTest ran
`backend_prepared_lookup_helper` and passed 1/1 matching test.
