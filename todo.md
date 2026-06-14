Status: Active
Source Idea Path: ideas/open/254_phase_f3_prepared_compatibility_fail_closed_proof_matrix.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Prepared Compatibility Surfaces

# Current Packet

## Just Finished

Step 1 - Inventory Prepared Compatibility Surfaces completed as an
analysis-only inventory. Current prepared compatibility surfaces needing
matrix rows are below, split into common rows and fact-family-specific rows
with code or test evidence for each observable surface.

### Common Compatibility Rows

| Surface | Observable rows needing matrix coverage | Evidence |
| --- | --- | --- |
| Prepared aggregate lookup bundle | `PreparedFunctionLookups` remains the shared prepared reader bundle for addressing, value locations, calls, storage, edge publications, source producers, move bundles, value homes, and route/oracle consumers. | `src/backend/prealloc/prepared_lookups.cpp`; `src/backend/mir/x86/x86.hpp` `ConsumedPlans::prepared_lookups` and `shared_function_lookups()`. |
| Helper/oracle status spelling | Public status name helpers must keep exact names for prepared decoded home storage, formal publications, edge-copy/source facts, aggregate stack source authority, typed stack source publication, scalar publication, store-source publication, block-entry/current-block-entry publication, call-boundary move classification, and route statuses. | `src/backend/prealloc/decoded_home_storage.hpp`, `formal_publications.hpp`, `publication_plans.hpp`, `value_locations.hpp`, `calls.hpp`; `src/backend/bir/bir.hpp` `route4_publication_availability_status_name`, `route5_publication_status_name`, `route6_call_use_status_name`, `RouteIndexValidationStatus`. |
| Target intent statuses | x86 and RISC-V edge-publication wrapper intent statuses must keep `Available`, `MissingSharedLookups`, `MissingPublication`, `UnsupportedPublication`, `UnsupportedSourceHome`, and `UnsupportedDestinationHome`. | `src/backend/mir/x86/prepared/prepared.hpp`; `src/backend/mir/riscv/codegen/emit.hpp`; status assertions in `tests/backend/bir/backend_x86_prepared_decoded_home_storage_test.cpp` and `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`. |
| Prepared printer/debug text | Prepared dump and route-debug rows are compatibility outputs: `callsite ... wrapper=...`, `call block_index=... wrapper_kind=...`, `block_entry_publication ... status=...`, `route6 scalar arg status ... status=... gate=...`, and grouped authority summaries. | `src/backend/prealloc/prepared_printer/functions.cpp`; `src/backend/prealloc/prepared_printer/value_locations.cpp`; `src/backend/prealloc/prepared_printer/select_chains.cpp`; `src/backend/mir/x86/debug/debug.cpp`; expected strings in `tests/backend/bir/backend_prepared_printer_test.cpp` and `tests/backend/bir/backend_x86_route_debug_test.cpp`. |
| Prepared dump CLI surface | `--dump-prepared-bir` remains a public prepared dump flag and cannot be silently replaced by route/BIR output. | `src/apps/c4cll.cpp`; prepared dump expectations in `tests/backend/bir/backend_prepared_printer_test.cpp`. |
| Wrapper names and output | `PreparedCallWrapperKind` names `same_module`, `direct_extern_fixed_arity`, `direct_extern_variadic`, and `indirect` appear in prepared printer rows and target lowering. | `src/backend/prealloc/calls.hpp`; `src/backend/prealloc/prepared_printer/functions.cpp`; `tests/backend/bir/backend_prepared_printer_test.cpp` call wrapper rows. |
| Target-policy-sensitive output | Target output must preserve prepared fallback/target policy when route facts are absent, stale, mismatched, incomplete, or policy-insufficient; output rows include x86 call asm fallback, RISC-V `mv`/`lw`/`sw` edge moves, and AArch64 prepared memory/GOT/address materialization behavior. | `tests/backend/bir/backend_x86_handoff_boundary_direct_extern_call_test.cpp`; `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`; `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp` fallback/policy checks. |

### Fact-Family-Specific Rows

