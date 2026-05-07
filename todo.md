# Current Packet

Status: Active
Source Idea Path: ideas/open/147_rendered_qualified_compatibility_bridge_removal.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Replace Parser Rendered Qualified `TextId` Handoffs

## Just Finished

Completed Step 2 migration for the declarator qualified-typename
record-member typedef fallback in
`src/frontend/parser/impl/types/declarator.cpp`. The fallback now projects the
record-member typedef key from the parsed owner `QualifiedNameRef` via
`qualified_name_key(owner_qn)` and the new
`record_member_typedef_key_from_owner_key()` helper, instead of resolving an
owner namespace context and calling `record_member_typedef_key_in_context()`
with `owner_qn.base_text_id`.

Added
`test_parser_record_member_typedef_key_uses_structured_owner_metadata` to prove
member typedef key construction resolves `ns::Owner::Member` from structured
owner metadata even when the owner `base_text_id` renders as the colliding
`other::Owner`, and that the stale rendered owner key does not recover the
binding.

## Suggested Next

Migrate the `types/base.cpp` record-member typedef sidecar family:
`record_member_key_for_node()` still derives a context and record `TextId` from
`Node` fields before calling `record_member_typedef_key_in_context()`. Thread or
construct a structured owner `QualifiedNameKey` there and use
`record_member_typedef_key_from_owner_key()` or equivalent direct metadata.

## Watchouts

- Do not treat helper renames or expectation rewrites as bridge-removal
  progress.
- Do not weaken qualified template or HIR tests to make the bridge deletion
  pass.
- The new helper is a projection from an already structured owner key; do not
  use it to paper over missing owner metadata by first reconstructing the owner
  from rendered spelling.
- The direct `record_member_typedef_key_in_context()` production caller left in
  owned scope is now the `types/base.cpp` sidecar path.
- `qualified_key_in_context()` still contains the rendered compatibility branch
  for remaining callers.

## Proof

`cd /workspaces/c4c && { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'frontend_parser_tests|frontend_parser_lookup_authority_tests|cpp_hir_parser_member_typedef|cpp_hir_parser_type_base_|cpp_positive_sema_.*member_typedef|cpp_positive_sema_.*dependent_typename|cpp_positive_sema_.*typedef.*alias'; } > test_after.log 2>&1`

Passed. `test_after.log` is the canonical executor proof log. CTest matched
and ran 106 delegated parser/member-typedef/dependent-typename tests; all
passed after the build completed.
