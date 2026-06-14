Status: Active
Source Idea Path: ideas/open/245_phase_f1_riscv_route5_route3_edge_publication_adapter.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Introduce the riscv adapter boundary

# Current Packet

## Just Finished

Completed Step 3: introduced the riscv adapter boundary around prepared
edge-publication consumption without changing externally visible behavior.

Changed surfaces:

- `src/backend/mir/riscv/codegen/emit.cpp` now routes
  `consume_edge_publication_move_intent()` through an internal
  `RiscvEdgePublicationMoveAdapter` seam.
- The adapter owns the prepared edge-publication lookup and prepared source-home
  rendering handoff, keeping fallback/status behavior visibly prepared-backed.
- `append_edge_publication_move_instruction()`, `EdgePublicationMoveIntent`,
  `EdgePublicationMoveIntentStatus`, prepared fallback behavior, and exact
  instruction text remain unchanged.
- No Route 5 or Route 3 fact is agreement-gated as authority in this packet.

## Suggested Next

Execute Step 4 from `plan.md`: agreement-gate Route 5 edge/source identity
behind the new riscv adapter seam while preserving prepared fallback behavior,
status names, and exact instruction text.

## Watchouts

- Preserve exact riscv wrapper instruction text and
  `EdgePublicationMoveIntentStatus` names.
- Treat Route 5 and Route 3 facts as agreement evidence only; riscv keeps
  target-local policy.
- Do not claim prepared lookup, aggregate, or draft 155 demotion readiness.
- Route 5 gating should extend `RiscvEdgePublicationMoveAdapter`, not bypass it
  or replace `PreparedFunctionLookups`/`PreparedBirModule` publication fields.
- Route 5 can identify edge/current-block publication source facts, including
  memory-source records, but it must not select riscv registers, stack offsets,
  scratch registers, or instruction spelling.
- Route 3 can compare memory-source identity; it must not authorize dynamic
  stack, pointer-base, aggregate-width, subword, unsigned, F32, large-offset,
  or address-materialization policy on its own.
- Current Route 5 edge index diagnostics expose duplicate edge records, but the
  finder does not reject duplicated edge rows as a conflict today. Treat that
  as diagnostic visibility for the adapter, not as existing route authority.
- The Step 3 seam is behavior-preserving and prepared-backed; Step 4 still owns
  the first route-agreement decision.

## Proof

Ran the supervisor-selected proof:

```bash
cmake --build build --target backend_riscv_prepared_edge_publication_test backend_prepared_lookup_helper_test && ctest --test-dir build -j --output-on-failure -R '^(backend_riscv_prepared_edge_publication|backend_prepared_lookup_helper)$' | tee test_after.log
```

Result: passed 2/2: `backend_riscv_prepared_edge_publication` and
`backend_prepared_lookup_helper`.

Proof log: `test_after.log`.