| Family | Surfaces needing rows | Evidence |
| --- | --- | --- |
| Calls | Prepared call-plan APIs and call wrappers: `find_prepared_call_plans`, `find_indexed_prepared_call_plan`, `find_prepared_call_argument_publication_source_routing`, `find_prepared_call_argument_source_producer_materialization`, `find_prepared_call_result_late_publication`; wrapper names; call argument/result plans; preserved/clobbered values; call-boundary classification statuses. Route/BIR agreement rows must cover Route 6 status names and x86 `ConsumedPlans` fallback when Route 6 is missing, nameless, invalid, duplicate, or mismatched. | `src/backend/prealloc/calls.hpp`; `src/backend/prealloc/call_plans.cpp`; `src/backend/mir/x86/x86.hpp`; `src/backend/mir/x86/debug/debug.cpp`; `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`; `tests/backend/bir/backend_x86_handoff_boundary_direct_extern_call_test.cpp`; `tests/backend/bir/backend_x86_route_debug_test.cpp`. |
| Memory | Prepared memory/access readers: `find_prepared_addressing`, `find_prepared_memory_access`, `find_prepared_global_load_access`, `find_prepared_same_block_global_load_access`; source-memory status `Unavailable`, `Available`, `MissingPreparedMemoryAccess`, `IncompletePreparedMemoryAccess`; target policy rows for global/GOT/TLS/frame/pointer addressing and address materialization. | `src/backend/prealloc/addressing.hpp`; `src/backend/prealloc/prepared_lookups.cpp`; `src/backend/mir/aarch64/codegen/calls.cpp`; `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`; `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`. |
| Edge publications | Prepared edge-publication lookup statuses `Available`, `MissingPredecessorLabel`, `MissingSuccessorLabel`, `MissingDestinationValue`, `MissingDestinationHome`; block-entry publication statuses; current-block entry statuses; x86/RISC-V move-intent statuses; Route 4/Route 5 status agreement/fallback rows, including duplicate, missing, wrong-key, wrong-owner, stale, `no_match`, `no_source`, and memory-source rows. | `src/backend/prealloc/publication_plans.hpp`; `src/backend/prealloc/value_locations.hpp`; `src/backend/prealloc/prepared_lookups.cpp`; `src/backend/bir/bir.hpp`; `tests/backend/bir/backend_prepared_printer_test.cpp`; `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`; `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`. |
| Source producers | Prepared source-producer identity rows for `LoadLocal`, `LoadGlobal`, `Cast`, `Binary`, `SelectMaterialization`, `Immediate`, and `Unknown`; route rows must prove Route 1/4/5/6 agreement without dropping prepared fallback. | `src/backend/prealloc/prepared_printer/select_chains.cpp`; `src/backend/prealloc/prepared_lookups.cpp`; `src/backend/mir/aarch64/codegen/calls.cpp`; `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`; `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`. |
| Names | Prepared interned-name surfaces for function, value, slot, and link names; matrix rows must reject raw-string fallback authority, missing names, stale symbol/link-name ids, and route/prepared name mismatches. | `src/backend/prealloc/names.hpp`; `src/backend/prealloc/control_flow.hpp`; `src/backend/mir/x86/prepared/prepared.hpp`; `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp` symbol/link-name mismatch; `tests/backend/bir/backend_x86_handoff_boundary_direct_extern_call_test.cpp` nameless Route 6 fallback. |
| Control flow | Prepared control-flow rows for block labels, branch conditions, join transfers, parallel-copy steps, current-block join sources, and Route 4/5 reference validation. Rows must preserve prepared printer/debug text when Route 4/5 evidence is missing, wrong successor/destination/instruction, duplicate, stale, or mismatched. | `src/backend/prealloc/control_flow.hpp`; `src/backend/prealloc/value_locations.hpp`; `src/backend/bir/bir.hpp`; `tests/backend/bir/backend_prepared_printer_test.cpp`; `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`. |
| Store-source publications | Prepared store-source publication rows for status `Available`, `MissingSourceValue`, `MissingDestinationAccess`, and intents; rows must preserve printer text and target fallback behavior for local/global store source publication. | `src/backend/prealloc/publication_plans.hpp`; `src/backend/prealloc/prepared_printer/select_chains.cpp`; `tests/backend/mir/backend_x86_store_source_publication_plan_reuse_test.cpp`; `tests/backend/mir/backend_aarch64_prepared_memory_operand_records_test.cpp`. |

## Suggested Next

Start Step 2 by drafting the common fail-closed matrix from the common rows
above: status/name preservation, fallback preservation, unsupported and missing
fail-closed behavior, route/BIR-only rejection, prepared-only preservation, and
policy-sensitive output stability.

## Watchouts

Do not implement adapters, refresh baselines, weaken expected strings, or
treat compatibility preservation as semantic ownership transfer. Several rows
are compatibility-oracle rows rather than permission to move semantic authority:
route agreement may annotate or gate, but non-agreeing route facts must keep
prepared fallback/output stable until a separate source idea authorizes an
ownership transfer.

## Proof

Analysis/documentation-only packet. No build, test, or `test_after.log` was
required by the delegated proof contract. Required proof command:
`git diff --check -- todo.md`.
