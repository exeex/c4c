# Current Packet

Status: Active
Source Idea Path: ideas/open/147_rendered_qualified_compatibility_bridge_removal.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate Qualified Template and HIR Compatibility Paths

## Just Finished

Step 3 migrated the remaining HIR canonical template primary family-root
fallback in `canonical_template_struct_primary()`. Structured
`tpl_struct_origin_key` metadata now blocks the rendered `_T` family-root
recovery after an origin-key miss, while complete `record_def` owner metadata
continues to block rendered recovery after a record-owner miss. The legacy
qualified `_T` family-root split remains available only for no-metadata
compatibility.

Added focused stale-rendered tests in
`cpp_hir_template_canonical_primary_origin_metadata_test.cpp` for origin
metadata, record-owner metadata, and the retained no-metadata compatibility
path.

## Suggested Next

Next Step 3 packet: inspect remaining HIR compatibility paths that still call
template primary lookup from rendered strings, especially
`instantiate_template_struct()` and nearby nested-template origin preservation,
and migrate any reachable stale-rendered authority to structured owner/name
metadata without deleting Step 4 bridge maps yet.

## Watchouts

- Do not remove `qualified_key_in_context()` bridges before the reachable HIR
  semantic fallbacks are migrated; that is Step 4 work.
- `hir_functions.cpp` contains multiple member-typedef compatibility fallbacks;
  keep them for a later packet after template primary/specialization owner-key
  routing is proven.
- This slice intentionally leaves exact rendered template primary lookup,
  string-key template primary maps, and parity probes in place; Step 4 owns
  deletion/isolation after remaining HIR callers are migrated.
- Avoid any fix that rewrites expectations or strips qualified strings to an
  unqualified suffix. The point of Step 3 is structured owner/name metadata, not
  a new rendered-spelling path.

## Proof

Delegated proof command ran exactly:

`cd /workspaces/c4c && { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'cpp_hir_.*template|cpp_hir_.*qualified|cpp_hir_.*member_typedef|frontend_hir_tests|cpp_positive_sema_.*(template|alias|dependent_typename)'; } > test_after.log 2>&1`

Result: passed, 354/354 tests. Proof log: `test_after.log`.
