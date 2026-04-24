Status: Active
Source Idea Path: ideas/open/93_hir_agent_index_header_hierarchy.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Create Private HIR Implementation Boundary

# Current Packet

## Just Finished

Step 2 - Create Private HIR Implementation Boundary completed the private
Lowerer engine index slice.

- Added `src/frontend/hir/impl/lowerer.hpp` with the implementation-only
  `Lowerer` declarations and helper declarations that had lived in
  `hir_lowerer_internal.hpp`.
- Reduced `src/frontend/hir/hir_lowerer_internal.hpp` to a transitional shim
  that includes the private Lowerer index.
- Retargeted HIR implementation `.cpp` files that directly included
  `hir_lowerer_internal.hpp` to include `impl/lowerer.hpp`.
- Verified no direct `#include "hir_lowerer_internal.hpp"` sites remain under
  `src/frontend/hir/`.
- Tightened `impl/lowerer.hpp` to include `hir_impl.hpp` directly and to use
  explicit relative includes for HIR-adjacent utility headers.

## Suggested Next

Next Step 2 closure packet: decide whether the transitional shims
`hir_lowering.hpp` and `hir_lowerer_internal.hpp` should stay for compatibility
through this plan or be removed after any remaining include-boundary review.

## Watchouts

- `hir_lowerer_internal.hpp` is now only a compatibility shim; new HIR
  implementation code should include `impl/lowerer.hpp` for Lowerer internals.
- `impl/lowerer.hpp` lives one directory deeper, so private/header utility
  includes should stay explicit relative paths from `hir/impl/`.
- Keep parser follow-up work from
  `ideas/open/94_parser_public_facade_pimpl_boundary.md` separate.

## Proof

`{ cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '^cpp_hir_'; } > test_after.log 2>&1`

Result: passed after the Step 2 private Lowerer engine index slice. HIR subset
proof passed 71/71 `cpp_hir_*` tests. Proof log: `test_after.log`.
