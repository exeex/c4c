Status: Active
Source Idea Path: ideas/open/272_phase_f5_riscv_memory_accesses_prepared_only_fail_closed_proof.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add or Expose the Prepared-Only Fail-Closed Row

# Current Packet

## Just Finished

Completed Step 2 by adding the same-consumer RISC-V prepared-only fail-closed
behavior for the dynamic stack-source `LoadLocal` memory row.

The backend consumer now rejects supplied non-agreeing Route 5 / Route 3
memory-source authority for prepared `LoadLocal` publications before rendering
the source operand. The prepared `memory_accesses` row remains normal and
internally coherent; the new fail-closed condition keys off the semantic
authority disagreement recorded by
`route5_edge_source_agrees_with_prepared_publication(...)`, not a corrupted
prepared lookup.

The focused RISC-V test row now asserts:

- positive Route 5 / Route 3 agreement still returns `Available` and emits
  byte-stable `lw a1, 12(s2)`;
- a coherent prepared row plus mismatched Route 3 memory byte offset fails
  closed with `UnsupportedSourceHome`, empty instruction text,
  `route5_edge_status == MemorySource`,
  `route5_edge_source_agrees == false`, and
  `route3_source_memory_agrees == false`;
- a coherent prepared row plus absent Route 3 memory identity on a Route 5
  memory-source row also fails closed with `UnsupportedSourceHome` and empty
  instruction text;
- the existing no-Route-5 diagnostic path still keeps the direct prepared
  emission compatibility output `lw a1, 12(s2)`.

## Suggested Next

Execute Step 3 from `plan.md`: preserve adjacent compatibility surfaces by
reviewing the targeted proof output, checking that helper/oracle behavior and
status/debug strings stayed stable, and deciding whether the supervisor wants
broader validation before Step 4 closure notes.

## Watchouts

- The fail-closed rule is intentionally limited to callers that supply Route 5
  authority for a prepared `LoadLocal` memory publication. The normal
  `emit_prepared_module(...)` path still has no Route 5 record parameter and
  continues to emit the byte-stable positive prepared output.
- Scalar non-memory Route 5 disagreement still preserves the existing prepared
  fallback behavior; this slice does not broaden scalar edge policy.
- Step 3 should verify adjacent helper/oracle names and prepared lookup helper
  rows remain stable; no helper expectation was weakened in Step 2.

## Proof

Command:

```bash
{ cmake --build build-x86 --target backend_riscv_prepared_edge_publication_test backend_prepared_lookup_helper_test && ctest --test-dir build-x86 -R '^(backend_riscv_prepared_edge_publication|backend_prepared_lookup_helper)$' --output-on-failure; } > test_after.log 2>&1
```

Result: passed. `test_after.log` contains successful builds for
`backend_riscv_prepared_edge_publication_test` and
`backend_prepared_lookup_helper_test`, then CTest reports 2/2 tests passed:
`backend_prepared_lookup_helper` and
`backend_riscv_prepared_edge_publication`.
