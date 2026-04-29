Status: Active
Source Idea Path: ideas/open/124_hir_legacy_string_lookup_removal_convergence.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Extend Structured Lookup Proof Across HIR Edge Paths

# Current Packet

## Just Finished

Completed Step 4 compile-time registry lookup proof extension.
`frontend_hir_lookup_tests` now covers stale rendered-name conflicts for
`CompileTimeState::find_template_def`, `find_template_struct_def`,
`find_template_struct_specializations(primary_def, rendered_name)`, and
`find_consteval_def`, and asserts declaration-key mirrors win while rendered
fallback remains available when the declaration key has no structured entry.

## Suggested Next

Next coherent Step 4 packet: have the supervisor review whether the HIR lookup
edge-path proof set is now broad enough for Step 4 acceptance, or select the
next uncovered structured lookup family if one remains in the active plan.

## Watchouts

This packet is test-only and exercises existing `CompileTimeState` precedence;
it does not change production lookup authority. The fallback assertions use
unregistered declaration keys to prove rendered-name compatibility stays
available when no structured entry exists.

## Proof

Supervisor-selected proof ran exactly:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_hir_lookup_tests$' > test_after.log 2>&1
```

Result: passed. `test_after.log` contains `frontend_hir_lookup_tests`: 1 passed
/ 0 failed after rebuilding `frontend_hir_lookup_tests`.
