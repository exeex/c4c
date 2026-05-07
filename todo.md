# Current Packet

Status: Active
Source Idea Path: ideas/open/147_rendered_qualified_compatibility_bridge_removal.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate Qualified Template and HIR Compatibility Paths

## Just Finished

Step 3 closed the remaining `realize_template_struct()` stale rendered-primary
hole from `review/147_step3_route_review.md`. Direct realization now consults
canonical primary metadata before rendered fallback, and the rendered fallback
rejects stale structured origin-key or primary-shaped record-owner mismatches
even when the `TypeSpec` has template arguments.

Added focused HIR coverage for argument-bearing stale rendered primary rejection
after complete structured origin-key miss and structured record-owner miss,
while preserving no-complete-metadata exact rendered compatibility with
arguments.

## Suggested Next

Next packet: Step 3 appears ready for supervisor review/transition toward Step
4 bridge deletion/isolation.

## Watchouts

- Do not remove `qualified_key_in_context()` bridges before the reachable HIR
  semantic fallbacks are migrated; that is Step 4 work.
- The retained rendered split is intentionally limited to no-complete-metadata
  compatibility, such as a single rendered `LegacyOwner::value_type` TextId
  without separate qualifier/member metadata.
- The retained return/parameter rendered owner fallback is intentionally limited
  to owner TypeSpecs without complete structured owner metadata, such as a
  tag-only legacy owner TextId.
- Argument-bearing `realize_template_struct()` still allows intended primary
  recovery when rendered primary text metadata matches the structured origin; do
  not broaden stale-primary rejection into normal pending template-owner
  realization.
- Avoid any fix that rewrites expectations or strips qualified strings to an
  unqualified suffix. The point of Step 3 is structured owner/name metadata, not
  a new rendered-spelling path.

## Proof

Delegated proof command ran exactly:

`cd /workspaces/c4c && { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'cpp_hir_.*template|cpp_hir_.*qualified|cpp_hir_.*member_typedef|frontend_hir_tests|cpp_positive_sema_.*(template|alias|dependent_typename)'; } > test_after.log 2>&1`

Result: passed. `test_after.log` records 354/354 tests passing.
