Status: Active
Source Idea Path: ideas/open/93_hir_agent_index_header_hierarchy.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Resolve Inspection Surface And Refresh Documentation

# Current Packet

## Just Finished

Step 6 `Resolve Inspection Surface And Refresh Documentation` completed as a
behavior-preserving inspection/docs packet.

Kept `hir_printer.hpp` public because `src/apps/c4cll.cpp` and
`tests/frontend/frontend_hir_tests.cpp` include it directly for normal dump
formatting use. Moved only the implementation:

- `src/frontend/hir/hir_printer.cpp` -> `src/frontend/hir/impl/inspect/printer.cpp`

Updated the moved implementation to include the existing private
`impl/inspect/inspect.hpp` index. Refreshed `src/frontend/hir/README.md` with
the actual HIR tree, public header roles, private index roles, and agent
navigation guidance.

Stale-reference search found no live references to the old
`src/frontend/hir/hir_printer.cpp` path outside lifecycle/archive paths. The
remaining `hir_expr_*` and `hir_stmt_*` matches are test names or historical
hotspot notes, not old moved implementation filenames.

## Suggested Next

Execute Step 7: final acceptance validation for the HIR hierarchy migration.

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
- Root HIR implementation files such as `hir.cpp`, `hir_build.cpp`,
  `hir_functions.cpp`, `hir_lowering_core.cpp`, and `hir_types.cpp` were not
  moved in Step 6; that was outside this packet.
- The `frontend_hir_tests` CTest selector does not exist in this build tree;
  this packet used the supervisor-selected `^cpp_hir` subset instead.
- This packet did not change public declarations or HIR behavior. Full CTest is
  not specifically warranted by this diff, but Step 7 is the milestone
  acceptance point and may choose broader validation.

## Proof

Delegated proof command run exactly:

`{ cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '^cpp_hir'; } > test_after.log 2>&1`

Result: passed; `c4c_frontend` and `c4cll` built successfully, and the
`^cpp_hir` subset passed 71/71 tests.

Proof log: `test_after.log`.
