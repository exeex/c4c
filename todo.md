Status: Active
Source Idea Path: ideas/open/139_addressing_lookup_ownership_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect current addressing lookup ownership and dependencies

# Current Packet

## Just Finished

Step 1 inspection recorded the addressing/frame lookup ownership boundary and
dependency constraints for `ideas/open/139_addressing_lookup_ownership_cleanup.md`.

Current declaration locations:

- `src/backend/prealloc/addressing.hpp`: owns direct addressing facts and
  direct scans:
  `PreparedAddress`, `PreparedAddressMaterialization`,
  `PreparedMemoryAccess`, `PreparedAddressingFunction`, `PreparedAddressing`;
  `find_prepared_addressing_function`;
  `find_prepared_memory_access`;
  `find_prepared_memory_access_by_result_name`;
  `find_prepared_memory_access_by_result_value_name`;
  `find_prepared_memory_access_before_by_result_value_name`;
  `find_prepared_address_materialization`.
- `src/backend/prealloc/prepared_lookups.hpp:21`: declares
  `PreparedAddressMaterializationLookups`.
- `src/backend/prealloc/prepared_lookups.hpp:26`: declares
  `PreparedMemoryAccessLookups`.
- `src/backend/prealloc/prepared_lookups.hpp:42`: declares
  `PreparedFrameAddressOffset`.
- `src/backend/prealloc/prepared_lookups.hpp:692`: `PreparedFunctionLookups`
  still contains cached aggregate members
  `.address_materializations` and `.memory_accesses`.
- `src/backend/prealloc/prepared_lookups.hpp:717`: declares
  `prepared_memory_access_position_key`.
- `src/backend/prealloc/prepared_lookups.hpp:726`: declares
  `make_prepared_address_materialization_lookups`.
- `src/backend/prealloc/prepared_lookups.hpp:730`: declares
  `make_prepared_memory_access_lookups`.
- `src/backend/prealloc/prepared_lookups.hpp:809`: declares
  `find_indexed_prepared_address_materializations`.
- `src/backend/prealloc/prepared_lookups.hpp:814`: declares
  indexed memory-access queries by result value name, position, unique result
  value name, result value id, and unique result value id.
- `src/backend/prealloc/prepared_lookups.hpp:839`: declares
  `collect_prepared_address_materializations_for_block`.
- `src/backend/prealloc/prepared_lookups.hpp:844`: declares
  `find_indexed_prepared_frame_address_offset_for_value`.
- `src/backend/prealloc/prepared_lookups.hpp:852`: declares
  `find_indexed_prepared_frame_address_offset_for_value_id`.
- `src/backend/prealloc/prepared_lookups.hpp:1003`: declares
  `find_prepared_global_load_access`.
- `src/backend/prealloc/prepared_lookups.hpp:1011`: declares
  `find_prepared_same_block_global_load_access`.
- `src/backend/prealloc/publication_plans.hpp:337`: declares
  `find_prepared_same_block_load_local_source_producer`, which consumes
  `PreparedMemoryAccessLookups`.

Current definition and constructor locations:

- `src/backend/prealloc/stack_layout/coordinator.cpp`: constructs
  `PreparedAddressMaterialization` records while preparing addressing facts:
  frame-slot materializations at lines 1098 and 1181; global materializations
  at line 1255; string-constant materializations at lines 1309 and 1386; label
  materializations at line 1449; `append_address_materializations` starts at
  line 1462 and is called from the per-function addressing population path at
  line 1640.
- `src/backend/prealloc/prepared_lookups.cpp:1317`: defines
  `make_prepared_address_materialization_lookups`; it builds the per-block
  index from `PreparedAddressingFunction::address_materializations`.
- `src/backend/prealloc/prepared_lookups.cpp:1332`: defines
  `make_prepared_memory_access_lookups`; it builds position, result-name, and
  result-id indexes from `PreparedAddressingFunction::accesses` and optionally
  `PreparedValueHomeLookups`.
- `src/backend/prealloc/prepared_lookups.cpp:1770`: defines
  `make_prepared_function_lookups`; AST callers show it is the only caller of
  both lookup builders in this translation unit, and it wires
  `.address_materializations` plus `.memory_accesses` into the aggregate.
- `src/backend/prealloc/prepared_lookups.cpp:2103`: defines
  `find_indexed_prepared_address_materializations`.
- `src/backend/prealloc/prepared_lookups.cpp:2117` through line 2183: define
  the indexed memory-access query helpers.
- `src/backend/prealloc/prepared_lookups.cpp:2186`: defines
  `collect_prepared_address_materializations_for_block`.
- `src/backend/prealloc/prepared_lookups.cpp:2199` and line 2262: define the
  frame-address-offset queries. AST callees are
  `find_indexed_prepared_address_materializations`, `find_frame_slot_by_id`,
  and the local `prepared_frame_address_object_is_addressable`.
- `src/backend/prealloc/prepared_lookups.cpp:2893` and line 2922: define
  global-load access queries. AST callees for the primary helper are
  `find_prepared_memory_access` from `addressing.hpp` and local
  `prepared_global_load_access_matches_result`.
- `src/backend/prealloc/publication_plans.cpp:943`: defines
  `find_prepared_same_block_load_local_source_producer`; AST callees include
  `find_indexed_prepared_memory_access`,
  `find_unique_indexed_prepared_memory_access_by_result_value_name`,
  stack-layout overlap/range helpers, and select-chain producer lookup.
- `src/backend/prealloc/prepared_lookups.cpp:1085` and line 1096: define
  `find_frame_slot_by_id` and `find_stack_object_by_id`, currently broad
  prepared-lookup declarations but frame-domain implementations over
  `PreparedStackLayout`.

