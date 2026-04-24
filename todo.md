Status: Active
Source Idea Path: ideas/open/93_hir_agent_index_header_hierarchy.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Create Private HIR Implementation Boundary

# Current Packet

## Just Finished

Step 2 - Create Private HIR Implementation Boundary is closed for the
top-level private HIR compatibility shim cleanup.

- Verified there are no active `#include` users of `hir_lowering.hpp` or
  `hir_lowerer_internal.hpp` under the repo.
- Removed `src/frontend/hir/hir_lowering.hpp` and
  `src/frontend/hir/hir_lowerer_internal.hpp`.
- Updated active HIR source comments to name `impl/hir_impl.hpp` and
  `impl/lowerer.hpp` as the private lowering boundary.

## Suggested Next

Next Step 3 packet: add the expression and statement subdomain indexes, moving
shared expression/statement lowering declarations behind private implementation
headers without widening into parser or backend/codegen work.

## Watchouts

- The top-level private compatibility shims are gone; new HIR implementation
  code should include `impl/hir_impl.hpp` or `impl/lowerer.hpp` directly.
- `impl/lowerer.hpp` lives one directory deeper, so private/header utility
  includes should stay explicit relative paths from `hir/impl/`.
- Keep parser follow-up work from
  `ideas/open/94_parser_public_facade_pimpl_boundary.md` separate.

## Proof

`{ cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '^cpp_hir_'; } > test_after.log 2>&1`

Result: passed for the Step 2 shim-removal cleanup. Build completed and HIR
subset proof passed 71/71 `cpp_hir_*` tests. Proof log: `test_after.log`.
