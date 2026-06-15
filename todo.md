Status: Active
Source Idea Path: ideas/open/273_phase_f5_riscv_memory_accesses_byte_offset_drift_fail_closed_proof.md
Source Plan Path: plan.md
Current Step ID: 2-3
Current Step Title: Add or Expose the Byte-Offset Drift Row; Preserve Adjacent Compatibility Surfaces

# Current Packet

## Just Finished

Step 2 and Step 3: Verified that the selected RISC-V byte-offset drift row is
already exposed through the supported fixture/oracle path, so no duplicate
test assertion or implementation edit was needed.

- Step 2 is satisfied by the existing
  `check_route5_route3_oracle_rows_preserve_prepared_riscv_fallback()` row:
  `make_load_local_dynamic_stack_source(dynamic_prepared, dynamic_ids,
  load_local_source_access(dynamic_ids))` uses normal prepared lookup
  construction, the public `memory_accesses` row remains available, the agreeing
  Route 3/Route 5 memory-source row proves `byte_offset=12`, and the selected
  drift row changes only
  `mismatched_route3_memory_edge.source_memory_access.byte_offset = 16`.
- The drift row is exposed to the RISC-V supported consumer through
  `consume_edge_publication_move_intent(&lookups, ..., &mismatched_route3_memory_edge)`
  and expects the fail-closed result:
  `UnsupportedSourceHome`, empty instruction text, Route 5 status
  `MemorySource`, `route5_edge_source_agrees=false`, and
  `route3_source_memory_agrees=false`.
- Step 3 is preserved by the adjacent assertions in the same fixture: the
  agreeing path still emits `lw a1, 12(s2)`, the no-Route-3-diagnostics
  fallback still remains `Available` with `lw a1, 12(s2)`, status/name surfaces
  such as `"memory_source"`, `"load_local"`, and `"pointer_value"` remain
  covered, and
  `check_load_local_dynamic_stack_source_exposes_shared_memory_access()` still
  proves normal `memory_accesses` public lookup construction plus missing and
  incomplete prepared-memory-access compatibility.

## Suggested Next

Supervisor/plan-owner should decide whether this runbook is ready for lifecycle
review or closure; the delegated Step 2/Step 3 executor slice is complete.

## Watchouts

- No code/test change was needed for this packet; accepting a duplicate row
  would add noise rather than prove new surface area.
- Keep the prepared memory row at byte offset `12`; the covered drift remains
  Route 3 diagnostic byte offset `16` versus prepared/public byte offset `12`.
- Do not weaken the public `memory_accesses` lookup requirement, the positive
  `lw a1, 12(s2)` output, or the missing/incomplete source-memory fail-closed
  cases.

## Proof

Ran the exact delegated proof command and preserved combined output in
`test_after.log`:

```sh
cmake --build build-x86 --target backend_riscv_prepared_edge_publication_test backend_prepared_lookup_helper_test && ctest --test-dir build-x86 -R '^(backend_riscv_prepared_edge_publication|backend_prepared_lookup_helper)$' --output-on-failure
```

Result: passed. Ninja reported no work to do, then CTest passed
`backend_prepared_lookup_helper` and
`backend_riscv_prepared_edge_publication` (`2/2` tests passed).
