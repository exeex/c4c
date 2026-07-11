Status: Active
Source Idea Path: ideas/open/684_bir_mir_interface_cleanup_from_prepared_mir_view_docs.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate X86 Internal Module Emission Entry

# Current Packet

## Just Finished

Step 2 (`Migrate X86 Internal Module Emission Entry`) kept
`x86::api::emit_prepared_module(const PreparedBirModule&)` as the public
compatibility wrapper while constructing a `PreparedMirCoreView` and routing
the internal x86 module emitter through a view-taking `x86::module::emit`
surface. Module-entry target identity/triple handling, module data access,
defined-function iteration, and function-view admission now go through the
core view; broad per-function lowering still uses the legacy
`PreparedBirModule` path for Step 3.

## Suggested Next

Start Step 3 by moving the first selected x86 per-function consumption path
behind `PreparedMirFunctionView` or a narrow view-backed input while keeping
Route 6, call, publication, storage, and regalloc authority unchanged unless
explicitly carried by the chosen view-backed handoff.

## Watchouts

- RV64, AArch64, and x86 per-function helpers still use their legacy
  `PreparedBirModule` paths until their own explicit migration packets.
- The internal x86 module entry now fails closed if a defined function has no
  corresponding core function view.

## Proof

- Required Step 2 proof command ran and wrote `test_after.log`:
  `set -o pipefail; (cmake --build build --target c4c_backend backend_prepared_mir_core_view_test -j && ctest --test-dir build -R '^backend_prepared_mir_core_view$|^backend_x86_shared_producer_query$|^backend_codegen_route_x86_64_.*observe_semantic_bir$' --output-on-failure) > test_after.log 2>&1`
- Required Step 2 proof result: passed, 78/78 tests passed.
