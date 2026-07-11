Status: Active
Source Idea Path: ideas/open/684_bir_mir_interface_cleanup_from_prepared_mir_view_docs.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add First Raw Dependency Gate For Migrated X86 Surfaces

# Current Packet

## Just Finished

Step 4 (`Add First Raw Dependency Gate For Migrated X86 Surfaces`) added
`scripts/x86_raw_prepared_dependency_guard.py`, a lightweight scanner for
`src/backend/mir/x86/` raw `PreparedBirModule` and broad prealloc include
dependencies. The gate classifies current hits as adapter bridge,
compatibility wrapper, tracked legacy surface, or diagnostic-only rendering,
prints category counts and representative paths, and fails if a new
unclassified raw dependency appears or an existing classified path's hit count
drifts from its reviewed expectation.

## Suggested Next

Ask the plan owner to decide whether this active runbook is complete, should
close, or needs a follow-up route for additional x86 feature-view/raw
dependency cleanup.

## Watchouts

- The gate is intentionally visibility-first for the current partial
  migration; it does not require full x86 raw-dependency cleanup yet.
- New raw `PreparedBirModule` or broad prealloc hits under x86 MIR must be
  explicitly classified before the gate will pass.

## Proof

- Raw dependency gate passed:
  `python3 scripts/x86_raw_prepared_dependency_guard.py`
- Gate result: passed with 103 classified hits: 3 adapter bridge, 7
  compatibility wrapper, 80 tracked legacy surface, and 13 diagnostic-only
  rendering; all classified path hit counts matched expected counts.
- Required Step 4 behavior proof command ran and wrote `test_after.log`:
  `set -o pipefail; (cmake --build build --target c4c_backend backend_prepared_mir_core_view_test -j && ctest --test-dir build -R '^backend_prepared_mir_core_view$|^backend_x86_shared_producer_query$|^backend_codegen_route_x86_64_.*observe_semantic_bir$' --output-on-failure) > test_after.log 2>&1`
- Required Step 4 behavior proof result: passed, 78/78 tests passed.
