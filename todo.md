Status: Active
Source Idea Path: ideas/open/141_prepared_lookups_residual_owner_audit.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Map consumers and include pressure

# Current Packet

## Just Finished

Step 3 consumer and include-pressure map completed for the residual
`prepared_lookups.hpp` APIs. Search covered the AArch64 files named in
`plan.md`, extra AArch64/prealloc/backend consumers discovered by `rg`, and
the source-idea revisit points for current-block join-copy facts/routing plus
stack-source publication helper ownership.

| Consumer | Residual API groups used | Include/API pressure |
| --- | --- | --- |
| `src/backend/mir/aarch64/codegen/dispatch.cpp` | No direct `prepared_lookups.hpp` include and no direct residual calls found. It routes through local AArch64 dispatch headers. | No direct pressure from this file. Keep it out of any follow-up unless transitive AArch64 headers change. |
| `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp` | Calls `find_indexed_prepared_edge_publication_source_producer` at `dispatch_edge_copies.cpp:214` while including `publication_plans.hpp`, not `prepared_lookups.hpp`. Also consumes `context.function.prepared_lookups->edge_publications` later in the file. | Strong evidence that source-producer lookup declarations want a publication/select-chain owner header. This consumer already points at the narrower publication header and relies on transitive exposure for the residual declaration. |
| `src/backend/mir/aarch64/codegen/dispatch_producers.cpp` | Includes `prepared_lookups.hpp`. Uses value-home indexes (`find_indexed_prepared_value_id`, local `PreparedValueHomeLookups`, `make_prepared_value_home_lookups`), same-block scalar producer queries, source-producer lookup helpers, and both current-block join APIs: `prepare_current_block_join_parallel_copy_source_facts` and `prepare_current_block_join_parallel_copy_instruction_routing`. | Mixed pressure. Value-home indexes point toward `value_locations.hpp`; source-producer lookups point toward `select_chain_lookups.hpp`/`publication_plans.hpp`; join-copy facts point toward publication/control-flow. The instruction-routing call is AArch64-local routing convenience layered on reusable facts and is the clearest split-fact-from-routing consumer. |
| `src/backend/mir/aarch64/codegen/dispatch_publication.cpp` | Includes `prepared_lookups.hpp`. Uses `find_prepared_current_block_entry_publication`, `PreparedCurrentBlockEntryPublicationQueryInputs`, and `PreparedCurrentBlockEntryPublicationStatus` around `dispatch_publication.cpp:222`. | Current-block entry publication is publication/value-home semantics. Existing owner pressure points to `value_locations.hpp` for block-entry publication collection/status and `publication_plans.hpp` for publication facts; the broad facade is heavier than this consumer needs. |
| `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp` | Includes `prepared_lookups.hpp`. Builds `PreparedSameBlockValueMaterializationQuery`, calls `find_prepared_same_block_scalar_producer` and `evaluate_prepared_same_block_integer_constant`, and also includes `select_chain_lookups.hpp`. | Same-block materialization query/fact APIs are the narrow pressure point. This file already separately includes select-chain ownership, so residual same-block scalar/integer materialization declarations could move to a narrower same-block/source-producer owner without changing target emission policy. |
| `src/backend/mir/aarch64/codegen/comparison.cpp` | Includes `prepared_lookups.hpp`. Uses fused-compare operand facts, fused-compare operand producer queries, materialized-condition producer queries, and `find_frame_slot_by_id`. | Comparison facts are reusable prepared semantics consumed by AArch64 comparison lowering. Candidate owner pressure is a comparison/materialized-condition helper or control-flow comparison area; stack slot id lookup is separate pressure toward `stack_layout/stack_layout.hpp`. AArch64-specific compare instruction selection remains local. |
| `src/backend/mir/aarch64/codegen/alu.cpp` | Includes `prepared_lookups.hpp`. Uses before-return ABI move lookup, return-chain terminal/next operand lookups, indexed value homes, same-block scalar producer queries, and fallback `make_prepared_function_lookups` construction. | Broadest named AArch64 pressure. Return-chain lookups currently have no obvious owner except the prepared aggregate/control-flow neighborhood. Same-block scalar producer pressure is separate from return-chain pressure. Fallback aggregate construction supports keeping `PreparedFunctionLookups` as a central aggregate even if query declarations move. |
| `src/backend/mir/aarch64/codegen/calls.cpp` | Does not include `prepared_lookups.hpp` directly, but reaches residual declarations through `module.hpp`/other transitive includes. Uses indexed value homes, indexed move bundle and before-call move lookups, frame-slot id lookup, call-argument source-producer materialization, current-block publication consumption, source-producer lookup helper, and same-block load-local stored-value source. | High API pressure but not direct include pressure. Call-specific pieces point to `calls.hpp` plus value-location indexes; same-block publication/indirect-callee facts point to publication/source-producer ownership; same-block load-local stored-value crosses source-producer, addressing, and stack-layout owners. |
| `src/backend/mir/aarch64/codegen/memory.cpp` | Includes `prepared_lookups.hpp`. Uses before-return ABI move lookup, indexed value id/home helpers, local `PreparedValueHomeLookups`, source-producer lookup helper, prepared aggregate fields, and typed stack-source publication emission from `publication_plans.hpp`. | Value-home index and move-bundle pressure dominate. Typed stack-source publication is already publication-plan API; AArch64 owns final load/store emission, register views, scratch use, and extension policy interpretation. |
| Extra AArch64: `select_materialization.cpp` | Includes `prepared_lookups.hpp`; search only found use of `context.function.value_home_lookups` from the function context. | Likely include-cleanup pressure rather than residual API ownership pressure; value-home type can come from narrower declarations if context/header layering permits. |
| Extra AArch64: `memory_store_retargeting.cpp` | Includes `prepared_lookups.hpp`. Uses `find_stack_object_by_id`, `find_frame_slot_by_id`, `PreparedValueHomeLookups`, and addressing/value-home frame-address helpers. | Stack id helpers should be considered for `stack_layout/stack_layout.hpp`; value-home type pressure points to `value_locations.hpp`; addressing helper calls already have an addressing owner. |
| Extra AArch64: `frame_slot_address.cpp` | Includes `prepared_lookups.hpp`. Thin wrappers call `find_frame_slot_by_id` and `find_stack_object_by_id`. | Clean stack-layout include/API pressure. These are not prepared aggregate queries and would fit `stack_layout/stack_layout.hpp`. |
| Extra AArch64: `fp_value_materialization.cpp` | Includes `prepared_lookups.hpp`. Uses same-block scalar producer query with generated source-producer lookups fallback. | Same-block materialization/source-producer owner pressure, analogous to `dispatch_value_materialization.cpp`. |
| Extra AArch64: `dispatch_lookup.cpp` | Includes `prepared_lookups.hpp`, but observed use is `find_prepared_value_home_for_bir_value` from value-location helpers rather than a residual prepared-lookup declaration. | Include-cleanup pressure toward `value_locations.hpp`; not a residual owner blocker. |
| Extra AArch64 module: `module.hpp` / `module.cpp` | Header includes `prepared_lookups.hpp` to expose `PreparedFunctionLookups`, `PreparedMoveBundleLookups`, and `PreparedValueHomeLookups` pointers in `FunctionLoweringContext`; `module.cpp` calls `find_indexed_prepared_after_call_result_lane_binding`. | Central aggregate and index type pressure. If value-home/move-bundle types move to narrower owner headers, this context can include those. The `PreparedFunctionLookups` pointer still justifies a central aggregate declaration somewhere. |
| Extra AArch64 traversal: `traversal.cpp` | Calls `make_prepared_function_lookups`, then stores pointers to aggregate fields: call-plan, address-materialization, move-bundle, and value-home lookups. | Strong keep-core-aggregate evidence for `PreparedFunctionLookups` wiring. Consumers want one preparation pass and field projections, not independent domain rescans. |
| Prealloc: `formal_publications.cpp` / `.hpp` | Includes `prepared_lookups.hpp` in the `.cpp`; uses `PreparedValueHomeLookups`, `find_indexed_prepared_value_id`, and `find_indexed_prepared_value_home`. | Narrow value-home/index pressure toward `value_locations.hpp`; this is not a broad prepared facade consumer. |
| Prealloc: `decoded_home_storage.hpp` / `.cpp` | Header includes `prepared_lookups.hpp` only to name `PreparedValueHomeLookups`; `.cpp` calls `find_indexed_prepared_value_home`. | Strong include-cleanup pressure: declarations fit the value-location owner and would avoid a broad header in a reusable prealloc helper header. |
| Prealloc: `select_chain_lookups.cpp` and `prepared_printer/select_chains.cpp` | `select_chain_lookups.cpp` includes `prepared_lookups.hpp` and defines `find_indexed_prepared_edge_publication_source_producer`; the printer includes the facade and calls that helper. | Existing implementation location already says source-producer lookup ownership is not really `prepared_lookups.cpp`. Declarations should likely live with `select_chain_lookups.hpp` or publication/source-producer declarations rather than requiring the facade. |
| Prealloc: `publication_plans.cpp` | Calls `find_indexed_prepared_edge_publication_source_producer` while owning publication-plan structs and same-width/aggregate stack-source helpers. | Publication/source-producer owner pressure. This file is a natural declaration home for publication facts, while select-chain remains a candidate for direct-global/select-chain lookup helpers. |
| Prealloc: `call_plans.cpp` | Includes `prepared_lookups.hpp`, but search found no residual Step-3 target calls; uses module/value-location/addressing/control-flow helpers and `find_prepared_move_bundle` from value locations. | Include-cleanup pressure only. It should not drive residual API moves unless a direct dependency appears. |
| Prealloc: `module.hpp` | Includes `prepared_lookups.hpp` as part of the broad `PreparedBirModule` aggregate header. | Coarse include pressure from central module aggregation. Keep separate from query ownership: this may remain a broad module convenience even if residual query declarations move. |
| Other backend: `src/backend/mir/riscv/codegen/emit.*` and `src/backend/mir/x86/x86.hpp` | RISC-V and x86 consume `PreparedFunctionLookups` and `make_prepared_function_lookups`; RISC-V also reads edge publication lookups from the aggregate. | Cross-target evidence for keeping a central function-level aggregate and builder. Follow-up moves must not assume AArch64 is the only consumer of the aggregate. |

