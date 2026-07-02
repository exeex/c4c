# Current Packet

Status: Complete
Source Idea Path: ideas/open/526_bir_route5_publication_body_extraction.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Route5 Boundaries

## Just Finished

Step 1 mapping completed for `ideas/open/526_bir_route5_publication_body_extraction.md`.
AST-backed queries mapped route5 definitions, declarations, direct callees,
direct callers, type references, and known MIR consumers before any body move.

Concrete route5 bodies to move from `src/backend/bir/bir.cpp` into the focused
route5 owner are:

- `route5_publication_source_kind` (`bir.cpp:194`)
- `route5_cfg_edge_publication_record` (`bir.cpp:215`)
- `route5_current_block_join_source_records` (`bir.cpp:346`)
- `route5_edge_destination_value_record` (`bir.cpp:427`)
- `route5_edge_source_value_record` (`bir.cpp:444`)
- `route5_join_destination_value_record` (`bir.cpp:461`)
- `route5_join_source_value_record` (`bir.cpp:478`)
- `route5_build_edge_join_source_index` (`bir.cpp:534`)
- `route5_find_cfg_edge_publication` (`bir.cpp:573`)
- `route5_find_current_block_join_source` (`bir.cpp:654`)

Private route5 helpers in the movement boundary are:

- `route5_find_block_by_label` (`bir.cpp:497`), currently anonymous namespace
- `route5_value_matches_record` (`bir.cpp:514`), currently anonymous namespace

Public route5 declarations and record/enumeration types stay in
`src/backend/bir/bir.hpp` at `bir.hpp:5059-5244`, including
`route5_publication_status_name`, all `Route5*` record types, and all public
route5 function declarations.

Dependency edges that must remain unchanged:

- route5 -> route1: `route1_source_value_identity`,
  `route1_build_producer_index`, and `route1_find_same_block_scalar_producer`
  remain the producer/value identity APIs used by CFG-edge and join-source
  publication.
- route5 -> route3: `route5_cfg_edge_publication_record` continues to use
  `route3_build_memory_access_index` and `route3_find_memory_access_record`
  only for `LoadLocal` source memory identity; do not duplicate route3 memory
  matching.
- route5 -> route4/shared matching: `route5_find_cfg_edge_publication` and
  `route5_find_current_block_join_source` call `route4_record_matches_block`.
  AST callers show only those two route5 functions call it today. Preserve this
  logic exactly; do not duplicate or rewrite route4 block matching inside
  route5.
- route5 -> MIR consumers: `src/backend/mir/query.cpp` calls
  `route5_build_edge_join_source_index`, `route5_find_cfg_edge_publication`,
  and `route5_current_block_join_source_records`; x86 publication agreement in
  `src/backend/mir/x86/module/module.cpp` builds `Route5EdgeJoinSourceIndex`
  and reads `edge_records`. These consumers should require no semantic changes.

Route6 proof requirement: not required for the mapped subset. AST callee checks
for route6 publication-source functions show route6 consumes route1, route3,
route4, and call-argument routing records directly; it does not directly call
route5 APIs or consume route5 record types in this boundary.

Expected new file for the body move: `src/backend/bir/bir_route5_publication.cpp`
unless the next packet identifies an already-suitable focused owner. Add only
the build registration needed for that file in the implementation packet.

Private/shared helper constraints for the move:

- Keep `route5_find_block_by_label` and `route5_value_matches_record`
  file-private in the route5 owner.
- Keep public declarations in `bir.hpp`; do not split route5 declarations into
  a new public header.
- If `route4_record_matches_block` must cross translation units, expose the
  smallest internal/private declaration or move the unchanged helper with the
  route5 body only after confirming no route4 caller is lost. Do not duplicate
  the route4 matching rule.
- Keep route1 and route3 APIs as dependencies rather than copying producer or
  memory-access logic into route5.

## Suggested Next

Delegate Step 2 to an executor: add `src/backend/bir/bir_route5_publication.cpp`
and move the mapped route5 bodies plus file-private route5 helpers without
changing declarations, record order, or route behavior.

## Watchouts

- Keep this slice behavior-preserving.
- Do not move route5 public declarations out of `bir.hpp`.
- Do not duplicate route3 or route4 matching logic inside route5.
- Do not combine this with route6 extraction.
- `route4_record_matches_block` is the only non-route5 local helper in the
  mapped movement boundary; preserve it unchanged if the body move needs a
  private cross-file surface.
- MIR query and x86 publication-agreement consumers read route5 records after
  the BIR index is built, so the move must not change route5 record ordering,
  selection, labels, status values, or source-producer fields.

## Proof

Not run; mapping-only packet, and the delegated proof explicitly required no
build or CTest proof.

Selected future proof command for the first implementation packet:

`cmake --build build -j --target c4c_backend backend_prepared_lookup_helper_test backend_aarch64_current_block_join_routing_test backend_riscv_prepared_edge_publication_test backend_lir_to_bir_notes_test && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_aarch64_current_block_join_routing|backend_riscv_prepared_edge_publication|backend_lir_to_bir_notes)$' --output-on-failure`

Route6 proof is not included because the mapped route6 subset does not directly
consume route5 records.
