Status: Active
Source Idea Path: ideas/open/93_hir_agent_index_header_hierarchy.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Move Template Lowering Family

# Current Packet

## Just Finished

Step 4 `Move Template Lowering Family` completed as a
behavior-preserving structure packet.

Moved template implementation files under `src/frontend/hir/impl/templates/`:

- `src/frontend/hir/hir_templates.cpp` -> `src/frontend/hir/impl/templates/templates.cpp`
- `src/frontend/hir/hir_templates_deduction.cpp` -> `src/frontend/hir/impl/templates/deduction.cpp`
- `src/frontend/hir/hir_templates_deferred_nttp.cpp` -> `src/frontend/hir/impl/templates/deferred_nttp.cpp`
- `src/frontend/hir/hir_templates_global.cpp` -> `src/frontend/hir/impl/templates/global.cpp`
- `src/frontend/hir/hir_templates_materialization.cpp` -> `src/frontend/hir/impl/templates/materialization.cpp`
- `src/frontend/hir/hir_templates_member_typedef.cpp` -> `src/frontend/hir/impl/templates/member_typedef.cpp`
- `src/frontend/hir/hir_templates_struct_instantiation.cpp` -> `src/frontend/hir/impl/templates/struct_instantiation.cpp`
- `src/frontend/hir/hir_templates_type_resolution.cpp` -> `src/frontend/hir/impl/templates/type_resolution.cpp`
- `src/frontend/hir/hir_templates_value_args.cpp` -> `src/frontend/hir/impl/templates/value_args.cpp`

Updated moved-file includes to use the local subdomain index headers, refreshed
the stale top-level template implementation filenames in live docs and the HIR
template helper-test comment, and confirmed no moved top-level
`hir_templates*.cpp` files remain in live source, docs, tests, or root build
files outside lifecycle/archive paths.

## Suggested Next

Execute Step 5: move compile-time and HIR-local follow-up implementation under
`src/frontend/hir/impl/compile_time/`, keeping the packet structural and
updating only directly owned includes and references.

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
