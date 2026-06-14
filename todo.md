Status: Active
Source Idea Path: ideas/open/245_phase_f1_riscv_route5_route3_edge_publication_adapter.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Agreement-gate Route 3 memory-source identity

# Current Packet

## Just Finished

Completed Step 5: agreement-gated Route 3 memory-source identity behind the
riscv adapter seam for prepared publications that require source-memory access,
without changing externally visible wrapper behavior.

Changed surfaces:

- `src/backend/mir/riscv/codegen/emit.hpp` adds
  `EdgePublicationMoveIntent::route3_source_memory_agrees` diagnostic state.
- `src/backend/mir/riscv/codegen/emit.cpp` compares Route 3 `LoadLocal`
  memory-source identity against prepared memory-source facts for source value
  identity, base kind, pointer/base id when available, byte offset, size,
  alignment, address space, and volatility.
- Route 5 `MemorySource` agreement now requires the embedded Route 3
  memory-source facts to agree with the prepared publication.
- Missing, incomplete, and non-agreeing Route 3 facts record no agreement and
  still return the prepared-backed status and exact riscv instruction text.
- Route 3 does not select riscv registers, stack slots, scratch registers,
  offsets, load/address spelling, address materialization, or wrapper emission.
- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp` now
  proves agreeing Route 3 memory-source identity, mismatched Route 3 fallback,
  incomplete Route 3 fallback, and absent route facts keep exact prepared riscv
  output.

## Suggested Next

Execute Step 6 from `plan.md`: prove compatibility and close readiness for the
riscv adapter, including matched regression-guard scope if the supervisor
delegates it, while confirming prepared field demotion remains deferred to
ideas 246/247.

## Watchouts

- Preserve exact riscv wrapper instruction text and
  `EdgePublicationMoveIntentStatus` names.
- Treat Route 5 and Route 3 facts as agreement evidence only; riscv keeps
  target-local register, stack, scratch, offset, load/address spelling, and
  wrapper policy.
- Do not claim prepared lookup, aggregate, or draft 155 demotion readiness.
- Prepared register-home scalar cases may have no comparable prepared producer
  pointer; producer-kind agreement is required only when prepared carries that
  comparable evidence.
- The Route 3 comparator accepts raw Route 3 pointer-base facts with invalid
  value-name ids only after the rest of the memory-source identity agrees; it
  requires id agreement when Route 3 carries an id.
- Current Route 5 edge index diagnostics expose duplicate edge records, but the
  finder does not reject duplicated edge rows as a conflict today. Treat that
  as diagnostic visibility for the adapter, not as existing route authority.

## Proof

Ran the supervisor-selected proof:

```bash
cmake --build build --target backend_riscv_prepared_edge_publication_test backend_prepared_lookup_helper_test && ctest --test-dir build -j --output-on-failure -R '^(backend_riscv_prepared_edge_publication|backend_prepared_lookup_helper)$' | tee test_after.log
```

Result: passed 2/2: `backend_riscv_prepared_edge_publication` and
`backend_prepared_lookup_helper`.

Proof log: `test_after.log`.
