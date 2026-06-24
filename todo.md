Status: Active
Source Idea Path: ideas/open/344_rv64_pointer_typed_select_publication_self_move.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The General Self-Move Path

# Current Packet

## Just Finished

Step 2 repaired the general RV64 prepared edge-publication self-move path in
`src/backend/mir/riscv/codegen/prepared_edge_publication_emit.cpp`. Register
destination publications now keep real differing-register `mv` instructions but
render same-register register and zero-delta pointer-base publications as
available no-ops with empty instruction text; the append helper skips empty
available instructions, so the terminal publication path no longer emits
`mv t0, t0` after the function body.

Focused coverage in
`tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp` preserves
the existing `mv a1, a0` assertion for differing-register publications and adds
a same-register fixture proving `mv a0, a0` is omitted while the publication is
still consumed through shared lookup authority.

## Suggested Next

Supervisor should decide whether Step 3 is lifecycle close review or broader
validation. The focused Step 2 proof is green and
`backend_codegen_route_riscv64_pointer_typed_select_publication` no longer
emits the forbidden terminal self-move.

## Watchouts

- The forbidden-snippet expectations were not weakened.
- The repair is semantic in the RV64 prepared edge-publication move emitter, not
  keyed to the failing test name or generated assembly path.
- Existing `_expected_repair.s` fixture files still contain literal `mv t0, t0`
  text, but the generated
  `build/tests/backend/riscv64_pointer_typed_select_publication.s` output does
  not.
- `emit.hpp` was left untouched because it was outside the delegated owned
  files; no new public status enum value was added.

## Proof

Ran the delegated Step 2 proof exactly:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '(^backend_riscv_prepared_edge_publication$|^backend_(dump|codegen_route)_riscv64_(pointer_integer_select_chain|pointer_typed_select_publication|nested_select_store_source_publication|short_circuit_select_false_lhs|compare_result_select_false_arm|byval_formal_gpr_publication)$)') > test_after.log 2>&1`.

Result: passed, `12/12` selected tests green. `test_after.log` is the preserved
proof log.
