Status: Active
Source Idea Path: ideas/open/140_edge_copy_facade_split.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect current edge-copy facade ownership and dependencies

# Current Packet

## Just Finished

Completed `plan.md` Step 1 inspection for the edge-copy facade split.

Declaration locations:

- `src/backend/prealloc/prepared_lookups.hpp:40`: `PreparedMoveBundleLookups`.
- `src/backend/prealloc/prepared_lookups.hpp:123`: `PreparedEdgeCopySourceFactsStatus`.
- `src/backend/prealloc/prepared_lookups.hpp:184`: `PreparedCurrentBlockJoinParallelCopySourceStatus`.
- `src/backend/prealloc/prepared_lookups.hpp:248`: `PreparedAggregateStackSourceAuthorityStatus`.
- `src/backend/prealloc/prepared_lookups.hpp:283`: `PreparedAggregateStackSourceAuthority`.
- `src/backend/prealloc/prepared_lookups.hpp:382`: `PreparedEdgePublication`.
- `src/backend/prealloc/prepared_lookups.hpp:443`: `PreparedEdgePublicationKey`.
- `src/backend/prealloc/prepared_lookups.hpp:456`: `PreparedEdgePublicationLookups`.
- `src/backend/prealloc/prepared_lookups.hpp:464`: `PreparedEdgeCopySourceFacts`.
- `src/backend/prealloc/prepared_lookups.hpp:509`: `PreparedCurrentBlockJoinParallelCopySourceFact`.
- `src/backend/prealloc/prepared_lookups.hpp:535`: `PreparedCurrentBlockJoinParallelCopySourceFacts`.
- `src/backend/prealloc/prepared_lookups.hpp:545`: `PreparedCurrentBlockJoinParallelCopySourceQueryInputs`.
- `src/backend/prealloc/prepared_lookups.hpp:572`: `PreparedCurrentBlockJoinParallelCopyInstructionRouting`.
- `src/backend/prealloc/prepared_lookups.hpp:584`: `PreparedTypedStackSourcePublicationStatus`.
- `src/backend/prealloc/prepared_lookups.hpp:628`: `PreparedTypedStackSourceExtensionPolicy`.
- `src/backend/prealloc/prepared_lookups.hpp:645`: `PreparedTypedStackSourcePublication`.
- `src/backend/prealloc/prepared_lookups.hpp:667`: `PreparedFunctionLookups` aggregate field owner.
- `src/backend/prealloc/prepared_lookups.hpp:698`: `make_prepared_move_bundle_lookups`.
- `src/backend/prealloc/prepared_lookups.hpp:708` and `:714`: public `make_prepared_edge_publication_lookups` overloads.
- `src/backend/prealloc/prepared_lookups.hpp:765`: `prepare_aggregate_stack_source_authority`.
- `src/backend/prealloc/prepared_lookups.hpp:769`: `prepare_same_width_i32_stack_source_publication`.
- `src/backend/prealloc/prepared_lookups.hpp:926`: `find_indexed_prepared_edge_publications`.
- `src/backend/prealloc/prepared_lookups.hpp:933`: `find_unique_indexed_prepared_edge_publication`.
- `src/backend/prealloc/prepared_lookups.hpp:939`: `prepare_edge_copy_source_facts`.
- `src/backend/prealloc/prepared_lookups.hpp:945`: `find_unique_indexed_block_entry_parallel_copy_edge_publication`.
- `src/backend/prealloc/prepared_lookups.hpp:952`: `prepare_block_entry_parallel_copy_edge_source_facts`.
- `src/backend/prealloc/prepared_lookups.hpp:959`: `prepare_current_block_join_parallel_copy_source_facts`.
- `src/backend/prealloc/prepared_lookups.hpp:963`: `prepare_current_block_join_parallel_copy_instruction_routing`.

Definition and construction wiring locations:

