Status: Active
Source Idea Path: ideas/open/258_phase_f3_x86_route3_loadlocal_source_memory_agreement_bridge.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Route 3 and x86 Memory Consumer Inventory

# Current Packet

## Just Finished

Step 1, Route 3 and x86 Memory Consumer Inventory, is complete.

Route 3 `LoadLocal` memory/source identity is represented by
`bir::Route3MemoryAccessRecord`, built through
`route3_memory_access_record(...)`, indexed by
`route3_build_memory_access_index(...)`, and looked up with
`route3_find_memory_access_record(...)`,
`route3_find_same_block_load_local_source(...)`, and
`mir::find_bir_same_block_load_local_source_identity(...)`. The selected
fields for the bridge are `instruction`, `instruction_index`, `node_kind =
LoadLocal`, `block_label` / `block_label_id`, `result_value`, `address_space`,
`is_volatile`, `base_kind`, local-slot identity (`local_slot_name` /
`local_slot_id`), pointer/global/string identity when applicable,
`byte_offset`, `size_bytes`, and `align_bytes`. The MIR facade exposes the same
surface as `mir::BirMemoryAccessIdentity` with `result_value_name`,
`base_kind`, local/global/pointer/string identities, volatility, address space,
offset, size, and alignment.

Prepared memory identity is carried by `prepare::PreparedMemoryAccess`:
`function_name`, `block_label`, `inst_index`, `result_value_name`,
`stored_value_name`, `address_space`, `is_volatile`, and `address`
(`base_kind`, frame slot, symbol, pointer value, byte offset, size, alignment,
and base-plus-offset support). `apply_source_memory_access_fact(...)` copies a
`LoadLocal` producer's prepared memory access into
`PreparedEdgePublication::source_memory_access` and the public
`source_memory_*` snapshot fields, with status rows
`Unavailable`, `MissingPreparedMemoryAccess`,
`IncompletePreparedMemoryAccess`, and `Available`. Stable public surfaces are
`PreparedFunctionLookups::memory_accesses`,
`find_prepared_memory_access(...)`,
`find_indexed_prepared_memory_access(...)`, unique result-name/id lookup
helpers, `PreparedEdgePublication::source_memory_access_status`,
`PreparedEdgePublication::source_memory_access`,
`PreparedEdgeCopySourceFacts`, prepared printer output, and route-debug status
strings.

Current x86 candidate boundary: no existing x86 reader/facade joins Route 3
and prepared source-memory identity. The narrow candidate for Step 2 is a new
x86-local facade around the local-slot memory consumer path used by
`render_prepared_local_slot_statement_memory_operand(...)` /
`find_prepared_local_slot_compare_load(...)` in
`src/backend/mir/x86/module/module.cpp`. That path already verifies prepared
`result_value_name` / `stored_value_name` before rendering a prepared frame-slot
operand, but it does not call `mir::find_bir_memory_access_identity(...)` or
`mir::find_bir_same_block_load_local_source_identity(...)`, so prepared
`memory_accesses` remains the only authority there today.

Existing proof surfaces:
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp` covers Route 3
`LoadLocal` memory identity, BIR/MIR query wrappers, prepared
`source_memory_access` status and match helpers, and stored-value source
fallback rows. `backend_prepared_printer` covers public prepared printing.
`backend_x86_handoff_boundary` covers x86 local-slot guard and LoadLocal-heavy
handoff behavior; `backend_x86_route_debug` covers x86 route diagnostics.
Those x86 tests are registered only when `C4C_ENABLE_X86_BACKEND_TESTS` is on,
so focused x86 proof needs an x86-enabled build.

## Suggested Next

Execute Step 2: define the smallest agreement facade around the x86 local-slot
prepared memory consumer path and write the fail-closed matrix for missing,
incomplete, duplicate/conflict, prepared-only, route-only, unsupported, and
mismatch cases.

## Watchouts

- Agreement must compare a Route 3 record with the prepared source-memory row;
  prepared `memory_accesses` alone is not semantic authority.
- Preserve prepared lookup/status observability, helper/oracle names, fallback
  names, x86 output formatting, and target-owned addressing/register policy.
- Use x86-enabled validation if the focused x86 tests are not registered in the
  default build.
- The analogous AArch64/RISC-V paths are useful shape references, but this
  packet did not implement or prove an x86 agreement gate.

## Proof

Ran `git diff --check -- todo.md`: pass. This delegated analysis-only proof
does not create `test_after.log`.
