Status: Active
Source Idea Path: ideas/open/22_x86_prepared_edge_publication_coverage_broadening.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Current Consumer Surface

# Current Packet

## Just Finished

Completed plan Step 1, "Map Current Consumer Surface".

Current x86 prepared edge-publication consumer surface:
- `x86::prepared::consume_edge_publication_move_intent` reads
  `ConsumedPlans::shared_function_lookups()->edge_publications` with
  `find_unique_indexed_prepared_edge_publication(predecessor_label,
  successor_label, destination_value_id)`.
- `x86::prepared::append_edge_publication_move_instruction` is a thin emitter
  wrapper around that consumer. It appends text only when the returned status is
  `Available`.
- The only implementation call path is in
  `append_prepared_i32_compare_join_edge_copies`: prepared control-flow
  parallel-copy bundle -> prepared out-of-SSA move bundle -> each prepared
  parallel-copy step -> `append_edge_publication_move_instruction(output,
  consumed, predecessor_label, successor_label, value_location_move->to_value_id)`.
- `src/backend/mir/x86/core/core.cpp` has no direct consumer call. The x86
  query surface already exposes shared block-entry publications through
  `Query::collect_block_entry_publications`, but that path currently collects
  facts for tests and does not emit edge-publication moves.

Current home coverage in `consume_edge_publication_move_intent`:
- Supported: `Available` shared publication, move op, source home
  `PreparedValueHomeKind::StackSlot` with `offset_bytes`, destination home
  `PreparedValueHomeKind::Register` with `register_name`; emits
  `mov <dest-reg>, DWORD PTR [rsp + <offset>]`.
- Unsupported source homes: `None`, `Register`,
  `RematerializableImmediate`, `PointerBasePlusOffset`, and `StackSlot`
  without `offset_bytes`.
- Unsupported destination homes: `None`, `StackSlot`,
  `RematerializableImmediate`, `PointerBasePlusOffset`, and `Register`
  without `register_name`.
- Unsupported publication: non-available shared publication, missing prepared
  move, or prepared move op other than `Move`.

Selected broadening target:
- Add x86 prepared support for `Register` source home to `Register`
  destination home edge-publication moves. This is a home-family broadening
  target, not a named-testcase shortcut: it extends the existing shared
  publication consumer to a general x86-emittable move shape while preserving
  shared lookup authority and the existing unsupported-home statuses.

## Suggested Next

Implement Step 2/3 as one focused packet:
- Owned implementation files:
  `src/backend/mir/x86/prepared/dispatch.cpp` and, only if a status/type
  surface change is unavoidable, `src/backend/mir/x86/prepared/prepared.hpp`.
- Owned test files:
  `tests/backend/bir/backend_x86_prepared_decoded_home_storage_test.cpp` for
  direct helper coverage and
  `tests/backend/bir/backend_x86_handoff_boundary_joined_branch_test.cpp` for
  module-route authority coverage if the supervisor wants end-to-end proof in
  the same packet.
- Recommended exact proof command:
  `cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(backend_x86_prepared_decoded_home_storage|backend_x86_prepared_handoff_label_authority|backend_codegen_route_x86_64_.*|backend_prepared_lookup_helper|backend_prealloc_block_entry_publications)$' >> test_after.log 2>&1`

## Watchouts

- Preserve the shared lookup path; do not scan BIR edges or rebuild
  predecessor/successor publication semantics in x86.
- Keep stack-slot destination homes unsupported unless the next packet
  deliberately selects a memory-destination design and proves scratch/clobber
  policy.
- Register-to-register support should still preserve missing shared lookups,
  missing publication, unsupported publication, unsupported source home, and
  unsupported destination home as explicit statuses.
- Do not implement RISC-V consumer work in this plan.
- Do not weaken tests or supported-path expectations to claim progress.

## Proof

No build required and no root-level logs created; this was a mapping-only Step
1 packet.
