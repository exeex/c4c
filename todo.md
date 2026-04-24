Status: Active
Source Idea Path: ideas/open/93_hir_agent_index_header_hierarchy.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Move Compile-Time And HIR-Local Follow-Up Work

# Current Packet

## Just Finished

Step 5 `Move Compile-Time And HIR-Local Follow-Up Work` completed as a
behavior-preserving structure packet.

Moved compile-time and coupled HIR-local follow-up implementation files under
`src/frontend/hir/impl/compile_time/`:

- `src/frontend/hir/compile_time_engine.cpp` -> `src/frontend/hir/impl/compile_time/engine.cpp`
- `src/frontend/hir/inline_expand.cpp` -> `src/frontend/hir/impl/compile_time/inline_expand.cpp`

Kept `compile_time_engine.hpp` and `inline_expand.hpp` as public top-level
contracts. Updated moved-file includes to use the existing private
`impl/compile_time/compile_time.hpp` index for the engine and the public
inline-expansion contract for the app-facing transform. Confirmed no owned old
`compile_time_engine.cpp` or `inline_expand.cpp` filename references remain in
live source, docs, tests, or root build files outside lifecycle/archive paths.

## Suggested Next

Execute Step 6: resolve the inspection/debug surface and refresh the HIR README
so the documented HIR tree matches the implementation layout after the parser
and HIR move packets.

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
- Inspection/debug support and root HIR implementation files were intentionally
  left in place for later plan steps.
- The `frontend_hir_tests` CTest selector does not exist in this build tree;
  this packet used the supervisor-selected `^cpp_hir` subset instead.
- This packet did not change public declarations or HIR behavior; full CTest is
  not specifically warranted by this diff, though the supervisor may still
  choose it for milestone confidence.

## Proof

Delegated proof command run exactly:

`{ cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '^cpp_hir'; } > test_after.log 2>&1`

Result: passed; `c4c_frontend` and `c4cll` built successfully, and the
`^cpp_hir` subset passed 71/71 tests.

Proof log: `test_after.log`.
