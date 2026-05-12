Status: Active
Source Idea Path: ideas/open/200_hir_legacy_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Fence Metadata-Rich Module And Owner Lookups

# Current Packet

## Just Finished

Completed `plan.md` Step 2 first narrow slice for module function/global
declaration lookup. `Module::resolve_function_decl` and
`Module::resolve_global_decl` now fail closed after any complete structured
declaration-key miss instead of recovering through rendered `fn_index` or
`global_index`. Explicit rendered-only callers still use
`find_*_by_rendered_decl_compatibility_name` and the legacy helper wrappers.

Focused stale rendered-index proof was updated in
`tests/frontend/frontend_hir_lookup_tests.cpp`; the HIR dump structured mirror
case keeps asserting structured/global-id/link-name lookup hits without parity
mismatches.

## Suggested Next

Continue `plan.md` Step 2 with the next owner-lookup fence: reject complete
record-owner metadata misses before falling back to rendered `struct_defs`,
`typespec_legacy_tag`, `callee_name`, or compatibility tags.

## Watchouts

- No-metadata fallback still depends on invalid/incomplete metadata; tests that
  intend rendered-only behavior must not accidentally provide complete
  declaration keys.
- The module declaration fence intentionally leaves LinkNameId and concrete
  GlobalId lookup authoritative before structured declaration-key lookup.
- The HIR dump regex now follows the actual lookup-hit order emitted by the
  dumper while still rejecting parity mismatches.

## Proof

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_hir_lookup_tests|cpp_hir_module_decl_lookup_structured_mirror)$"' > test_after.log 2>&1`

Result: passed. `test_after.log` is the proof log.
