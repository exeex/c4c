# Current Packet

Status: Active
Source Idea Path: ideas/open/147_rendered_qualified_compatibility_bridge_removal.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate Qualified Template and HIR Compatibility Paths

## Just Finished

Step 3 fenced the remaining signature return/parameter `::type`
member-typedef compatibility blocks below `substitute_signature_template_type()`.
Complete structured owner/member metadata now attempts HIR owner-key lookup and,
when that lookup misses, avoids the rendered `Owner::type` compatibility
fallback.

Added focused `frontend_hir_lookup_tests` coverage for return and parameter
complete owner/member metadata miss rejection against stale rendered
member-typedef entries, plus retained no-complete-metadata rendered owner
compatibility for both paths.

## Suggested Next

Next packet: have the supervisor decide whether Step 3 is complete enough for
review or whether another narrow HIR signature compatibility path remains
before moving to Step 4.

## Watchouts

- Do not remove `qualified_key_in_context()` bridges before the reachable HIR
  semantic fallbacks are migrated; that is Step 4 work.
- The retained rendered split is intentionally limited to no-complete-metadata
  compatibility, such as a single rendered `LegacyOwner::value_type` TextId
  without separate qualifier/member metadata.
- The retained return/parameter rendered owner fallback is intentionally limited
  to owner TypeSpecs without complete structured owner metadata, such as a
  tag-only legacy owner TextId.
- Avoid any fix that rewrites expectations or strips qualified strings to an
  unqualified suffix. The point of Step 3 is structured owner/name metadata, not
  a new rendered-spelling path.

## Proof

Delegated proof command ran exactly:

`cd /workspaces/c4c && { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'cpp_hir_.*template|cpp_hir_.*qualified|cpp_hir_.*member_typedef|frontend_hir_tests|cpp_positive_sema_.*(template|alias|dependent_typename)'; } > test_after.log 2>&1`

Result: passed. `test_after.log` records 354/354 tests passing.
