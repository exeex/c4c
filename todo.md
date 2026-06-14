Status: Active
Source Idea Path: ideas/open/258_phase_f3_x86_route3_loadlocal_source_memory_agreement_bridge.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement or Prove the Narrow x86 Agreement Path

# Current Packet

## Just Finished

Step 3, Implement or Prove the Narrow x86 Agreement Path, replaced the reset
implementation in `src/backend/mir/x86/module/module.cpp` with a narrow
x86-local Route 3 / prepared source-memory agreement facade.

Implemented facts:
- selected x86 local-slot `LoadLocal` render paths now call
  `render_agreed_route3_load_local_statement_memory_operand(...)` before using
  prepared memory data as the semantic mirror;
- the agreement facade requires a Route 3 `LoadLocal` memory record, a matching
  `PreparedEdgePublication` row whose source producer is the same prepared
  block/instruction/value, `source_memory_access_status == Available`, and a
  matching `PreparedEdgePublication::source_memory_access`;
- raw `PreparedMemoryAccess` is used only after the publication source-memory
  row agrees with Route 3, and the rendered access pointer must still be the
  agreed publication source-memory access;
- Route 3 local-slot identity is mapped through prepared frame-slot metadata:
  publication frame-slot id -> prepared frame slot -> prepared stack object ->
  prepared slot spelling, then compared with the Route 3 BIR slot spelling;
- no `SlotNameId` / `PreparedFrameSlotId` cast or direct id-domain comparison
  is used;
- legacy/no-publication `LoadLocal` compatibility rows remain distinct from
  positive agreement: if no prepared source-publication candidate exists, the
  legacy prepared memory renderer may preserve existing output, but the
  agreement predicate still returns no agreed source-memory access.

## Suggested Next

Proceed to Step 4 focused agreement and rejection proof. The next packet should
add or confirm direct x86-enabled rows for accepted agreement plus missing,
incomplete, duplicate/conflict, prepared-only, route-only, unsupported,
fallback, mismatch, and policy-sensitive rejection behavior without weakening
prepared lookup/status or output contracts.

## Watchouts

- Bypass scan in `src/backend/mir/x86/module/module.cpp`: selected local-slot
  `LoadLocal` memory/source consumers in the symbol-call lane, one-block
  return lane, i32 guard-prefix lane, scalar guard block lane, and short-circuit
  compare-load helper route through the agreement wrapper. Remaining
  `render_prepared_local_slot_statement_memory_operand(...)` calls are stores
  or the compatibility render performed after agreement.
- The facade currently fails closed when a prepared source-publication
  candidate exists but has missing/incomplete source-memory rows, mismatched
  source producer block/instruction/value, route/prepared memory mismatch, or
  duplicate/conflicting source-memory access pointers. Rows with no prepared
  source-publication candidate use the legacy compatibility render and must not
  be counted as Route 3 agreement proof.
- Public prepared lookup/status surfaces, route-debug strings, fallback names,
  helper/oracle names, x86 output formatting, and target-owned addressing or
  register/materialization policy were not intentionally changed.

## Proof

Supervisor-delegated Step 3 proof:

```bash
(cmake --build --preset default && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_prepared_printer)$' --output-on-failure && cmake --build build-x86 --target backend_x86_route_debug_test backend_x86_handoff_boundary_test -j2 && ctest --test-dir build-x86 -j --output-on-failure -R '^(backend_x86_route_debug|backend_x86_handoff_boundary)$') > test_after.log 2>&1
```

Result: passed, 4/4 tests. Proof log path is `test_after.log`.
