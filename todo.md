Status: Active
Source Idea Path: ideas/open/344_rv64_pointer_typed_select_publication_self_move.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reproduce And Localize The Self-Move

# Current Packet

## Just Finished

Step 1 reproduced and localized
`backend_codegen_route_riscv64_pointer_typed_select_publication` without
implementation changes. The focused generated assembly at
`build/tests/backend/riscv64_pointer_typed_select_publication.s` ends with
`ret` followed by `mv t0, t0`, so the forbidden self-move is appended after the
function body rather than produced by the in-block select lowering.

Suspected path: `emit_prepared_module_text()` in
`src/backend/mir/riscv/codegen/prepared_module_emit.cpp` calls
`append_available_edge_publication_moves()` after
`append_simple_prepared_bir_function_asm()`. That helper delegates to
`append_edge_publication_move_instruction()` in
`src/backend/mir/riscv/codegen/prepared_edge_publication_emit.cpp`, whose
register-destination fallback can build `mv <destination>, <source>` without a
same-register guard. Existing `emit_move_to_register()` guards same-register
copies, and `peephole.cpp` has `eliminate_self_moves()`, but the prepared-module
terminal text route does not appear to run `peephole_optimize()` after these
late publication appends.

## Suggested Next

Execute Step 2: repair the RV64 edge-publication append path so available
register-to-same-register publication moves do not emit terminal self-moves.
The bounded target is the prepared edge-publication move emission path, with a
focused proof on
`backend_codegen_route_riscv64_pointer_typed_select_publication` and nearby RV64
publication/select route tests.

## Watchouts

- Keep the forbidden-snippet contract for `mv t0, t0` intact.
- Do not special-case the failing test name, fixture path, or exact emitted
  assembly.
- Preserve real publication moves where source and destination registers differ;
  this packet only localized the same-register no-op path.
- If choosing between a local emission guard and a terminal peephole call,
  account for the fact that the offending move is appended after `ret` by
  `append_available_edge_publication_moves()`.
- The previous source idea remains open and parked until this broad-proof
  blocker is resolved or explicitly dispositioned.

## Proof

Ran the delegated localization proof exactly:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_codegen_route_riscv64_pointer_typed_select_publication$') > test_after.log 2>&1 || true`.

Result: expected focused failure reproduced. `test_after.log` reports
`[BACKEND_ROUTE_FORBIDDEN_SNIPPET]` for
`build/tests/backend/riscv64_pointer_typed_select_publication.s` with unexpected
snippet `mv t0, t0`. The failing focused test is expected for Step 1
localization and is not a localization blocker.