Helper classification:

- Addressing-domain declarations for Step 2: `PreparedAddressMaterializationLookups`,
  `PreparedMemoryAccessLookups`, `prepared_memory_access_position_key`,
  `make_prepared_address_materialization_lookups`,
  `make_prepared_memory_access_lookups`,
  `find_indexed_prepared_address_materializations`, all indexed
  `PreparedMemoryAccessLookups` queries,
  `collect_prepared_address_materializations_for_block`,
  `find_prepared_global_load_access`, and
  `find_prepared_same_block_global_load_access`.
- Frame-domain declarations for Step 2: `PreparedFrameAddressOffset`,
  `find_indexed_prepared_frame_address_offset_for_value`, and
  `find_indexed_prepared_frame_address_offset_for_value_id`, because they
  combine address-materialization lookups with `PreparedStackLayout` and frame
  slot/object addressability.
- Broad aggregate declarations that should stay in `prepared_lookups.hpp` for
  Step 2: `PreparedFunctionLookups`, `make_prepared_function_lookups`, and the
  cached aggregate wiring for `.address_materializations` and `.memory_accesses`.

Known consumer and include constraints:

- `src/backend/mir/aarch64/module/module.hpp:6` includes
  `prepared_lookups.hpp` and exposes `PreparedFunctionLookups` plus
  `PreparedAddressMaterializationLookups*` in `FunctionLoweringContext`.
  Step 2 can rely on `prepared_lookups.hpp` including the new domain header
  rather than changing module context ownership.
- `src/backend/mir/aarch64/codegen/traversal.cpp:76` constructs
  `PreparedFunctionLookups` and stores
  `&prepared_lookups.address_materializations` in the function context.
- AArch64 direct address lookup consumers:
  `dispatch_value_materialization.cpp:289`,
  `fp_value_materialization.cpp:357`, and `globals.cpp:174` call
  `find_prepared_same_block_global_load_access` or
  `find_prepared_global_load_access`.
- AArch64 frame-offset consumers:
  `globals.cpp:1061`, `memory.cpp:4716`, and
  `memory_store_retargeting.cpp:75` call the indexed frame-address-offset
  helpers. `memory_store_retargeting.hpp:10` currently forward-declares
  `PreparedAddressMaterializationLookups`.
- AArch64 address-materialization consumers:
  `globals.cpp:1225` uses indexed materializations from the cached context,
  and `globals.cpp:1245` falls back to
  `collect_prepared_address_materializations_for_block`.
- AArch64 memory-access consumers:
  `alu.cpp:1034` and `alu.cpp:1101` consume
  `PreparedFunctionLookups::memory_accesses`; the latter passes it to
  `find_prepared_same_block_load_local_source_producer`.
- `publication_plans.hpp` includes both `addressing.hpp` and
  `prepared_lookups.hpp`; after Step 2 its declaration should get
  `PreparedMemoryAccessLookups` from `addressing.hpp`, while keeping any broad
  prepared-lookup dependency only where still required.

## Suggested Next

Execute Step 2 from `plan.md`: move the public declaration boundary for
addressing/frame lookup facts from `src/backend/prealloc/prepared_lookups.hpp`
into domain headers without moving definitions yet.

Exact Step 2 move:

- Move `PreparedAddressMaterializationLookups`, `PreparedMemoryAccessLookups`,
  `prepared_memory_access_position_key`,
  `make_prepared_address_materialization_lookups`,
  `make_prepared_memory_access_lookups`,
  `find_indexed_prepared_address_materializations`, the indexed
  memory-access query declarations,
  `collect_prepared_address_materializations_for_block`,
  `find_prepared_global_load_access`, and
  `find_prepared_same_block_global_load_access` into
  `src/backend/prealloc/addressing.hpp`.
- Move `PreparedFrameAddressOffset`,
  `find_indexed_prepared_frame_address_offset_for_value`, and
  `find_indexed_prepared_frame_address_offset_for_value_id` into
  `src/backend/prealloc/stack_layout/stack_layout.hpp` or another narrow
  frame-domain header if the supervisor chooses one; because
  `addressing.hpp` already includes `frame.hpp`, moving the result struct into
  `addressing.hpp` is also build-plausible, but the ownership classification
  is frame-domain.
- Keep definitions in `prepared_lookups.cpp` for Step 2. Keep
  `PreparedFunctionLookups` and `make_prepared_function_lookups` declarations
  in `prepared_lookups.hpp`, and let that header include the domain owner
  header(s) so aggregate cached access remains intact.

## Watchouts

- Do not edit implementation files during Step 1.
- Do not change address lowering, frame layout, TLS/global relocation behavior,
  or store-source publication semantics.
- Do not replace prepared addressing facts with local scans or name matching.
- Keep `PreparedFunctionLookups` cached aggregate access intact.
- `find_prepared_global_load_access` is addressing-domain but its definition
  currently uses local result-matching logic in `prepared_lookups.cpp`; moving
  only the declaration avoids a broader helper split.
- `find_indexed_prepared_frame_address_offset_for_value_id` depends on
  `PreparedValueHomeLookups`; Step 2 will need whichever header owns that type
  visible from the chosen frame-domain declaration site.
- `publication_plans.*` consumes memory-access lookup declarations but should
  not be redesigned in this cleanup slice.

## Proof

Inspection-only packet. Used `c4c-clang-tools` for symbol signatures,
definition lookup, caller/callee checks, and `rg`/line-number scans for include
and consumer constraints. No build was required and no test logs were created.
