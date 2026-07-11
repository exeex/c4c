Status: Active
Source Idea Path: ideas/open/684_bir_mir_interface_cleanup_from_prepared_mir_view_docs.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Move X86 Per-Function Consumption Behind The View

# Current Packet

## Just Finished

Step 3 (`Move X86 Per-Function Consumption Behind The View`) added a
view-backed `consume_plans(module, PreparedMirFunctionView)` path and migrated
the x86 local-slot return module-emission helper to use it. That migrated path
now takes its function id, control flow, value locations, addressing, stack
layout, and prepared lookup cache from `PreparedMirFunctionView`; frame,
dynamic-stack, call, regalloc, storage, and Route 6 compatibility facts remain
on the legacy `PreparedBirModule` path for later feature-view packets.

## Suggested Next

Start Step 4 by adding the first raw dependency gate for migrated x86 surfaces,
classifying current direct `PreparedBirModule` / broad prealloc include hits as
adapter bridge, compatibility wrapper, or still-unmigrated per-function paths.

## Watchouts

- RV64, AArch64, and x86 per-function helpers still use their legacy
  `PreparedBirModule` paths until their own explicit migration packets.
- The internal x86 module entry now fails closed if a defined function has no
  corresponding core function view.
- The new view-backed consumed-plan overload intentionally keeps feature-like
  frame, call, regalloc, storage, and Route 6 authorities on compatibility
  lookups; do not treat that as a completed feature-view migration.

## Proof

- Required Step 3 proof command ran and wrote `test_after.log`:
  `set -o pipefail; (cmake --build build --target c4c_backend backend_prepared_mir_core_view_test -j && ctest --test-dir build -R '^backend_prepared_mir_core_view$|^backend_x86_shared_producer_query$|^backend_codegen_route_x86_64_.*observe_semantic_bir$' --output-on-failure) > test_after.log 2>&1`
- Required Step 3 proof result: passed, 78/78 tests passed.
