# Current Packet

Status: Active
Source Idea Path: ideas/open/81_aarch64_dispatch_edge_copy_owner_contraction.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Relocate narrow target-local emission helpers

## Just Finished

Completed `plan.md` Step 1 by inventorying `dispatch_edge_copies.*` with
`c4c-clang-tool-ccdb function-signatures`, `c4c-clang-tool-ccdb list-symbols`,
and focused call-site reads. No implementation files were changed.

Helper classification:

| Helper | Classification | Current owner recommendation |
| --- | --- | --- |
| `prepared_named_value_id` | source fact validation support | Private utility used by source matching and memory-access validation; keep near its consumers until a larger prepared-source helper extraction is justified. |
| `prepared_edge_select_source_is_destination_register` | source fact validation | Public thin prepared-home predicate; can stay in edge-copy surface for now because it names edge-select semantics and does not drive the first contraction. |
| `prepared_edge_publication_producer_block_context` | producer-context lookup | Candidate dispatch/edge producer support helper; keep with producer-context family until orchestration helpers move together. |
| `prepared_edge_publication_source_matches_value` | source fact validation | Prepared-publication guard used before materialization; keep with edge publication emission helpers. |
| `prepared_edge_publication_producer_context` | producer-context lookup | Core prepared edge publication producer lookup; keep with edge publication emission helpers unless dispatch producer owner takes the whole family. |
| `prepared_edge_source_producer_context` | producer-context lookup | Core prepared source-producer lookup; same family as `prepared_edge_publication_producer_context`. |
| `prepared_edge_named_source_producer_context` | producer-context lookup | Prepared named-value producer lookup; same producer-context family. |
| `prepared_edge_copy_source_facts_have_materializable_producer` | source fact validation | Edge-copy fact predicate used by block-entry filtering and select lowering; keep near those edge-copy consumers. |
| `EdgeSelectChainState` | dispatch orchestration | Private recursion-cycle state for edge value/select lowering; keep with `emit_edge_value_publication_to_register_impl`. |
| `prepared_memory_access` | source fact validation / memory lookup | First contraction candidate: move from `dispatch_edge_copies.*` to `memory.*`; call sites already include `memory.cpp`, `dispatch.cpp`, and `dispatch_value_materialization.cpp`. |
| `prepared_memory_access_matches_instruction` | source fact validation / memory lookup | Move with `prepared_memory_access` to `memory.*`; it validates prepared memory facts against BIR load/store instructions. |
| `prepared_publication_source_home_matches_source` | source fact validation | Private prepared-publication consistency check; keep with edge publication source validation. |
| `prepared_publication_source_memory_matches_access` | source fact validation / memory lookup | Prepared memory fact validation used only by edge load-local emission; do not move in the first packet because it is coupled to prepared edge publication semantics. |
| `prepared_publication_source_memory_access` | source fact validation / memory lookup | Private edge-publication source-memory resolver; keep with load-local edge emission until that family is relocated. |
| `prepared_publication_source_register` | source fact validation | Private prepared-publication source-register resolver; keep with load-local edge emission fallback. |
| `emit_prepared_va_list_field_carrier_to_register` | load-local emission | Private load-local special case; candidate for `variadic.*` or memory owner only if a later packet moves the load-local edge emission family. |
| `edge_value_publication_may_read_register_index` | source fact validation / dispatch orchestration | Public dependency-clobber query for edge publication/block-entry filtering; keep in edge-copy surface until block-entry filtering is folded. |
| `emit_edge_load_local_to_register` | load-local emission | Public wrapper around load-local edge materialization; later packet candidate with `emit_edge_load_local_to_register_impl`, likely memory-facing but still edge-publication-coupled. |
| `emit_edge_load_local_to_register_impl` | load-local emission | Private load-local edge materialization; keep with `emit_edge_value_publication_to_register_impl` until source-recursion coupling is split. |
| `emit_edge_value_publication_to_register` | dispatch orchestration / source publication emission | Public prepared edge-value materialization wrapper; keep in edge-copy surface until orchestration relocation is planned. |
| `emit_edge_value_publication_to_register_impl` | dispatch orchestration / source publication emission | Private recursive edge-value materializer for immediate, home, load, cast, binary, and select producers; do not split in the first packet. |
| `PreparedTypedStackSourceEdgeEmission` | typed stack-source emission | Private result carrier for typed stack-source edge publication; keep with `emit_same_width_i32_stack_source_publication`. |
| `emit_same_width_i32_stack_source_publication` | typed stack-source emission | Private typed stack-source emission helper used by predecessor join publication; candidate for a later typed-stack-source packet, not the first move. |
| `lower_predecessor_join_source_publication` | select parallel-copy lowering / typed stack-source emission | Public join-source publication lowering; keep with select parallel-copy lowering until that family is moved as one packet. |
| `should_emit_block_entry_edge_copy_move` | block-entry move filtering | Public dispatch filter used by `dispatch.cpp`; later candidate to fold into `dispatch.cpp` with its private validation dependencies if ordering can be kept intact. |
| `lower_predecessor_select_parallel_copy_sources` | select parallel-copy lowering | Public dispatch hook for predecessor select parallel-copy source lowering; later candidate to fold into `dispatch.cpp` only with `lower_predecessor_join_source_publication` accounted for. |

## Suggested Next

Active bounded contraction packet for `plan.md` Step 3:

- Objective: move only `prepared_memory_access` and
  `prepared_memory_access_matches_instruction` declarations/definitions from
  `dispatch_edge_copies.*` to `memory.*`.
- Owned files: `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`,
  `src/backend/mir/aarch64/codegen/dispatch_edge_copies.hpp`,
  `src/backend/mir/aarch64/codegen/memory.cpp`,
  `src/backend/mir/aarch64/codegen/memory.hpp`, and `todo.md`.
- Scope: update includes/call sites as needed, preserve signatures and behavior
  exactly, and leave the edge-publication memory-source helpers in
  `dispatch_edge_copies.cpp`.
- Recommended proof command:
  `cmake --build build --target c4c_backend c4c_tests -- -j2 && ctest --test-dir build -R 'backend_aarch64_(prepared_memory_operand_records|memory_operand_contract|instruction_dispatch)|backend_codegen_route_aarch64_(got_load_global_prepared_memory|pointer_value_named_scalar_writeback_uses_computed_store_value)' --output-on-failure | tee test_after.log`

## Watchouts

- The first packet is a header/source ownership contraction, not a semantic
  rewrite; do not rederive prepared memory facts or alter prepared lookup
  behavior.
- `prepared_memory_access` is already called from `memory.cpp`, `dispatch.cpp`,
  and `dispatch_value_materialization.cpp`, so moving it to `memory.*` should
  reduce the false edge-copy dependency without touching edge-copy ordering.
- Leave `prepared_publication_source_memory_matches_access` and
  `prepared_publication_source_memory_access` in `dispatch_edge_copies.cpp` for
  now because they combine memory facts with prepared edge-publication source
  authority.
- Do not re-derive prepared edge-copy or typed stack-source facts locally.
- Do not mix dispatch-publication relocation into this plan.
- Do not change edge-copy ordering behavior.
- Do not weaken edge-copy or block-entry publication expectations.

## Proof

No code proof required or run for this inventory-only Step 1 packet. No
`test_after.log` was produced. The recommended proof command for the first
implementation packet is recorded in `## Suggested Next`.
