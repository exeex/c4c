Status: Active
Source Idea Path: ideas/open/193_route3_prepared_policy_boundary_hardening.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Route 3 Memory/Source Consumers

# Current Packet

## Just Finished

Completed Step 1 inventory of Route 3 memory/source consumers.

Route-view accessors and declarations:

- `bir::route3_memory_access_record`, `route3_memory_access_result_value_record`,
  `route3_memory_access_stored_value_record`, `route3_build_memory_access_index`,
  and `route3_find_memory_access_record` are route-first eligible for
  target-neutral memory identity only: node kind, block/instruction position,
  result/stored value identity, base kind, local/global/string/pointer source
  names, byte range, address space, and volatility.
- `bir::route3_same_block_global_load_access_record` and
  `bir::route3_find_same_block_global_load_access` are route-first eligible for
  same-block global-load source identity. Final AArch64 global addressing policy
  remains fallback-required/target-policy-owned.
- `bir::route3_same_block_load_local_source_record` and
  `bir::route3_find_same_block_load_local_source` are route-first eligible for
  same-block load-local source identity when Route 3 has exact local-slot facts.
  Prepared fallback remains required when the route facts are absent,
  mismatched, future-positioned, non-local, or invalidated by stores.
- `bir::route3_find_same_block_load_local_stored_value_source` is route-first
  eligible only for exact same-slot same-range load/store source identity.
  Prepared fallback is required when Route 3 lacks range authority, overlaps are
  ambiguous, or prepared source-producer facts recover a source that Route 3
  cannot prove.

Direct consumers and classification:

- `mir::find_bir_memory_access_identity` is a retained route-view facade over
  `route3_find_memory_access_record`; route-first eligible for semantic
  memory-access identity, but not final AArch64 operand policy.
- `mir::find_bir_same_block_global_load_access_identity` is route-first
  eligible for locating the same-block global-load producer. AArch64 consumers
  in `dispatch_value_materialization.cpp` and `fp_value_materialization.cpp`
  still require `prepared_memory_access_matches_instruction` and prepared
  global-load rendering before emitting target instructions.
- `mir::find_bir_same_block_load_local_source_identity` is route-first eligible
  for local source identity and is currently covered by prepared oracle tests.
  AArch64 scalar/load-source consumers still need prepared frame-slot or source
  publication authority for target operands.
- `mir::find_bir_same_block_load_local_stored_value_source_identity` is a
  route-first eligible source-identity facade for exact same-range local
  store/load chains. Its AArch64 indirect-callee consumer in `calls.cpp`
  intentionally falls back to
  `prepare::find_prepared_same_block_load_local_stored_value_source` when Route
  3 cannot answer.
- `aarch64_codegen::prepared_memory_access`,
  `prepared_memory_access_matches_instruction`,
  `make_prepared_memory_operand_record`,
  `make_prepared_load_memory_instruction_record`, and
  `make_prepared_store_memory_instruction_record` are fallback-required and
  target-policy-owned. They remain authoritative for prepared addressing,
  frame-slot conversion, pointer-value homes, global/string identity,
  address-space/volatility checks, result/store storage, and final memory
  instruction records.
- AArch64 global-load emitters in `globals.cpp`, `dispatch_value_materialization.cpp`,
  and `fp_value_materialization.cpp` are target-policy-owned. Route 3 may locate
  the semantic load producer, but prepared/global target policy still chooses
  GOT versus direct addressing and relocation operands.
- AArch64 dispatch memory-index retry logic in `dispatch.cpp` is
  fallback-required. It keeps prepared memory indexes usable when retained BIR
  instruction indexes and prepared memory-only indexes diverge.
- `dispatch_edge_copies.cpp`, `alu.cpp`, `memory.cpp`, `frame_slot_address.cpp`,
  `comparison.cpp`, `fp_value_materialization.cpp`, and related AArch64 helpers
  are prepared-consumer surfaces, not Route 3 authority surfaces, whenever they
  ask prepared lookups for source memory, frame offsets, recovered narrow-store
  sources, or materialized target operands.

Retained public prepared oracle surfaces:

- `prepare::find_prepared_memory_access`,
  `find_indexed_prepared_memory_access`,
  `find_unique_indexed_prepared_memory_access_by_result_value_name`, and
  `find_unique_indexed_prepared_memory_access_by_result_value_id`.
- `prepare::find_prepared_global_load_access` and
  `find_prepared_same_block_global_load_access`.
- `prepare::find_prepared_same_block_load_local_source_producer`.
- `prepare::find_prepared_same_block_load_local_stored_value_source`.
- `aarch64_codegen::make_prepared_memory_operand_record`,
  `make_prepared_load_memory_instruction_record`, and
  `make_prepared_store_memory_instruction_record`.

Nearby same-feature proof gaps:

- Add a narrow AArch64 consumer-boundary proof that a Route 3 global-load
  identity still refuses emission when `prepared_memory_access_matches_instruction`
  fails or prepared GOT/direct policy is missing.
- Add a focused indirect-callee/materialization proof showing the Route 3
  stored-value source is accepted for exact same-range local load/store, while
  the prepared fallback is used when Route 3 lacks range authority.
- Keep direct-memory mismatch coverage for result/stored value, symbol,
  offset/size, address space, volatility, pointer value, string identity, and
  missing structured address; do not claim policy readiness from route-only
  identity tests.
- Keep prepared memory-index divergence covered before moving any consumer to
  route-first lookup, because `dispatch.cpp` still has a prepared-memory-index
  retry path.

## Suggested Next

Execute Step 2 by choosing the narrowest consumer boundary, likely the AArch64
global-load or indirect-callee stored-value-source path, and make the
route-first versus prepared-fallback rule explicit without moving target policy
into Route 3 facts.

## Watchouts

- Do not move AArch64 addressing, frame, relocation, volatility,
  materialization, or final operand policy into Route 3 facts.
- Do not delete prepared memory/source helpers during the inventory step.
- Do not claim progress through expectation rewrites, unsupported downgrades,
  helper renames, or named-case-only fixes.
- The AST caller query is translation-unit scoped and did not report
  cross-translation-unit callers for Route 3 helpers; direct consumer inventory
  used AST-backed declarations plus targeted `rg`.

## Proof

Docs/lifecycle-only Step 1 inventory; no build or test proof required, and no
`test_after.log` was produced by this packet.
