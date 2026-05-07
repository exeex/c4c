# Current Packet

Status: Active
Source Idea Path: ideas/open/147_rendered_qualified_compatibility_bridge_removal.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate Qualified Template and HIR Compatibility Paths

## Just Finished

Step 3 fenced `substitute_signature_template_type()` qualified member-typedef
compatibility. Complete structured owner/member metadata now resolves through
the owner-key path and, when that lookup misses, returns the unresolved
`TypeSpec` instead of recovering by splitting rendered `Owner::member` text.

Added focused `frontend_hir_lookup_tests` coverage for complete owner/member
metadata miss rejection against a stale rendered member-typedef entry and for
retained no-complete-metadata rendered split compatibility.

## Suggested Next

Next Step 3 packet: migrate the remaining signature return/parameter
member-typedef compatibility blocks in `resolve_signature_*_type_if_needed()`
so complete structured owner/member metadata misses fail closed before the
`member_typedef_compatibility_name(..., "type")` rendered fallback.

## Watchouts

- Do not remove `qualified_key_in_context()` bridges before the reachable HIR
  semantic fallbacks are migrated; that is Step 4 work.
- `hir_functions.cpp` still has return/parameter member-typedef compatibility
  fallbacks below `substitute_signature_template_type()`; this packet only
  fences the direct qualified typedef substitution path.
- The retained rendered split is intentionally limited to no-complete-metadata
  compatibility, such as a single rendered `LegacyOwner::value_type` TextId
  without separate qualifier/member metadata.
- Avoid any fix that rewrites expectations or strips qualified strings to an
  unqualified suffix. The point of Step 3 is structured owner/name metadata, not
  a new rendered-spelling path.

## Proof

Delegated proof command ran exactly:

`cd /workspaces/c4c && { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'cpp_hir_.*template|cpp_hir_.*qualified|cpp_hir_.*member_typedef|frontend_hir_tests|cpp_positive_sema_.*(template|alias|dependent_typename)'; } > test_after.log 2>&1`

Result: passed. `test_after.log` records 354/354 tests passing.
