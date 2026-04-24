Status: Active
Source Idea Path: ideas/open/93_hir_agent_index_header_hierarchy.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Clean Root Public And Private HIR Boundaries

# Current Packet

## Just Finished

Step 2 `Clean Root Public And Private HIR Boundaries` completed as a
behavior-preserving boundary cleanup packet.

Updated comments in:

- `src/frontend/hir/hir.hpp`
- `src/frontend/hir/hir_ir.hpp`
- `src/frontend/hir/hir_printer.hpp`
- `src/frontend/hir/impl/hir_impl.hpp`
- `src/frontend/hir/impl/lowerer.hpp`

Boundary decisions:

- `hir.hpp` remains the public pipeline facade and exposes only
  `build_hir(...)` plus `format_summary(...)`.
- `hir_ir.hpp` remains the public HIR data-model contract shared by frontend,
  sema, codegen, dump tooling, and HIR-local transforms.
- `impl/hir_impl.hpp` and `impl/lowerer.hpp` remain the private lowering
  indexes for shared lowering declarations and the implementation-only
  `Lowerer` engine.
- `format_summary(...)` remains public through `hir.hpp`.
- `format_hir(...)` remains public for now through `hir_printer.hpp` because
  `c4cll --dump-hir` and frontend HIR tests include the full dump surface
  explicitly.
- `compile_time_engine.hpp` remains public for now because app/pipeline callers
  use the compile-time retry and materialization contract directly; private
  implementation files should prefer `impl/compile_time/compile_time.hpp`.
- `inline_expand.hpp` remains public for now because `c4cll` invokes the
  follow-up transform directly before emission.
- No public AST/parser-facing includes were changed, so parser proof was not
  added to this packet.

## Suggested Next

Execute Step 3: move the expression and statement implementation families under
`src/frontend/hir/impl/expr/` and `src/frontend/hir/impl/stmt/`, rename them to
short subdomain-local filenames, and update includes/stale references for those
moves only.

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
- Existing docs/tests still name old top-level `.cpp` paths for later move
  packets; cleanup those references only with the packet that owns the move.
- The delegated proof command exited successfully, but the exact
  `ctest -R '^frontend_hir_tests$'` selector matched no tests in this build
  tree. Supervisor follow-up used the registered `cpp_hir` HIR subset instead.

## Proof

Delegated proof command run exactly:

`{ cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$'; } > test_after.log 2>&1`

Result: command exited 0; `c4c_frontend` and `c4cll` built successfully.
`ctest` reported `No tests were found!!!` for the exact
`^frontend_hir_tests$` selector, so no focused HIR test binary was run by that
regex.

Supervisor follow-up proof passed:

`{ cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '^cpp_hir'; } > test_after.log 2>&1`

Result: 100% tests passed, 0 tests failed out of 71.

Proof log: `test_after.log`.