Current-block join parallel-copy revisit:

- `dispatch_producers.cpp` consumes both reusable source facts and
  instruction-routing booleans. The fact query publishes predecessor/successor,
  move/publication/home, incoming-expression, source-value, stack/immediate, and
  register-sharing facts that are reusable prepared information. Those facts
  are not just AArch64 emission policy.
- `PreparedCurrentBlockJoinParallelCopyInstructionRouting` is different: it
  converts value-id sets into per-instruction result boolean vectors for the
  current AArch64 block walk. That is target-facing routing convenience. A
  follow-up should split or relocate it separately from reusable join-copy
  source facts.
- Owner pressure for reusable facts points toward `publication_plans.hpp`
  because they build on `PreparedEdgeCopySourceFactsStatus`,
  `PreparedEdgePublication`, and edge-copy source facts. Parallel-copy bundle
  discovery also depends on `control_flow.hpp`, so a follow-up must name the
  boundary between publication facts and control-flow bundle traversal instead
  of hiding both behind another facade.

Same-width and aggregate stack-source publication helper revisit:

- No same-width typed stack-source or aggregate stack-source helper remains as
  a residual public declaration in `prepared_lookups.hpp`.
- The aggregate authority helper is declared in `publication_plans.hpp:458` and
  defined in `publication_plans.cpp:516`; the typed same-width stack-source
  helper is declared in `publication_plans.hpp:462` and defined in
  `publication_plans.cpp:598`.
