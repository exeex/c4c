Status: Active
Source Idea Path: ideas/open/166_hir_rendered_registry_mirror_retirement_audit.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Process Remaining Lowerer Registries

# Current Packet

## Just Finished

Completed plan Step 5 lowerer ref-overload registry fencing:
struct-method ref overload sets now have an owner/method-key mirror used by
member and operator calls before rendered `tag::method` keys, and free-function
ref overload sets now have a declaration-key mirror used before rendered base
names. Complete structured misses fail closed; no-metadata calls still use the
rendered compatibility map. `ref_overload_mangled_` remains a node-identity
reverse map for already-selected overload nodes, not rendered lookup authority.

## Suggested Next

Supervisor can review and commit this Step 5 ref-overload registry fencing
slice, then decide whether any Step 5 lowerer registries remain or hand off for
lifecycle review.

## Watchouts

Structured free-function ref-overload grouping is now per declaration key, so
same-spelled overload candidates in different namespace contexts no longer
share the rendered first-declaration slot. Rendered fallback is intentionally
retained only when the callee lacks complete declaration metadata.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|frontend_hir_lookup_tests)$' > test_after.log 2>&1`.

Result: passed. `test_after.log` contains the passing focused
`frontend_hir_tests` ref-overload owner/declaration-key stale rendered
rejection and no-metadata compatibility coverage plus the passing
`frontend_hir_lookup_tests` subset.
