Status: Active
Source Idea Path: ideas/open/93_hir_agent_index_header_hierarchy.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory HIR Layout And Build References

# Current Packet

## Just Finished

Step 1 `Inventory HIR Layout And Build References` completed as an
inventory-only packet.

Lightweight commands run:

- `rg --files src/frontend/hir | sort`
- `find src/frontend/hir -maxdepth 3 -type d | sort`
- `find src/frontend/hir -maxdepth 3 -type f | sort`
- `rg -n "frontend/hir|hir/|GLOB|CONFIGURE_DEPENDS|file\\(GLOB|c4c_frontend|frontend" CMakeLists.txt src -g 'CMakeLists.txt' -g '*.cmake'`
- `rg -n "hir_(expr|stmt|templates)|compile_time_engine|inline_expand|hir_printer|hir_lowering_core|hir_build|hir_functions|hir_types|src/frontend/hir|frontend/hir" --glob '!ideas/**' --glob '!plan.md' --glob '!todo.md' --glob '!review/**'`

Current file families:

- Public facade and public entrypoints:
  - `src/frontend/hir/hir.hpp`
  - `src/frontend/hir/hir.cpp` as the public facade implementation and
    pipeline coordinator
  - `src/frontend/hir/compile_time_engine.hpp` remains a public pipeline/dump
    contract
  - `src/frontend/hir/inline_expand.hpp` remains a public transform contract
  - `src/frontend/hir/hir_printer.hpp` remains a public debug/inspection
    contract
- Public IR contract:
  - `src/frontend/hir/hir_ir.hpp`
- Private root implementation indexes:
  - `src/frontend/hir/impl/hir_impl.hpp`
  - `src/frontend/hir/impl/lowerer.hpp`
- Root/private implementation `.cpp` files:
  - `src/frontend/hir/hir_build.cpp`
  - `src/frontend/hir/hir_functions.cpp`
  - `src/frontend/hir/hir_lowering_core.cpp`
  - `src/frontend/hir/hir_types.cpp`
- Expression implementation:
  - `src/frontend/hir/hir_expr.cpp`
  - `src/frontend/hir/hir_expr_builtin.cpp`
  - `src/frontend/hir/hir_expr_call.cpp`
  - `src/frontend/hir/hir_expr_object.cpp`
  - `src/frontend/hir/hir_expr_operator.cpp`
  - `src/frontend/hir/hir_expr_scalar_control.cpp`
  - private index: `src/frontend/hir/impl/expr/expr.hpp`
- Statement implementation:
  - `src/frontend/hir/hir_stmt.cpp`
  - `src/frontend/hir/hir_stmt_control_flow.cpp`
  - `src/frontend/hir/hir_stmt_decl.cpp`
  - `src/frontend/hir/hir_stmt_range_for.cpp`
  - `src/frontend/hir/hir_stmt_switch.cpp`
  - private index: `src/frontend/hir/impl/stmt/stmt.hpp`
- Template/dependent lowering implementation:
  - `src/frontend/hir/hir_templates.cpp`
  - `src/frontend/hir/hir_templates_deduction.cpp`
  - `src/frontend/hir/hir_templates_deferred_nttp.cpp`
  - `src/frontend/hir/hir_templates_global.cpp`
  - `src/frontend/hir/hir_templates_materialization.cpp`
  - `src/frontend/hir/hir_templates_member_typedef.cpp`
  - `src/frontend/hir/hir_templates_struct_instantiation.cpp`
  - `src/frontend/hir/hir_templates_type_resolution.cpp`
  - `src/frontend/hir/hir_templates_value_args.cpp`
  - private index: `src/frontend/hir/impl/templates/templates.hpp`
- Compile-time/materialization and follow-up transform support:
  - `src/frontend/hir/compile_time_engine.cpp`
  - `src/frontend/hir/inline_expand.cpp`
  - private index: `src/frontend/hir/impl/compile_time/compile_time.hpp`
- Inspection/debug support:
  - `src/frontend/hir/hir_printer.cpp`
  - private index: `src/frontend/hir/impl/inspect/inspect.hpp`
- Other HIR-local support:
  - `src/frontend/hir/README.md`

Build/source discovery:

- `src/frontend/CMakeLists.txt` uses
  `file(GLOB_RECURSE C4C_FRONTEND_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")`
  followed by `list(SORT C4C_FRONTEND_SOURCES)`.
- No explicit HIR `.cpp` source list needs edits after moving files under
  `src/frontend/hir/**`; CMake should reconfigure because the recursive glob
  uses `CONFIGURE_DEPENDS`.
- Include directories already expose `src/frontend/hir` to frontend consumers;
  moved private implementation files should prefer relative/private index
  includes rather than adding new include directories.

Intended move map for later packets:

- Keep top-level unless Step 2 decides otherwise:
  - `hir.hpp`
  - `hir_ir.hpp`
  - `compile_time_engine.hpp`
  - `inline_expand.hpp`
  - `hir_printer.hpp`
  - `hir.cpp`
