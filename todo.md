Status: Active
Source Idea Path: ideas/open/142_value_home_move_bundle_lookup_ownership.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Value-Home And Move-Bundle Surface

# Current Packet

## Just Finished

Step 1 inspection mapped the value-home and move-bundle lookup surface before
code movement.

Declaration/definition map:

- `PreparedValueHomeLookups`: declared in
  `src/backend/prealloc/prepared_lookups.hpp` with `homes_by_id` and
  `value_ids`; built by `make_prepared_value_home_lookups` declared in the same
  header and defined in `src/backend/prealloc/prepared_lookups.cpp`.
- Indexed value-home/id helpers:
  `find_indexed_prepared_value_home`, `find_indexed_prepared_value_id`, and
  `find_prepared_value_home_for_bir_value` are inline templates/helpers in
  `src/backend/prealloc/value_locations.hpp`, not in `prepared_lookups.hpp`.
  They accept any lookup object with `homes_by_id`/`value_ids` and fall back to
  `PreparedValueLocationFunction`/`PreparedRegallocFunction` scans.
- `PreparedMoveBundleLookups`: declared in
  `src/backend/prealloc/prepared_lookups.hpp` with bundle, before-call,
  before-return, and after-call result-lane indexes; built by
  `make_prepared_move_bundle_lookups` declared in the same header and defined
  in `src/backend/prealloc/prepared_lookups.cpp`.
- Move-bundle key builders:
  `prepared_move_bundle_position_key`,
  `prepared_after_call_result_lane_position_key`, and
  `prepared_before_return_abi_move_source_bank_key` are declared in
  `prepared_lookups.hpp` and defined in `prepared_lookups.cpp`.
  `prepared_call_argument_position_key` is used by the before-call argument
  move index but is currently defined in `prepared_lookups.cpp` with no public
  declaration.
- Indexed move-bundle helpers:
  `find_indexed_prepared_move_bundle`,
  `find_indexed_prepared_before_call_argument_move`,
  `find_indexed_prepared_after_call_result_lane_binding`, and
  `find_prepared_before_return_abi_move_by_source_and_destination_bank` are
  declared in `prepared_lookups.hpp` and defined in `prepared_lookups.cpp`.
- After-call result-lane binding builders:
  `PreparedAfterCallResultLaneBinding` is declared in `prepared_lookups.hpp`;
  `append_prepared_after_call_result_lane_binding` and
  `publish_prepared_after_call_result_lane_bindings` are file-local helpers in
  `prepared_lookups.cpp`. The publisher is called only from
  `make_prepared_function_lookups` after `make_prepared_move_bundle_lookups`
  and before the aggregate `PreparedFunctionLookups` return.
- Current-block entry publication status/query:
  `PreparedCurrentBlockEntryPublicationStatus`,
  `PreparedCurrentBlockEntryPublicationQueryInputs`,
  `PreparedCurrentBlockEntryPublication`, and both
  `find_prepared_current_block_entry_publication` overloads are declared in
  `prepared_lookups.hpp` and defined in `prepared_lookups.cpp`. They depend on
  `PreparedValueHomeLookups`, `PreparedValueLocationFunction`, and
  block-entry publication collection from publication plans.

Consumer map:

- `make_prepared_function_lookups` owns aggregate wiring for value homes, move
  bundles, memory-access lookup value-id indexing, after-call result-lane
  binding publication, edge publications, and the final
  `PreparedFunctionLookups` aggregate.
- AArch64 module context stores `PreparedFunctionLookups*`,
  `PreparedMoveBundleLookups*`, and `PreparedValueHomeLookups*` in
  `src/backend/mir/aarch64/module/module.hpp`; traversal populates those from
  the aggregate in `src/backend/mir/aarch64/codegen/traversal.cpp`.
- Value-home lookup consumers include prealloc edge-publication,
  formal-publication, decoded-home-storage, current-block join routing, and
  AArch64 dispatch, calls, memory, ALU, comparison, select materialization,
  store retargeting, operands/prologue, and dispatch-publication code.
- `make_prepared_value_home_lookups` is also used for local fallback indexes in
  AArch64 memory and dispatch-producer paths when the function context lacks an
  aggregate lookup.
- Move-bundle lookup consumers include AArch64 calls via
  `find_indexed_prepared_move_bundle` and
  `find_indexed_prepared_before_call_argument_move`, AArch64 module/calls via
  after-call result-lane binding lookup, and AArch64 memory/ALU via
  before-return ABI move lookup.
- `find_prepared_current_block_publication_consumption` is declared through
  `select_chain_lookups.hpp` and defined in `prepared_lookups.cpp`, but it uses
  edge-publication source-producer lookups rather than value-home or move-bundle
  lookup ownership.

Owner/scope decision:

- Chosen value-location owner: move the concrete value-home lookup struct and
  builder toward the value-location ownership layer, but treat the indexed
  value-home/id helpers as value-location APIs for now because they are generic
  templates with scan fallback over `PreparedValueLocationFunction` and
  `PreparedRegallocFunction`.
- Chosen move-bundle owner: move the move-bundle lookup struct, public key
  builders, builder, indexed move-bundle helpers, before-call/before-return ABI
  move helpers, and after-call result-lane binding type/public lookup together
  with the move-bundle/value-location ownership layer. Keep the file-local
  after-call lane publisher coupled to aggregate lookup construction unless the
  move also provides the BIR/control-flow inputs it needs.
- Current-block entry publication status/query belongs in this route: yes,
  because the query resolves destination value ids/homes through
  `PreparedValueHomeLookups` and checks block-entry publication collection for
  the current block. `find_prepared_current_block_publication_consumption` does
  not belong in this route; it is source-producer/select-chain lookup work.

## Suggested Next

Execute Step 2 from `plan.md`: move only the value-home/value-location owned
lookup declarations/definitions first, preserving the generic indexed helper
fallbacks and `PreparedFunctionLookups` aggregate wiring.

## Watchouts

- This active idea is an ownership move only; do not change value-home
  construction, move-bundle semantics, call plans, return handling, or target
  emission policy.
- Keep `PreparedFunctionLookups` as the aggregate wiring point.
- Do not move return-chain lookups.
- Do not replace reusable lookup facts with local rescans.
- Only include current-block entry publication status/query if inspection
  proves it remains directly tied to value homes and block-entry publication
  collection.
- Do not mix in call-argument, comparison, load-local, current-block
  join-routing, or residual include cleanup work; those are separate open
  ideas.
- Keep `prepared_lookups.hpp` includes where a consumer still needs
  `PreparedFunctionLookups`, `make_prepared_function_lookups`, return-chain
  helpers, or another residual prepared lookup API.

## Proof

Inspection-only packet. No build or tests were delegated, and no
`test_after.log` is required.
