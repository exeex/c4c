Status: Active
Source Idea Path: ideas/open/272_phase_f5_riscv_memory_accesses_prepared_only_fail_closed_proof.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Preserve Adjacent Compatibility Surfaces

# Current Packet

## Just Finished

Completed Step 3 by verifying adjacent compatibility surfaces for the accepted
RISC-V prepared-only fail-closed row.

The delegated build and CTest proof is green. The exercised assertions still
cover the byte-stable positive dynamic stack-source output `lw a1, 12(s2)`;
the same helper/oracle names around `LoadLocal`, `memory_source`,
`load_local`, and `pointer_value`; normal
`PreparedFunctionLookups::memory_accesses` lookup behavior through
`find_unique_indexed_prepared_memory_access_by_result_value_id(...)`; and the
prepared lookup helper entry points `verify_prepared_memory_access_lookup()`,
`verify_bir_direct_memory_access_identity_lookup()`, and
`verify_route3_load_local_stored_value_source_matches_prepared_or_falls_back()`.

Fallback, status, debug, and adjacent expected text remain stable:

- agreeing Route 5 / Route 3 memory-source authority still returns
  `Available`, `MemorySource`, `route5_edge_source_agrees == true`,
  `route3_source_memory_agrees == true`, and emits `lw a1, 12(s2)`;
- mismatched or incomplete Route 3 memory identity still fails closed with
  `UnsupportedSourceHome`, empty instruction text, `MemorySource`,
  `route5_edge_source_agrees == false`, and
  `route3_source_memory_agrees == false`;
- scalar non-memory Route 5 non-agreement still preserves prepared fallback
  behavior with `mv a1, a0`;
- direct prepared emission with no Route 5 diagnostic input still emits
  `lw a1, 12(s2)`;
- adjacent missing-publication, missing-memory-row, incomplete-memory-row,
  materialization-required, non-i32 dynamic load, and aggregate-width
  fail-closed assertions stayed intact.

## Suggested Next

Execute Step 4 from `plan.md`: close the proof packet notes by summarizing the
prepared-only PO-family blocker reduced by this RISC-V same-consumer row, the
remaining `memory_accesses` blockers, and whether the supervisor wants broader
validation before lifecycle closure.

## Watchouts

- Narrow proof is sufficient for Step 3 compatibility review, but broader
  validation is recommended before Step 4 closure if the supervisor intends to
  close the source idea or treat this as a milestone slice. The touched
  behavior crosses backend RISC-V emission plus shared prepared lookup helper
  contracts, so a matching regression guard or broader CTest pass would give
  better closure confidence.
- The proof does not claim whole-API demotion readiness for
  `memory_accesses`, `PreparedFunctionLookups`, or `PreparedBirModule`; it only
  preserves compatibility around this one RISC-V same-consumer
  `LoadLocal` row.

## Proof

Command:

```bash
{ cmake --build build-x86 --target backend_riscv_prepared_edge_publication_test backend_prepared_lookup_helper_test && ctest --test-dir build-x86 -R '^(backend_riscv_prepared_edge_publication|backend_prepared_lookup_helper)$' --output-on-failure; } > test_after.log 2>&1
```

Result: passed. `test_after.log` contains `ninja: no work to do`, then CTest
reports 2/2 tests passed:
`backend_prepared_lookup_helper` and
`backend_riscv_prepared_edge_publication`.

The supervisor-selected proof was sufficient for Step 3 compatibility review.
Broader validation is recommended before Step 4 closure notes if this packet is
used as source-idea closure evidence.
