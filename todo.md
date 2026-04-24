Status: Active
Source Idea Path: ideas/open/93_hir_agent_index_header_hierarchy.md
Source Plan Path: plan.md
Current Step ID: 7
Current Step Title: Final Acceptance Validation

# Current Packet

## Just Finished

Step 7 `Final Acceptance Validation` completed for the HIR hierarchy
migration.

Acceptance findings:

- Public facade include probe for `frontend/hir/hir.hpp` plus
  `frontend/hir/hir_ir.hpp` compiles with the normal frontend include roots.
- Expression implementation files live under `src/frontend/hir/impl/expr/`
  with prefix-free names.
- Statement implementation files live under `src/frontend/hir/impl/stmt/`
  with prefix-free names.
- Template implementation files live under
  `src/frontend/hir/impl/templates/` with prefix-free names.
- Compile-time implementation files live under
  `src/frontend/hir/impl/compile_time/` as `engine.cpp` and
  `inline_expand.cpp`.
- Inspection implementation lives under
  `src/frontend/hir/impl/inspect/printer.cpp`.
- Private indexes exist for root, lowerer, expression, statement, template,
  compile-time, and inspection work:
  `impl/hir_impl.hpp`, `impl/lowerer.hpp`, `impl/expr/expr.hpp`,
  `impl/stmt/stmt.hpp`, `impl/templates/templates.hpp`,
  `impl/compile_time/compile_time.hpp`, and `impl/inspect/inspect.hpp`.
- No moved top-level `hir_expr*.cpp`, `hir_stmt*.cpp`,
  `hir_templates*.cpp`, `compile_time_engine.cpp`, `inline_expand.cpp`, or
  `hir_printer.cpp` implementation files remain.
- Stale-reference search found no old moved HIR implementation filename
  references in live source, docs, tests, root build files, or scripts outside
  lifecycle/archive paths, except historical test names and hotspot prose that
  are not old implementation file paths.
- Remaining top-level root implementation files are root/facade/pipeline
  implementation: `hir.cpp`, `hir_build.cpp`, `hir_functions.cpp`,
  `hir_lowering_core.cpp`, and `hir_types.cpp`.

## Suggested Next

Ask the plan owner to decide whether the active plan can close.

## Watchouts

- This packet made no implementation or file-move changes.
- Keep any follow-up root implementation movement as a separate lifecycle
  decision; the remaining top-level `.cpp` files are root/facade/pipeline
  implementation for this plan's acceptance scope.
- Leave `review/step3_parser_facade_boundary_review.md` untouched.

## Proof

Focused HIR proof command run exactly:

`{ cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '^cpp_hir'; } > test_after.log 2>&1`

Result: passed; `c4c_frontend` and `c4cll` built successfully, and the
`^cpp_hir` subset passed 71/71 tests.

Full-suite proof command run exactly:

`ctest --test-dir build -j --output-on-failure`

Result: passed; 2974/2974 tests passed.

Proof log: `test_after.log` for the focused proof. The full-suite command was
not redirected by the delegated proof contract.