- Private root implementation candidates:
  - `hir_build.cpp` -> `impl/build.cpp`
  - `hir_functions.cpp` -> `impl/functions.cpp`
  - `hir_lowering_core.cpp` -> `impl/lowering_core.cpp`
  - `hir_types.cpp` -> `impl/types.cpp`
- Expression moves:
  - `hir_expr.cpp` -> `impl/expr/expr.cpp`
  - `hir_expr_builtin.cpp` -> `impl/expr/builtin.cpp`
  - `hir_expr_call.cpp` -> `impl/expr/call.cpp`
  - `hir_expr_object.cpp` -> `impl/expr/object.cpp`
  - `hir_expr_operator.cpp` -> `impl/expr/operator.cpp`
  - `hir_expr_scalar_control.cpp` -> `impl/expr/scalar_control.cpp`
- Statement moves:
  - `hir_stmt.cpp` -> `impl/stmt/stmt.cpp`
  - `hir_stmt_control_flow.cpp` -> `impl/stmt/control_flow.cpp`
  - `hir_stmt_decl.cpp` -> `impl/stmt/decl.cpp`
  - `hir_stmt_range_for.cpp` -> `impl/stmt/range_for.cpp`
  - `hir_stmt_switch.cpp` -> `impl/stmt/switch.cpp`
- Template moves:
  - `hir_templates.cpp` -> `impl/templates/templates.cpp`
  - `hir_templates_deduction.cpp` -> `impl/templates/deduction.cpp`
  - `hir_templates_deferred_nttp.cpp` -> `impl/templates/deferred_nttp.cpp`
  - `hir_templates_global.cpp` -> `impl/templates/global.cpp`
  - `hir_templates_materialization.cpp` -> `impl/templates/materialization.cpp`
  - `hir_templates_member_typedef.cpp` -> `impl/templates/member_typedef.cpp`
  - `hir_templates_struct_instantiation.cpp` -> `impl/templates/struct_instantiation.cpp`
  - `hir_templates_type_resolution.cpp` -> `impl/templates/type_resolution.cpp`
  - `hir_templates_value_args.cpp` -> `impl/templates/value_args.cpp`
- Compile-time and transform moves:
  - `compile_time_engine.cpp` -> `impl/compile_time/engine.cpp`
  - `inline_expand.cpp` -> `impl/compile_time/inline_expand.cpp` if the later
    packet keeps it coupled to compile-time/materialized HIR state
- Inspection/debug moves:
  - `hir_printer.cpp` -> `impl/inspect/printer.cpp` if `hir_printer.hpp`
    remains the public inspection facade and `impl/inspect/inspect.hpp`
    becomes the implementation-side bridge.
- Uncertain/later-decision items:
  - `hir.cpp` should likely stay top-level because it implements the public
    facade entrypoint.
  - `compile_time_engine.hpp`, `inline_expand.hpp`, and `hir_printer.hpp`
    are currently public by README contract and app/test includes.
  - `docs/frontend_compile_time_hotspot_inventory.md`, `src/frontend/hir/hir.hpp`,
    `src/frontend/hir/impl/hir_impl.hpp`, and `src/frontend/hir/impl/lowerer.hpp`
    contain old filename references that should be refreshed in the later
    stale-reference/doc cleanup packet.

## Suggested Next

Execute Step 2: clean root public/private HIR boundaries before moving broad
implementation families. Keep public headers top-level unless call sites are
updated, and refresh private index comments only where needed for the boundary
work.

## Watchouts

- Keep this initiative structural and behavior-preserving.
- Do not change HIR semantics, testcase expectations, template behavior, or
  codegen-facing HIR output while moving files.
- Leave `review/step3_parser_facade_boundary_review.md` untouched.
- CMake source discovery is recursive, but future move packets should still
  rely on the delegated build to confirm the glob reconfigured.
- Public app/test includes currently use `hir_printer.hpp`,
  `compile_time_engine.hpp`, `inline_expand.hpp`, and `hir/hir_ir.hpp`; do not
  demote these headers without updating callers and validating the wider
  include surface.
- Existing HIR comments/docs still name old top-level `.cpp` paths; stale
  reference cleanup should happen after the relevant move packet, not in this
  inventory slice.

## Proof

Inventory-only lifecycle scratchpad work; no build or test validation required
and no `test_after.log` was created.

Likely proof commands for future code-moving packets:

- Focused default:
  `{ cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$'; } > test_after.log 2>&1`
- Add parser proof if public AST/parser-facing includes change:
  `{ cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|frontend_parser_tests)$'; } > test_after.log 2>&1`
- Escalate to full `ctest --test-dir build -j --output-on-failure` when a
  packet changes codegen-facing HIR declarations, template materialization,
  compile-time normalization, or LIR/LLVM include paths.
