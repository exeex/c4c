Status: Active
Source Idea Path: ideas/open/93_hir_agent_index_header_hierarchy.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Split Template/Dependent Lowering Boundary

# Current Packet

## Just Finished

Step 4 - Split Template/Dependent Lowering Boundary completed the narrow
template/dependent private index slice.

- Added `src/frontend/hir/impl/templates/templates.hpp` as the private
  template/dependent lowering implementation index.
- Retargeted `src/frontend/hir/hir_templates*.cpp` to include
  `impl/templates/templates.hpp` instead of directly including
  `impl/lowerer.hpp`.
- Updated the `impl/lowerer.hpp` navigation comment to point template-family
  implementation readers at the new subdomain index.

## Suggested Next

Next packet: Step 5 - Move Compile-Time And Follow-Up Transform Boundary.
Introduce `src/frontend/hir/impl/compile_time/compile_time.hpp` as the
compile-time/materialization private index, then decide whether inline
expansion belongs under that boundary or should be recorded as separate scope.

## Watchouts

- The new template header is a directory-level index only; it intentionally
  does not introduce one header per template-family implementation file.
- Template-family translation units still reach the same declarations through
  `impl/templates/templates.hpp`, so behavior should remain unchanged.
- Type, callable, build, compile-time, and core HIR implementation files still
  include `impl/lowerer.hpp` directly.

## Proof

`{ cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '^cpp_hir_'; } > test_after.log 2>&1`

Result: passed for the Step 4 template/dependent subdomain index slice. Build
completed and HIR subset proof passed 71/71 `cpp_hir_*` tests. Proof log:
`test_after.log`.
