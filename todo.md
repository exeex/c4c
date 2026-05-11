Status: Active
Source Idea Path: ideas/open/166_hir_rendered_registry_mirror_retirement_audit.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Process Remaining Lowerer Registries

# Current Packet

## Just Finished

Completed plan Step 5 lowerer constructor/destructor lookup fencing:
constructor and destructor map lookups now route through a shared owner-tag
resolver that uses complete structured record metadata first and fails closed
on complete structured misses instead of reopening rendered string maps.
Rendered compatibility remains available when the TypeSpec has no structured
owner carrier.

## Suggested Next

Supervisor can review and commit this Step 5 constructor/destructor lookup
fencing slice, then choose the next remaining lowerer registry packet or
lifecycle handoff.

## Watchouts

The new lowerer registry owner resolver intentionally treats `record_def`
carriers as structured identity. If a `record_def` can form a complete owner
key but the module has no matching structured record, constructor/destructor
lookup stops there; no-metadata TypeSpecs still use rendered tag
compatibility.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|frontend_hir_lookup_tests)$' > test_after.log 2>&1`.

Result: passed. `test_after.log` contains the passing focused
`frontend_hir_tests` constructor/destructor stale rendered rejection and
no-metadata compatibility coverage plus the passing `frontend_hir_lookup_tests`
subset.
