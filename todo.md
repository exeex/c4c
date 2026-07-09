Status: Active
Source Idea Path: ideas/open/634_large_selected_pointer_offset_local_memory_policy.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Trace Selected Pointer And Range Authority

# Current Packet

## Just Finished

Step 2 of idea `634` traced the prepared carrier boundary for large selected pointer-offset local-memory rows without changing implementation code.

Producer functions and carrier fields:

| Layer | Exact function | Carrier facts |
| --- | --- | --- |
| BIR pointer provenance | `pointer_value_memory_provenance` and `pointer_value_memory_provenance_with_layout_authority` in `src/backend/bir/lir_to_bir/memory/provenance.cpp` | Fills missing pointer base identity, requested byte range, dynamic-array data when present, layout authority, and range verdict. |
| BIR load producer | `BirFunctionLowerer::try_lower_addressed_pointer_load` in `src/backend/bir/lir_to_bir/memory/provenance.cpp` | Creates `bir::LoadLocalInst` with `MemoryAddress::BaseKind::PointerValue`, selected `base_value`, `byte_offset`, `size_bytes`, `align_bytes`, and provenance. |
| Prepared address producer | `build_pointer_indirect_address` in `src/backend/prealloc/stack_layout/coordinator.cpp` | Publishes `PreparedAddress::base_kind`, `pointer_value_name`, `byte_offset`, `size_bytes`, `align_bytes`, `can_use_base_plus_offset`, and `provenance`. |
| Prepared access producer | `build_pointer_indirect_access` for `LoadLocalInst` in `src/backend/prealloc/stack_layout/coordinator.cpp` | Publishes `PreparedMemoryAccess::function_name`, `block_label`, `inst_index`, `result_value_name`, `address_space`, `is_volatile`, and `address`. |
| RV64 consumer gate | `local_memory_diagnostic` in `src/backend/mir/riscv/codegen/object_emission.cpp` and `prepared_pointer_value_base_offset` in `src/backend/mir/riscv/codegen/prepared_local_memory_emit.cpp` | Requires default address space, nonvolatile pointer-value base, pointer value name, base-plus-offset, matching size/alignment, base GPR lookup, and currently a signed 12-bit offset. |

Target row evidence:

- `src/ipa-sra-2.c` `foo` block_1 inst 0: `%p.agg+3999996`, width 4, prepared row `base=pointer_value result=%t5 pointer=%p.agg offset=3999996 size=4 align=4 base_plus_offset=yes layout_authority=unknown range_verdict=unknown_compatible`; `%p.agg` has register home/storage `a1`.
- `src/pr60822.c` `Avg` entry inst 0 and 1: `%p.p+800000` and `%p.p+1700004`, width 4, prepared rows `base=pointer_value ... pointer=%p.p ... size=4 align=4 base_plus_offset=yes layout_authority=unknown range_verdict=unknown_compatible`; `%p.p` has register home/storage `a0`.
- The first unsupported authority boundary is the RV64 consumer predicate `fits_signed_12_bit_immediate(access->address.byte_offset)` inside `prepared_pointer_value_base_offset`; the prepared carrier already exposes selected base identity, offset, width, default access space, volatility, base GPR home, and range verdict for these rows.

Fail-closed states recorded:

- Missing selected base or absent pointer identity: `prepared_pointer_value_base_offset` rejects unless `base_kind == PointerValue` and `pointer_value_name` is present.
- Absent offset/width/address-space carrier: not observed for the target rows; `PreparedAddress::byte_offset`, `size_bytes`, `align_bytes`, and `PreparedMemoryAccess::address_space` are present.
- Stale or mismatched memory-use freshness: no selected freshness authority is attached to the load-local access row; only downstream store-source publication freshness is selected in `ipa-sra-2.c` for `%t5`/`%t10` store-local publication.
- Ambiguous pointer base: not observed for these rows; `gpr_register_number_for_value_name_local` can resolve the base value homes to `a1`/`a0`.
- Unsupported range: observed at the RV64 consumer helper because offsets `3999996`, `800000`, and `1700004` do not fit the signed 12-bit load/store immediate.
- Scratch/clobber authority: visible only in call-plan reserved scratch/clobber rows such as `gpr:t0`; no local-memory large-offset materialization scratch/clobber contract is published at the access row.

Evidence artifacts:

- `build/agent_state/634_step2_boundary_summary.md`
- `build/agent_state/634_step2_ipa-sra-2.prepared.txt`
- `build/agent_state/634_step2_ipa-sra-2.evidence.txt`
- `build/agent_state/634_step2_pr60822.prepared.txt`
- `build/agent_state/634_step2_pr60822.evidence.txt`
- `build/agent_state/634_step2_producer_provenance_excerpt.txt`
- `build/agent_state/634_step2_pointer_load_producer_excerpt.txt`
- `build/agent_state/634_step2_prepared_access_producer_excerpt.txt`
- `build/agent_state/634_step2_prepared_carrier_fields_excerpt.txt`
- `build/agent_state/634_step2_prepared_printer_fields_excerpt.txt`
- `build/agent_state/634_step2_object_emission_gate_excerpt.txt`
- `build/agent_state/634_step2_pointer_consumer_boundary_excerpt.txt`
- `build/agent_state/634_step2_materialization_helper_excerpt.txt`
- `build/agent_state/634_step2_base_register_lookup_excerpt.txt`
- `build/agent_state/634_step2_consumer_symbol_map.txt`

## Suggested Next

Step 3 should define the RV64 large-offset selected pointer materialization contract at the consumer boundary, starting from `prepared_pointer_value_base_offset` and `materialize_prepared_pointer_value_base_offset`. The smallest non-overfit packet is to require explicit default address space, nonvolatile access, pointer-value base, matching width/alignment, base GPR availability, large-offset range classification, and scratch/clobber safety, then fail closed with precise diagnostics when scratch/clobber authority is missing. Do not admit the target cases yet unless the scratch/clobber contract is explicit.

Step 4 should consume the Step 3 contract in `local_memory_diagnostic` and the load emission path, materializing only prepared selected pointer-value local-memory rows that satisfy the contract. Keep direct globals, frame slots, zero/narrow offsets, byval/sret stack pointer paths, filename predicates, exact-offset predicates, and expectation changes out of this admission route.

## Watchouts

The trace shows a consumer-range/materialization gap, not a missing prepared pointer carrier for the target rows. `prepared_pointer_value_memory_has_proven_authority` is stricter than these pointer rows because their layout authority remains `unknown` and range verdict is `unknown_compatible`; do not silently swap to that authority predicate unless Step 3 intentionally decides this idea must require proven object extent. The diagnostic remains generic and still does not distinguish missing selected pointer facts from large-offset scratch/clobber policy.

## Proof

Trace-only proof ran read-only dump/excerpt commands and wrote non-regression evidence under `build/agent_state/634_step2_*`:

- `build/c4cll --target riscv64-linux-gnu --dump-prepared-bir tests/c/external/gcc_torture/src/ipa-sra-2.c > build/agent_state/634_step2_ipa-sra-2.prepared.txt 2> build/agent_state/634_step2_ipa-sra-2.prepared.err`
- `build/c4cll --target riscv64-linux-gnu --dump-prepared-bir tests/c/external/gcc_torture/src/pr60822.c > build/agent_state/634_step2_pr60822.prepared.txt 2> build/agent_state/634_step2_pr60822.prepared.err`
- targeted `rg`, `nl`, and `sed` excerpt commands into the `build/agent_state/634_step2_*` artifacts listed above.

No code change was made, no root-level canonical logs were written, and `test_after.log` was not created for this trace-only packet.
