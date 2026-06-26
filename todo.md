Status: Active
Source Idea Path: ideas/open/379_rv64_object_route_20000112_runtime_join_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Implement the First Semantic Publication Repair

# Current Packet

## Just Finished

Step 2 implemented the first semantic RV64 object-route publication repair for
prepared predecessor-terminator `select_materialization` parallel copies.
The shared edge-publication adapter now accepts authoritative immediate
sources carried by `PreparedMoveResolution::source_immediate_i32`, so an
immediate incoming value can publish directly into the join result register
without requiring a separate source home. The object route now validates
predecessor-terminator select-publication bundles before using the existing
move-bundle emitter, admitting only immediate-to-GPR and GPR-to-GPR moves and
rejecting adjacent stack-source or non-select-carrier shapes.

Focused object coverage now asserts the published select carrier is skipped at
the join, predecessor terminators emit the expected `li` and `mv` publication
bytes before their jumps, and unsupported adjacent publication shapes remain
fail-closed.

## Suggested Next

Run Step 3 for `src/20000112-1.c` through the supervisor-provided RV64 GCC
torture allowlist command, then record whether the representative passes or
advances to a distinct next owner.

## Watchouts

The object route still intentionally rejects select-publication stack sources,
cycle-temp moves, non-predecessor execution, and carrier drift. The validator
uses prepared publication and parallel-copy metadata; do not loosen it through
testcase names, exact labels, hard-coded value ids, or physical-register
special cases. The next packet should not broaden into CFG reconstruction or
register allocation replacement if the representative exposes a different
owner.

## Proof

Ran the supervisor-selected proof command:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_riscv_prepared_edge_publication|backend_riscv_object_emission|backend_codegen_route_riscv64_(short_circuit_select_false_lhs|compare_result_select_false_arm|pointer_typed_select_publication)|backend_rv64_runtime_riscv64_(short_circuit_select_false_lhs|compare_result_select_false_arm|pointer_typed_select_publication))$' | tee test_after.log`

Result: passed, 8/8 tests green. Proof log: `test_after.log`.