- `src/backend/prealloc/prepared_lookups.cpp:1365`: builds `PreparedMoveBundleLookups`; `:335` publishes after-call result lane bindings into the same aggregate.
- `src/backend/prealloc/prepared_lookups.cpp:1589`, `:1752`, `:1761`: build `PreparedEdgePublicationLookups`; the internal overload publishes edge facts from `PreparedControlFlowFunction::join_transfers`, value homes, source producers, memory accesses, matching move bundles, and parallel-copy steps.
- `src/backend/prealloc/prepared_lookups.cpp:1733`: stores `PreparedAggregateStackSourceAuthority` into each available `PreparedEdgePublication` during lookup construction.
- `src/backend/prealloc/prepared_lookups.cpp:1770`: `make_prepared_function_lookups` wires `.move_bundles` and `.edge_publications` into `PreparedFunctionLookups`; this aggregate construction should stay in `prepared_lookups.cpp`.
- `src/backend/prealloc/prepared_lookups.cpp:1878`: `prepare_aggregate_stack_source_authority`.
- `src/backend/prealloc/prepared_lookups.cpp:1960`: `prepare_same_width_i32_stack_source_publication`.
- `src/backend/prealloc/prepared_lookups.cpp:3015`: `find_indexed_prepared_edge_publications`.
- `src/backend/prealloc/prepared_lookups.cpp:3031`: `find_unique_indexed_prepared_edge_publication`.
- `src/backend/prealloc/prepared_lookups.cpp:3066`, `:3113`: local copy/validation helpers for `PreparedEdgeCopySourceFacts`.
- `src/backend/prealloc/prepared_lookups.cpp:3192`: `prepare_edge_copy_source_facts`.
- `src/backend/prealloc/prepared_lookups.cpp:3246`: `find_unique_indexed_block_entry_parallel_copy_edge_publication`.
- `src/backend/prealloc/prepared_lookups.cpp:3279`: `prepare_block_entry_parallel_copy_edge_source_facts`.
- `src/backend/prealloc/prepared_lookups.cpp:3341`: `prepare_current_block_join_parallel_copy_source_facts`.
- `src/backend/prealloc/prepared_lookups.cpp:3569`: `prepare_current_block_join_parallel_copy_instruction_routing`.

Classification:

- Reusable target-neutral edge facts: `PreparedMoveBundleLookups`; `PreparedEdgePublication`, `PreparedEdgePublicationKey`, `PreparedEdgePublicationLookups`; `PreparedEdgeCopySourceFactsStatus`; `PreparedEdgeCopySourceFacts`; edge publication source producer and source memory status/data fields; `PreparedAggregateStackSourceAuthority` and status because it is cached on `PreparedEdgePublication` and describes source/destination authority without target instruction spelling; `PreparedTypedStackSourcePublication`, status, and extension policy because both AArch64 and RISC-V consume it; `find_indexed_prepared_edge_publications`; `find_unique_indexed_prepared_edge_publication`; `prepare_edge_copy_source_facts`; `find_unique_indexed_block_entry_parallel_copy_edge_publication`; `prepare_block_entry_parallel_copy_edge_source_facts`; `prepared_edge_copy_source_facts_have_materializable_producer`; `prepared_edge_publication_source_home_matches_source`; `prepared_edge_publication_source_memory_matches_access`.
- Target-facing routing convenience: `PreparedCurrentBlockJoinParallelCopySourceStatus`, `PreparedCurrentBlockJoinParallelCopySourceFact`, `PreparedCurrentBlockJoinParallelCopySourceFacts`, `PreparedCurrentBlockJoinParallelCopySourceQueryInputs`, `PreparedCurrentBlockJoinParallelCopyInstructionRouting`, `prepare_current_block_join_parallel_copy_source_facts`, and `prepare_current_block_join_parallel_copy_instruction_routing`.
- Target-facing fields on `PreparedCurrentBlockJoinParallelCopySourceFact`: `destination_register_name`, `source_is_incoming_expression`, `destination_is_source_value`, `source_is_source_value`, `source_shares_destination_register`, `source_home_is_stack`, `immediate_source`. The base edge identity/source/home fields in the same struct mirror reusable `PreparedEdgeCopySourceFacts`, but the struct itself is AArch64 dispatch routing shape.
- Cached construction/aggregate wiring: `make_prepared_move_bundle_lookups`, `make_prepared_edge_publication_lookups`, the local publication copy/validation helpers, `make_prepared_function_lookups`, and the `PreparedFunctionLookups::{move_bundles,edge_publications}` fields.

Consumer and include constraints:

- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp` includes `prepared_lookups.hpp` and is the only direct external consumer of `PreparedCurrentBlockJoinParallelCopy*`; it has local wrappers at `:79`, `:335`, and `:352` that already form an AArch64-local facade.
- `src/backend/mir/aarch64/codegen/dispatch_producers.hpp:5` includes `prepared_lookups.hpp` for `PreparedEdgePublicationSourceProducer`; do not move broad current-block join routing declarations there unless the implementation needs them.
- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp:1105`, `:1167`, and `:1206` uses shared edge facts plus `prepare_same_width_i32_stack_source_publication`.
- `src/backend/mir/aarch64/codegen/memory.hpp:174` and `src/backend/mir/aarch64/codegen/memory.cpp:1357`, `:1979`, `:2669` use `PreparedEdgePublicationLookups` and `prepare_same_width_i32_stack_source_publication`.
- `src/backend/mir/aarch64/codegen/instruction.hpp:1871` declares memory helpers that take `PreparedEdgePublicationLookups`.
- `src/backend/mir/riscv/codegen/emit.cpp:374` also uses `prepare_same_width_i32_stack_source_publication`; this helper must remain shared or be given a shared name, not moved AArch64-local.
- `src/backend/mir/aarch64/codegen/prologue.cpp` is in the known consumer list but does not directly use the edge-copy facade targets found by AST/rg.
- `src/backend/prealloc/publication_plans.hpp` currently includes `prepared_lookups.hpp`, so moving reusable edge declarations into `publication_plans.hpp` needs care to avoid a self-include cycle.

