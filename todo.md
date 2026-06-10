Status: Active
Source Idea Path: ideas/open/154_bir_memory_access_identity.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Locate Prepared Memory Surface And Evidence Set

# Current Packet

## Just Finished

Step 1 evidence inventory is complete without implementation edits.

Inspected owner surfaces:
- `src/backend/prealloc/addressing.hpp`: `PreparedAddress`,
  `PreparedMemoryAccess`, `PreparedSameBlockGlobalLoadAccess`,
  `PreparedSameBlockLoadLocalStoredValueSource`,
  `PreparedAddressingFunction`, `PreparedMemoryAccessLookups`,
  `find_prepared_memory_access*`,
  `find_prepared_global_load_access`,
  `find_prepared_same_block_global_load_access`, and
  `find_prepared_same_block_load_local_stored_value_source`.
- `src/backend/prealloc/prepared_lookups.cpp`: lookup construction and bodies
  for `make_prepared_memory_access_lookups`,
  `find_indexed_prepared_memory_access*`,
  `prepared_load_access_matches_result`,
  `prepared_global_load_access_matches_result`,
  `prepared_store_access_matches_value`,
  `prepared_access_ranges_overlap`,
  `prepared_access_ranges_match`,
  `find_prepared_global_load_access`,
  `find_prepared_same_block_global_load_access`, and
  `find_prepared_same_block_load_local_stored_value_source`.
- `src/backend/prealloc/publication_plans.hpp/.cpp`:
  `PreparedScalarLoadLocalSourceProducer` and
  `find_prepared_same_block_load_local_source_producer`.
- Context docs: `docs/bir_prealloc_fusion/phase_a_normalization_candidates.md`
  memory/access rows and `ideas/open/154_bir_memory_access_identity.md`.

Direct callers and consumers:
- `find_prepared_global_load_access`: called by
  `find_prepared_same_block_global_load_access` in
  `src/backend/prealloc/prepared_lookups.cpp` and by
  `prepared_current_global_load_access` in
  `src/backend/mir/aarch64/codegen/globals.cpp`.
- `find_prepared_same_block_global_load_access`: called by
  `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp` and
  `src/backend/mir/aarch64/codegen/fp_value_materialization.cpp` for
  same-block `LoadGlobal` materialization.
- `find_prepared_same_block_load_local_stored_value_source`: called by
  `find_prepared_indirect_callee_stored_value_source` in
  `src/backend/mir/aarch64/codegen/calls.cpp`.
- `find_prepared_same_block_load_local_source_producer`: called by
  `find_prepared_load_local_source_producer` in
  `src/backend/mir/aarch64/codegen/alu.cpp` and directly tested in
  `tests/backend/mir/backend_store_source_publication_plan_test.cpp`.
- Indexed memory-access helpers are built into
  `PreparedFunctionLookups::memory_accesses` by
  `make_prepared_function_lookups` and are covered by
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`.

Semantic fields to carry into the BIR-owned memory identity:
- Operation position and identity: function name if needed for lookup context,
  block label, instruction index, memory node kind, result/stored value name,
  and BIR operation identity.
- Access semantics: `address_space`, `is_volatile`,
  semantic base kind (`FrameSlot` as BIR local slot/name identity,
  `GlobalSymbol`, `PointerValue`, `StringConstant`), link/text/value/slot name
  identity, load/store result or stored-value relation, same-block
  `LoadGlobal` link, same-block `LoadLocal` source producer, and prior
  store/source value link.
- Negative semantics: missing prepared access, missing/ambiguous result access,
  result/stored value mismatch, symbol/pointer/string mismatch, missing source
  producer, unavailable value name, intervening store conflict, missing prior
  matching store/source, and no-source cases.

Rejected target/layout/addressing fields that must not become BIR authority:
- Prepared frame slot ids, object ids, absolute stack offsets, frame size,
  frame alignment, concrete byte offsets used for final memory operands,
  size/align layout, range overlap/range match offsets, stack home ids,
  register homes, storage encoding, destination/source register names,
  `can_use_base_plus_offset`, address-materialization kind, GOT/direct/page-low
  policy, TLS model/thread-pointer register/relocations, AArch64 memory
  operand legality, final instruction record errors, and scratch/register
  materialization policy.
- Current prepared same-block local-source queries use stack-layout range
  overlap as oracle behavior; BIR should preserve the semantic availability
  result for equivalence, not import stack-layout data as BIR-owned identity.

Candidate equivalence proof cases already present:
- Local/frame-slot load and store with address-space/volatile:
  `frame_slot_load_conversion_preserves_prepared_and_bir_facts`,
  `frame_slot_store_conversion_selects_structured_register_source`, and
  `volatile_memory_does_not_fabricate_atomic_authority` in
  `tests/backend/mir/backend_aarch64_prepared_memory_operand_records_test.cpp`.
- Global store and same-block global-load consumers:
  `global_symbol_store_conversion_preserves_prepared_and_bir_facts` plus
  `backend_codegen_route_aarch64_got_load_global_prepared_memory` for route
  coverage.
- Pointer-value load/store:
  `pointer_value_store_conversion_preserves_prepared_and_bir_facts`,
  `pointer_value_store_combines_selected_address_and_member_offsets`, and
  `pointer_value_load_combines_selected_address_and_member_offsets`.
- String constant:
  `string_constant_load_conversion_preserves_prepared_and_bir_facts`.
- Load-local source recovery and no-source/intervening-store negatives:
  `finds_unpublished_load_local_source_from_indexed_authority` in
  `tests/backend/mir/backend_store_source_publication_plan_test.cpp`.
- Memory-access lookup uniqueness/ambiguity:
  lookup helper coverage around
  `find_unique_indexed_prepared_memory_access_by_result_value_name` and
  `find_unique_indexed_prepared_memory_access_by_result_value_id` in
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`.
- F128/symbol-backed prepared memory-access contract:
  `check_f128_symbol_backed_load_local_addressing_contract` in
  `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`.
