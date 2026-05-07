# Current Packet

Status: Active
Source Idea Path: ideas/open/147_rendered_qualified_compatibility_bridge_removal.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Replace Parser Rendered Qualified `TextId` Handoffs

## Just Finished

Completed Step 2 first parser migration for the qualified template lookup
overload family in `src/frontend/parser/impl/types/template.cpp`:
`find_template_struct_primary(const QualifiedNameRef&)`,
`find_template_struct_specializations(const QualifiedNameRef&, ...)`, and
`find_template_global_primary(const QualifiedNameRef&)` now use the carried
`QualifiedNameRef` metadata for qualified names, including text-id-only
qualification in `qualifier_text_ids`, instead of resolving a context and
feeding a possibly rendered qualified `base_text_id` through
`alias_template_key_in_context()`. `Parser::qualified_name_key()` now sanitizes
stale rendered qualified base/qualifier `TextId`s back to the structured
`base_name` and qualifier segments when projecting a structured key.

Added
`test_parser_qualified_template_lookup_uses_qn_metadata_over_rendered_text_id`
to prove qualified template primary/specialization/global lookups choose
`ns::Alias` from `QualifiedNameRef` metadata even when `base_text_id` renders
as the colliding `other::Alias`, including the case where
`qualifier_segments` is empty and `qualifier_text_ids` carries the owner.
Regression fix: global-qualified structured owners such as `::api::holder`
first try the exact structured key, then fall back to the same absolute owner
path with `is_global_qualified` cleared when matching namespace-context
template registrations. This preserves structured owner/base authority without
returning to rendered qualified `TextId` lookup.

## Suggested Next

Migrate the next parser alias/member caller family that still constructs a
record-member typedef key from owner context plus record `TextId`, likely the
`record_member_typedef_key_in_context()` owner paths in
`src/frontend/parser/impl/types/declarator.cpp` or
`src/frontend/parser/impl/types/base.cpp`, so they can consume a
`QualifiedNameRef`/`QualifiedNameKey` owner directly.

## Watchouts

- Do not treat helper renames or expectation rewrites as bridge-removal
  progress.
- Do not weaken qualified template or HIR tests to make the bridge deletion
  pass.
- The unqualified `QualifiedNameRef` template overload path still delegates to
  `alias_template_key_in_context()` because current-scope and visible lookup
  behavior is intentional there; this packet only migrated qualified-name
  carriers.
- `qualified_key_in_context()` still contains the rendered compatibility branch
  for remaining callers.

## Proof

`cd /workspaces/c4c && { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'frontend_parser_tests|frontend_parser_lookup_authority_tests|cpp_hir_parser_|cpp_positive_sema_.*(template|alias)|cpp_positive_sema_qualified_dependent_typename_global_parse_cpp'; } > test_after.log 2>&1`

Passed. `test_after.log` is the canonical executor proof log. CTest matched
and ran 254 delegated parser/template/regression tests; all passed after the
build completed.