Exact Step 2 facade boundary:

- Move reusable declaration ownership for edge publication/source facts out of `prepared_lookups.hpp` into `src/backend/prealloc/publication_plans.hpp`: `PreparedEdgeCopySourceFactsStatus`, `PreparedEdgePublicationSourceProducerKind`, `PreparedEdgePublicationSourceMemoryAccessStatus`, `PreparedAggregateStackSourceAuthorityStatus`, `PreparedAggregateStackSourceAuthority`, `PreparedEdgePublicationSourceProducer`, `PreparedEdgePublication`, `PreparedEdgePublicationKey`, `PreparedEdgePublicationKeyHash`, `PreparedEdgePublicationLookups`, `PreparedEdgeCopySourceFacts`, `PreparedTypedStackSourcePublicationStatus`, `PreparedTypedStackSourceExtensionPolicy`, `PreparedTypedStackSourcePublication`, and the shared edge/publication query declarations listed above.
- Keep `PreparedMoveBundleLookups` in `prepared_lookups.hpp` for Step 2 unless the supervisor widens the packet to a value-location lookup split; it is reusable but not specific to the edge-publication facade and is already tied to broad `PreparedFunctionLookups` construction.
- Keep `make_prepared_edge_publication_lookups` declarations and definitions in `prepared_lookups.*` for Step 2; they are cached lookup construction and depend on `PreparedBirModule`, control-flow, value homes, addressing, source producers, and function aggregate wiring.
- Keep `PreparedFunctionLookups` and its `.edge_publications` field in `prepared_lookups.hpp`; only its field type should come from the new shared declaration owner.
- Do not move definitions in Step 2 except any inline constexpr status-name helpers required with moved enum declarations; non-inline implementation should remain in `prepared_lookups.cpp` until Step 3.
- For current-block join routing, prefer target-local declaration ownership in `src/backend/mir/aarch64/codegen/dispatch_producers.hpp` for `PreparedCurrentBlockJoinParallelCopySourceStatus`, `PreparedCurrentBlockJoinParallelCopySourceFact`, `PreparedCurrentBlockJoinParallelCopySourceFacts`, `PreparedCurrentBlockJoinParallelCopySourceQueryInputs`, `PreparedCurrentBlockJoinParallelCopyInstructionRouting`, and the two `prepare_current_block_join_parallel_copy_*` declarations. If dependency direction makes target-local declaration movement too wide in Step 2, rename the shared declarations with an explicit AArch64/current-block routing prefix rather than leaving them presented as generic prepared lookup facts.
- Do not move `prepare_same_width_i32_stack_source_publication` AArch64-local; RISC-V uses it. If renamed, use a shared target-facing name such as `prepare_target_same_width_i32_stack_source_publication`.

## Suggested Next

Execute Step 2 from `plan.md`: move declaration ownership for reusable edge
publication/source facts to `publication_plans.hpp`, keep cached construction
and `PreparedFunctionLookups` wiring in `prepared_lookups.*`, and move or
explicitly rename only the current-block join routing convenience declarations.

## Watchouts

- Do not edit implementation files during Step 1.
- Do not change control-flow semantics, out-of-SSA parallel-copy generation,
  dispatch lowering, memory lowering, or predecessor/successor transfer
  identity.
- Do not replace prepared edge facts with predecessor rescans, BIR rediscovery,
  local value-location scans, or named-case shortcuts.
- Keep reusable edge publication and move-bundle facts available to non-AArch64
  backends.
- `publication_plans.hpp` currently includes `prepared_lookups.hpp`; Step 2 may
  need include-cycle cleanup if declarations move there.
- `prepare_same_width_i32_stack_source_publication` is used by both AArch64 and
  RISC-V; do not move it into an AArch64-only header.
- `prepare_current_block_join_parallel_copy_instruction_routing` calls
  `prepare_current_block_join_parallel_copy_source_facts`; if routing
  declarations move target-local, both declarations should move together.

## Proof

Inspection-only packet. Used `c4c-clang-tool`/`c4c-clang-tool-ccdb` for
function signatures, definitions, type refs, and direct callers, supplemented
by `rg` include and consumer scans. No build was required and no test logs were
created.