- Negative fail-closed cases:
  `unsupported_or_mismatched_memory_facts_fail_closed` covers missing prepared
  access, incomplete pointer base, result mismatch, symbol mismatch,
  stored-value mismatch, address-space/volatile mismatch, missing structured
  address facts, pointer home ambiguity/missing home, pointer mismatch, and
  string mismatch. Treat offset/size/align mismatch as prepared-oracle
  comparison evidence only, not BIR schema authority.

## Suggested Next

First code-changing packet: implement the smallest BIR memory-access identity
record/query surface for direct memory nodes only, with no consumer switch.
Suggested scope:
- Add target-neutral BIR memory identity records keyed by block label,
  instruction index, and BIR memory node kind for `LoadLocalInst`,
  `LoadGlobalInst`, `StoreLocalInst`, and `StoreGlobalInst`.
- Populate only result/stored value name, address space, volatile flag,
  semantic base kind, BIR local slot/name identity, global link/text identity,
  pointer value name, and string constant identity.
- Add test-only comparison against the prepared oracle for existing local,
  global, pointer, string, volatile, missing, and mismatch cases.
- Leave same-block load-local stored-value range/overlap recovery for a
  follow-up after the direct access identity surface compiles and proves.

Credible first proof command for that packet:
`cmake --build build --target backend_aarch64_prepared_memory_operand_records_test backend_prepared_lookup_helper_test && ctest --test-dir build --output-on-failure -R '^(backend_aarch64_prepared_memory_operand_records|backend_prepared_lookup_helper)$' > test_after.log 2>&1`

## Watchouts

- Keep `review/phase_a_steps_1_4_route_review.md` untouched.
- Keep prepared memory/access queries available as the oracle until BIR
  equivalence is proven.
- Use BIR slot/name identity for locals instead of prepared frame slot ids.
- Do not import frame layout, concrete offsets, TLS relocation spelling,
  GOT/direct/page-low policy, base-plus-offset legality, target
  addressing-mode choice, or AArch64 memory operand legality into BIR memory
  identity.
- Do not satisfy Step 2 by copying `PreparedAddress` or
  `PreparedMemoryAccess` wholesale; both are mixed semantic plus target/layout
  carriers.
- Do not switch `dispatch_value_materialization.cpp`,
  `fp_value_materialization.cpp`, `alu.cpp`, `calls.cpp`, `globals.cpp`, or
  memory-retargeting consumers before prepared/BIR equivalence exists.
- `find_prepared_same_block_load_local_stored_value_source` and
  `find_prepared_same_block_load_local_source_producer` currently depend on
  stack-layout range matching/overlap for correctness. A BIR query can expose
  the semantic source answer after equivalence, but should not own frame
  offsets, concrete sizes, or overlap math as BIR data.

## Proof

No build/test run for this evidence-only Step 1 packet, per delegation. No
`test_after.log` update was required or made.
