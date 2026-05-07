# Current Packet

Status: Active
Source Idea Path: ideas/open/147_rendered_qualified_compatibility_bridge_removal.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate Qualified Template and HIR Compatibility Paths

## Just Finished

Step 3 migrated the first HIR template primary/specialization authority path.
`collect_initial_type_definitions()` now forms a structured owner key for
specialization registration from the specialization node namespace plus
unqualified `template_origin_name`, consults `template_struct_defs_by_owner_`,
and only falls back to rendered primary lookup when that structured key is not
available. The old recovery path that rebuilt `ns::Wrapper` by splitting
rendered `ns::Wrapper_T...` spelling is gone from this registration path.

Added a focused stale-qualified-origin test in
`cpp_hir_template_canonical_primary_origin_metadata_test.cpp`: a rendered-only
`ns::Wrapper` primary with no owner key must not receive a specialization whose
structured owner key points at namespace-context `Wrapper`.

## Suggested Next

Next Step 3 packet: migrate the remaining `canonical_template_struct_primary()`
rendered family-root fallback in `src/frontend/hir/impl/templates/templates.cpp`
so `_T` suffix and qualified-scope splitting are only legacy no-metadata
compatibility behavior, not semantic authority when structured origin or
record-def owner metadata is present.

## Watchouts

- Do not remove `qualified_key_in_context()` bridges before the reachable HIR
  semantic fallbacks are migrated; that is Step 4 work.
- `hir_functions.cpp` contains multiple member-typedef compatibility fallbacks;
  keep them for a later packet after template primary/specialization owner-key
  routing is proven.
- This slice intentionally leaves string-key template primary maps and parity
  probes in place; Step 4 owns deletion/isolation after remaining HIR callers
  are migrated.
- Avoid any fix that rewrites expectations or strips qualified strings to an
  unqualified suffix. The point of Step 3 is structured owner/name metadata, not
  a new rendered-spelling path.

## Proof

Delegated proof command ran exactly:

`cd /workspaces/c4c && { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'cpp_hir_.*template|cpp_hir_.*qualified|cpp_hir_.*member_typedef|frontend_hir_tests|cpp_positive_sema_.*(template|alias|dependent_typename)'; } > test_after.log 2>&1`

Result: passed, 354/354 tests. Proof log: `test_after.log`.
