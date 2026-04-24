Status: Active
Source Idea Path: ideas/open/93_hir_agent_index_header_hierarchy.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Split Template/Dependent Lowering Boundary

# Current Packet

## Just Finished

Step 3 - Add Expression And Statement Subdomain Indexes completed the narrow
expression/statement private index slice.

- Added `src/frontend/hir/impl/expr/expr.hpp` as the private expression
  lowering implementation index.
- Added `src/frontend/hir/impl/stmt/stmt.hpp` as the private statement
  lowering implementation index.
- Retargeted `src/frontend/hir/hir_expr*.cpp` to include
  `impl/expr/expr.hpp` and `src/frontend/hir/hir_stmt*.cpp` to include
  `impl/stmt/stmt.hpp`.
- Updated the `impl/lowerer.hpp` navigation comment to point expression and
  statement implementation readers at the new subdomain indexes.

## Suggested Next

Next packet: supervisor should decide whether Step 3 is complete enough for
commit or whether another narrow Step 3 cleanup is needed before moving to the
next runbook step.

## Watchouts

- The new expression and statement headers are directory-level indexes only;
  they intentionally do not introduce one header per implementation file.
- Template, type, callable, build, and core HIR implementation files still
  include `impl/lowerer.hpp` directly.

## Proof

`{ cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '^cpp_hir_'; } > test_after.log 2>&1`

Result: passed for the Step 3 expression/statement subdomain index slice. Build
completed and HIR subset proof passed 71/71 `cpp_hir_*` tests. Proof log:
`test_after.log`.