- `prepared_lookups.cpp` only consumes `prepare_aggregate_stack_source_authority`
  while building edge publications. AArch64 `memory.cpp` consumes
  `PreparedTypedStackSourcePublication` for emission, and AArch64 owns final
  register view, scratch, load/store line emission, and extension handling.
  Publication-plan ownership is already clear; no residual prepared-lookup
  owner move is needed for these helpers.

## Suggested Next

Execute Step 4 from `plan.md`: classify each residual group using the Step 1
inventory, Step 2 definition map, and this Step 3 consumer map. Keep
`PreparedFunctionLookups`/`make_prepared_function_lookups` separate from the
domain query declarations when deciding ownership.

## Watchouts

- Do not classify every broad include as proof of a move. Some files include
  `prepared_lookups.hpp` only through `module.hpp` or only to name aggregate
  field types.
- `find_indexed_prepared_edge_publication_source_producer` has its declaration
  in `prepared_lookups.hpp` but its definition in `select_chain_lookups.cpp`;
  classify that as concrete owner/API pressure, not as missing implementation.
- Return-chain consumers are concentrated in AArch64 `alu.cpp`, but no narrow
  owner was obvious beyond control-flow/return semantics. Avoid inventing a
  vague follow-up if Step 4 cannot name a concrete owner file.
- Stack-source publication helpers already have a publication-plan home. Do
  not reopen idea 140 for those helpers unless Step 4 finds a residual facade
  declaration, which this packet did not.

## Proof

Inspection-only analysis slice. Ran targeted `rg` and file reads over the
AArch64 files named in `plan.md`, additional AArch64/prealloc/backend consumers
found by search, `prepared_lookups.hpp`, and the relevant prealloc owner files.
No build or test command was run, and no `test_after.log` was required by the
delegated packet.
