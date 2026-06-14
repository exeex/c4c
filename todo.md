Status: Active
Source Idea Path: ideas/open/259_phase_f3_x86_route5_prepared_edge_publication_agreement_bridge.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Narrow x86 Agreement Bridge

# Current Packet

## Just Finished

Step 3: `Implement Narrow x86 Agreement Bridge` is complete.

Implemented a module-local Route 5/prepared agreement facade in
`src/backend/mir/x86/module/module.cpp`, near the compare-join
edge-publication consumer. `append_prepared_compare_join_parallel_copy(...)`
now keeps BIR function/block context in the x86 module layer, calls the
existing prepared dispatch helper for lookup/status/operand rendering, then
uses the local facade to compare the selected `PreparedEdgePublication` row
against Route 5 edge-publication facts.

The facade resolves predecessor and successor BIR blocks from prepared labels,
builds the Route 5 edge/join source index for the current BIR function, rejects
duplicate matching Route 5 edge records, and checks same edge, destination
value name/type, source value kind/name/type, source producer kind, producer
block label, and producer instruction index. Dynamic `LoadLocal` publications
require Route 5 memory-source evidence and reuse the existing Route 3
source-memory agreement helper before allowing the x86 publication move path.

Compatibility retained: `consume_edge_publication_move_intent(...)` and public
prepared edge-publication lookup/status APIs were not widened or renamed, and
non-`LoadLocal` non-agreeing rows remain on the existing compatibility
prepared-emission path instead of claiming Route 5 agreement. x86 output
spelling, register choice, fallback behavior, route-debug text, and RISC-V
diagnostics were preserved by the delegated proof.

## Suggested Next

Step 4: add focused agreement and rejection proof for the Route 5 bridge. Cover
the positive selected `LoadLocal` publication path and naturally reachable
fail-closed rows for missing/duplicate/wrong Route 5 evidence, source producer
mismatch, and Route 3 source-memory disagreement. Do not hand-build stale
prepared publication state or downgrade expectations.

## Watchouts

- The Step 3 implementation intentionally preserves compatibility fallback for
  existing non-`LoadLocal` prepared publication moves; Step 4 should prove that
  these rows do not claim Route 5 agreement rather than forcing a behavior
  downgrade.
- `LoadLocal` edge-publication moves are the fail-closed branch because they
  require Route 5 memory-source identity plus Route 3 source-memory agreement.
- Keep BIR context in `module.cpp`; do not move block discovery or target
  policy into `consume_edge_publication_move_intent(...)`.
- Avoid synthetic/stale publication rows and named-case shortcuts.

## Proof

Delegated proof command:

```bash
(cmake --build --preset default && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_prepared_printer|backend_riscv_prepared_edge_publication)$' --output-on-failure && cmake --build build-x86 --target backend_x86_route_debug_test backend_x86_handoff_boundary_test -j2 && ctest --test-dir build-x86 -j --output-on-failure -R '^(backend_x86_route_debug|backend_x86_handoff_boundary)$') > test_after.log 2>&1
```

Result: passed. `test_after.log` shows 3/3 default
prepared/RISC-V tests and 2/2 x86-enabled tests passed.
