Status: Active
Source Idea Path: ideas/open/66_aarch64_local_dispatch_block_route.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Remove Residual Semantic Drift From Block Dispatch

# Current Packet

## Just Finished

Step 5 completed: scanned AArch64 block dispatch for residual producer,
publication, value-materialization, prepared-query, and target-emission
authority, then removed one small local semantic lookup.

Cleanup made:
- `instruction_result_has_stack_home` now asks the publication owner API
  `prepared_value_home_for_value` instead of resolving prepared value ids and
  indexing prepared homes directly in `dispatch.cpp`.
- The now-unused local `prepared_named_value_id` helper was removed from
  `dispatch.cpp`.

Residual route classification:
- Current-block join suppression remains a route adapter:
  `build_current_block_join_prepared_query_routing`,
  `current_block_join_prepared_query_incoming_expression`, and
  `current_block_join_prepared_query_source` come from the prepared-query
  routing owner; `dispatch.cpp` only uses their booleans to skip already-owned
  incoming/source instructions when the result is not stack-homed.
- Producer/publication/value-materialization hits are owner calls sequenced by
  the route: call-boundary moves, block-entry publications, before-return
  moves, address materialization, scalar/memory result recording, retargeting,
  and branch-fusion hooks remain local ordering decisions around precise owner
  APIs.
- Target-emission spelling scans found no local instruction factory or
  assembly spelling authority in `dispatch.cpp`; no hits for
  `make_bir_machine_instruction`, `AssemblerInstructionRecord`,
  `inline_asm_template`, `adrp`, GOT labels, frame-slot address spelling,
  scalar load mnemonics, or relocated global helper names.
- `dispatch.hpp` public entries remain unchanged: `make_block_lowering_context`
  and `dispatch_prepared_block` are still the durable public block route
  surface.

## Suggested Next

Step 6 should prove local dispatch route preservation and record closure
readiness. Suggested packet: run the delegated final backend proof, confirm the
Step 2 private scaffolding remains retired, confirm Step 5 removed raw prepared
home lookup authority from `dispatch.cpp`, and record any residual route-local
surface that plan-owner closure should evaluate.

## Watchouts

This plan preserves the local AArch64 block route. Do not move block traversal,
diagnostics, hook sequencing, prepared-memory retry routing, or before-return
ordering out of `dispatch.cpp` during closure proof. Do not reintroduce raw
prepared-home or move-bundle reconstruction into the route, and do not bundle
idea 67's local-slot offset probe into this route.

Residual route adapters to justify during Step 6: `instruction_result_value`
still extracts BIR result identity only so the route can preserve stack-homed
current-block join producers, `make_dispatch_branch_fusion_hooks` still wires
owner callbacks into branch fusion, and before-return duplicate filtering still
records emitted ABI publications in local scalar state.

## Proof

Proof passed:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_|backend_cli_aarch64_)') > test_after.log 2>&1`

`test_after.log` reports 32/32 tests passed. `git diff --check` passed.

Route-quality scans run:
- `rg -n "resolve_prepared_value_name_id|find_indexed_prepared_value_home|prepared_named_value_id" src/backend/mir/aarch64/codegen/dispatch.cpp`
  found no remaining local raw prepared-home lookup authority.
- `rg -n "make_bir_machine_instruction|AssemblerInstructionRecord|inline_asm_template|adrp|:got|frame_slot_address|scalar_load_mnemonic|load_global_symbol_label|find_load_global_target" src/backend/mir/aarch64/codegen/dispatch.cpp`
  found no target-emission spelling authority.
- Targeted scans for prepared-query, producer, publication,
  value-materialization, retargeting, result recording, value moves, and
  before-return moves found only route-level orchestration around owner APIs.
