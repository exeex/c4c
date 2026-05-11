Status: Active
Source Idea Path: ideas/open/166_hir_rendered_registry_mirror_retirement_audit.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Process Remaining Lowerer Registries

# Current Packet

## Just Finished

Completed plan Step 5 lowerer struct-method lookup fencing:
`find_struct_method_mangled`, `find_struct_method_link_name_id`, and
`find_struct_method_return_type` now stop before rendered method maps when a
complete owner/method key is available and misses. No-metadata rendered
compatibility remains available, and complete-key misses still continue into
structured base lookup.

## Suggested Next

Supervisor can review and commit this Step 5 struct-method lookup fencing
slice, then choose the next remaining lowerer registry packet or lifecycle
handoff.

## Watchouts

The helper-local rendered-fallback gate is intentionally per queried tag. A
derived complete-key miss suppresses the derived rendered maps but still walks
`base_tags`, allowing the base lookup to make its own structured/no-metadata
decision.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|frontend_hir_lookup_tests)$' > test_after.log 2>&1`.

Result: passed. `test_after.log` contains the passing focused
`frontend_hir_tests` method lookup stale rendered rejection/base fallback
coverage and the passing `frontend_hir_lookup_tests` subset.
