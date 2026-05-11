Status: Active
Source Idea Path: ideas/open/166_hir_rendered_registry_mirror_retirement_audit.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Process Remaining Module and Compile-Time Registries

# Current Packet

## Just Finished

Completed plan Step 4 Module declaration lookup fencing: `Module`
function/global resolution and remaining struct-definition lowering paths now
reject rendered `fn_index`/`global_index` and `struct_defs` fallback when a
complete structured declaration or record-owner key is available but misses.
`Module::struct_defs` remains payload/display/order storage for explicit
compatibility paths without complete owner metadata.

Extended focused HIR lookup coverage for function/global stale-rendered
collisions, stale rendered duplicate struct-definition hits, and stale rendered
base-tag recovery after complete structured-key misses.

## Suggested Next

Supervisor can review and commit this Step 4 Module and struct-definition
registry fencing slice, then decide whether any remaining compile-time registry
handoff is still needed.

## Watchouts

Generated helper names can still reach rendered compatibility when their
TextId does not belong to the structured declaration registry domain; this
keeps existing template-call helper LinkNameId carrier tests passing without
reopening fallback for stale source declaration-key collisions. Struct base
display tags may still copy a real `record_def->name` after a complete owner
miss, but stale rendered `struct_defs` payload/tag recovery is fenced.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|frontend_hir_lookup_tests)$' > test_after.log 2>&1`.

Result: passed. `test_after.log` contains the passing `frontend_hir_tests` and
`frontend_hir_lookup_tests` ctest results.
