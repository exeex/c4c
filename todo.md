Status: Active
Source Idea Path: ideas/open/93_hir_agent_index_header_hierarchy.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Create Private HIR Implementation Boundary

# Current Packet

## Just Finished

Step 2 - Create Private HIR Implementation Boundary completed the first private
HIR implementation index slice.

- Added `src/frontend/hir/impl/hir_impl.hpp` with the shared HIR lowering
  declarations that had lived in `hir_lowering.hpp`.
- Reduced `src/frontend/hir/hir_lowering.hpp` to a compatibility shim that
  includes the private implementation index.
- Retargeted direct HIR implementation `.cpp` includes from `hir_lowering.hpp`
  to `impl/hir_impl.hpp`: `hir.cpp`, `hir_build.cpp`, `hir_expr.cpp`,
  `hir_functions.cpp`, `hir_lowering_core.cpp`, and `hir_types.cpp`.

## Suggested Next

Next Step 2 sub-slice: introduce the private `impl/lowerer.hpp` boundary for
the declarations currently exposed through `hir_lowerer_internal.hpp`, then
retarget implementation includes that need the `Lowerer` internals.

## Watchouts

- Remaining `hir_lowering.hpp` include site after this slice:
  `src/frontend/hir/hir_lowerer_internal.hpp`. Leave that for the
  `impl/lowerer.hpp` sub-slice unless the supervisor broadens ownership.
- Preserve `hir_lowering.hpp` as a temporary shim while transitional include
  sites still exist.
- Keep parser follow-up work from
  `ideas/open/94_parser_public_facade_pimpl_boundary.md` separate.

## Proof

`{ cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '^cpp_hir_'; } > test_after.log 2>&1`

Result: passed after the Step 2 private implementation index slice. HIR subset
proof passed 71/71 `cpp_hir_*` tests. Proof log: `test_after.log`.
