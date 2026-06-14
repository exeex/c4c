Status: Active
Source Idea Path: ideas/open/245_phase_f1_riscv_route5_route3_edge_publication_adapter.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Agreement-gate Route 5 edge/source identity

# Current Packet

## Just Finished

Completed Step 4: agreement-gated Route 5 edge/source identity behind the
riscv adapter seam without changing externally visible wrapper behavior.

Changed surfaces:

- `src/backend/mir/riscv/codegen/emit.hpp` adds a compatible
  `consume_edge_publication_move_intent(..., route5_edge)` overload plus
  `EdgePublicationMoveIntent` diagnostics for Route 5 edge status/agreement.
- `src/backend/mir/riscv/codegen/emit.cpp` compares supplied Route 5 edge facts
  against the prepared publication's predecessor/successor labels,
  destination/source value identity, and producer kind when the prepared row has
  comparable producer evidence.
- Missing Route 5 facts preserve the existing prepared-only call path; non-
  agreeing or incomplete Route 5 rows record no agreement and still return the
  prepared-backed status and instruction text.
- Route 5 does not select riscv registers, stack slots, scratch registers,
  offsets, instruction spelling, or wrapper emission.
- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp` now
  proves agreeing scalar Route 5 identity, non-agreeing Route 5 fallback, and a
  memory-source Route 5 identity row keep exact prepared riscv output.

## Suggested Next

Execute Step 5 from `plan.md`: agreement-gate Route 3 memory-source identity
for prepared publications that require source-memory access while preserving
prepared handling for pointer-base, large-offset, dynamic-stack, aggregate,
subword, unsigned, F32, and address-materialization policy cases.

## Watchouts

- Preserve exact riscv wrapper instruction text and
  `EdgePublicationMoveIntentStatus` names.
- Treat Route 5 and Route 3 facts as agreement evidence only; riscv keeps
  target-local register, stack, scratch, offset, load/address spelling, and
  wrapper policy.
- Do not claim prepared lookup, aggregate, or draft 155 demotion readiness.
- Step 4 intentionally records Route 5 memory-source edge agreement only at the
  edge/source level. Step 5 still owns Route 3 memory-access identity agreement
  and negative memory-policy proof.
- Prepared register-home scalar cases may have no comparable prepared producer
  pointer; producer-kind agreement is required only when prepared carries that
  comparable evidence.
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
