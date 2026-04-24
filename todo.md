Status: Active
Source Idea Path: ideas/open/93_hir_agent_index_header_hierarchy.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Move Expression And Statement Implementation Families

# Current Packet

## Just Finished

Step 3 `Move Expression And Statement Implementation Families` completed as a
behavior-preserving structure packet.

Moved expression implementation files under `src/frontend/hir/impl/expr/`:

- `src/frontend/hir/hir_expr.cpp` -> `src/frontend/hir/impl/expr/expr.cpp`
- `src/frontend/hir/hir_expr_builtin.cpp` -> `src/frontend/hir/impl/expr/builtin.cpp`
- `src/frontend/hir/hir_expr_call.cpp` -> `src/frontend/hir/impl/expr/call.cpp`
- `src/frontend/hir/hir_expr_object.cpp` -> `src/frontend/hir/impl/expr/object.cpp`
- `src/frontend/hir/hir_expr_operator.cpp` -> `src/frontend/hir/impl/expr/operator.cpp`
- `src/frontend/hir/hir_expr_scalar_control.cpp` -> `src/frontend/hir/impl/expr/scalar_control.cpp`

Moved statement implementation files under `src/frontend/hir/impl/stmt/`:

- `src/frontend/hir/hir_stmt.cpp` -> `src/frontend/hir/impl/stmt/stmt.cpp`
- `src/frontend/hir/hir_stmt_control_flow.cpp` -> `src/frontend/hir/impl/stmt/control_flow.cpp`
- `src/frontend/hir/hir_stmt_decl.cpp` -> `src/frontend/hir/impl/stmt/decl.cpp`
- `src/frontend/hir/hir_stmt_range_for.cpp` -> `src/frontend/hir/impl/stmt/range_for.cpp`
- `src/frontend/hir/hir_stmt_switch.cpp` -> `src/frontend/hir/impl/stmt/switch.cpp`

Updated moved-file includes to use the local subdomain index headers, refreshed
the stale top-level expression/statement implementation filenames in live docs
and HIR helper-test comments, and confirmed no moved top-level
`hir_expr*.cpp` or `hir_stmt*.cpp` files remain.

## Suggested Next

Execute Step 4: move the template lowering implementation family under
`src/frontend/hir/impl/templates/`, rename the moved files to short
subdomain-local filenames, and update includes/stale references for that move
only.

## Watchouts

- Keep this initiative structural and behavior-preserving.
- Do not change HIR semantics, testcase expectations, template behavior, or
  codegen-facing HIR output while moving files.
- Leave `review/step3_parser_facade_boundary_review.md` untouched.
- CMake source discovery is recursive, but future move packets should still
  rely on the delegated build to confirm the glob reconfigured.
- Public app/test includes still use `hir_printer.hpp`,
  `compile_time_engine.hpp`, `inline_expand.hpp`, and `hir/hir_ir.hpp`; do not
  demote these headers without updating callers and validating the wider
  include surface.
- Template implementation files still use old top-level `hir_templates*.cpp`
  names and should be handled by the Step 4 packet only.
- Compile-time, inline expansion, inspection, and root HIR implementation files
  were intentionally left in place for later plan steps.
- The `frontend_hir_tests` CTest selector does not exist in this build tree;
  this packet used the supervisor-selected `^cpp_hir` subset instead.

## Proof

Delegated proof command run exactly:

`{ cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '^cpp_hir'; } > test_after.log 2>&1`

Result: passed; `c4c_frontend` and `c4cll` built successfully, and the
`^cpp_hir` subset passed 71/71 tests.

Proof log: `test_after.log`.
