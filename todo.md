Status: Active
Source Idea Path: ideas/open/124_hir_legacy_string_lookup_removal_convergence.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Extend Structured Lookup Proof Across HIR Edge Paths

# Current Packet

## Just Finished

Completed Step 4 struct owner-key proof extension. `frontend_hir_lookup_tests`
now covers stale rendered struct member, method mangled-name, method link-name,
and method return-type lookup maps disagreeing with the structured owner-key
mirrors, and asserts those owner-key parity paths record mismatches instead of
letting rendered spelling silently count as authoritative.

## Suggested Next

Next coherent Step 4 packet: extend `frontend_hir_lookup_tests` to cover the
compile-time registry edge paths that already have structured mirrors. Build
stale-rendered mismatch tests for `CompileTimeState::find_template_def`,
`find_template_struct_def`, `find_template_struct_specializations(primary_def,
rendered_name)`, and `find_consteval_def`, proving declaration-key lookup wins
over a conflicting rendered-name entry and rendered fallback remains available
only when declaration metadata is absent.

## Watchouts

The new test deliberately uses the existing owner-key parity mirrors and does
not change production lookup authority; it proves detection rather than a full
owner-key replacement. Link-name and return-type assertions still expect the
rendered lookup result while requiring the structured owner-key parity mismatch
to be counted.

## Proof

Supervisor-selected proof ran exactly:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_hir_lookup_tests$' > test_after.log 2>&1
```

Result: passed. `test_after.log` contains `frontend_hir_lookup_tests`: 1 passed
/ 0 failed after rebuilding `frontend_hir_lookup_tests`.
