Status: Active
Source Idea Path: ideas/open/258_phase_f3_x86_route3_loadlocal_source_memory_agreement_bridge.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Focused Agreement and Rejection Proof

# Current Packet

## Just Finished

Step 4, Focused Agreement and Rejection Proof, hit a route blocker on the
direct addressed-local-slot guard surface and is reset to a supported-fixture
search for the new Route 3 / prepared source-memory agreement facade.

What was checked:
- `backend_x86_handoff_boundary_local_slot_guard_lane_test.cpp` has an
  addressed `LoadLocal` fixture that can provide a Route 3 memory record and
  legacy no-publication compatibility behavior;
- adding a synthetic `PreparedEdgePublication` candidate to that fixture is not
  a valid proof path because the addressed local-slot guard route rejects
  non-empty `join_transfers` before reaching
  `render_agreed_route3_load_local_statement_memory_operand(...)`;
- existing local-slot guard rows therefore prove route-only/no-publication
  fallback and prepared frame/access stability, but they do not prove the
  positive agreement branch or the missing/incomplete/mismatch prepared
  source-publication rejection rows through the x86 facade.

No focused test was committed for Step 4 because the first attempted surface
would have been route-shape overfit: it made an unsupported synthetic
publication shape fail before the selected agreement code ran. The active
Step 4 route is now to search existing supported x86 fixtures before adding any
new proof.

## Suggested Next

Next executor packet: identify a supported x86 fixture whose real prepared
publication shape reaches
`render_agreed_route3_load_local_statement_memory_operand(...)`, then add the
minimal agreement/rejection proof there.

Start with existing real-publication surfaces, in this order:
- `tests/backend/bir/backend_x86_handoff_boundary_short_circuit_test.cpp`,
  especially local-slot short-circuit compare/load rows that already publish
  `join_transfers`, prepared frame-slot authority, and local-slot frame-slot
  accesses;
- if the short-circuit surface cannot cover the selected `LoadLocal`
  `source_memory_access` agreement branch, inspect
  `backend_x86_handoff_boundary_joined_branch_test.cpp` and
  `backend_x86_handoff_boundary_loop_countdown_test.cpp` for a real prepared
  publication path that reaches the same facade.

Only return to plan-owner for a split testability/fixture idea if no existing
supported x86 route can drive the agreement facade without hand-injecting an
unsupported `join_transfers`/`PreparedEdgePublication` shape.

## Watchouts

- Do not count legacy no-publication compatibility fallback as positive Route 3
  agreement proof.
- A valid Step 4 test must use a route where prepared publication ownership is
  already part of the supported x86 shape. Adding ad hoc `join_transfers` to
  the addressed local-slot guard route only proves the route rejects
  unsupported control-flow shape.
- Do not create a new fixture/testability idea until the existing supported
  short-circuit, joined-branch, and loop-countdown x86 surfaces have been
  checked for a real publication path into the agreement facade.
- Duplicate/conflict source-memory pointer rows still appear not coverable
  through the current local prepared lookup surface because
  `find_prepared_memory_access(...)` returns a single access by block and
  instruction.

## Proof

Supervisor-delegated Step 4 proof, run without new focused test edits because
the attempted test surface was invalid:

```bash
(cmake --build --preset default && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_prepared_printer)$' --output-on-failure && cmake --build build-x86 --target backend_x86_route_debug_test backend_x86_handoff_boundary_test -j2 && ctest --test-dir build-x86 -j --output-on-failure -R '^(backend_x86_route_debug|backend_x86_handoff_boundary)$') > test_after.log 2>&1
```

Result: passed, 4/4 tests. Proof log path is `test_after.log`.
