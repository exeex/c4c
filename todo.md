# Current Packet

Status: Active
Source Idea Path: ideas/open/147_rendered_qualified_compatibility_bridge_removal.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Replace Parser Rendered Qualified `TextId` Handoffs

## Just Finished

Corrected the Step 2 `types/base.cpp` sidecar migration so
`record_member_key_for_node()` rejects qualified rendered spelling instead of
recovering a semantic owner base by splitting on `::`. It now projects member
typedef keys only from unqualified owner metadata or an already-structured
qualified owner key.

Adjusted
`test_alias_member_typedef_nttp_substitution_uses_text_id_over_stale_name` so
the sidecar fixture keeps stale rendered text in the display `name` field while
using the unqualified `template_origin_name` metadata that the sidecar is
allowed to trust.

## Suggested Next

Continue Step 2 by auditing the remaining parser production caller in
`src/frontend/parser/impl/types/struct.cpp`, then migrate or classify that path
before Step 3 removes the rendered-qualified compatibility branch from
`qualified_key_in_context()`.

## Watchouts

- Do not treat helper renames or expectation rewrites as bridge-removal
  progress.
- Do not weaken qualified template or HIR tests to make the bridge deletion
  pass.
- The new helper is a projection from an already structured owner key; do not
  use it to paper over missing owner metadata by first reconstructing the owner
  from rendered spelling.
- Do not reintroduce suffix splitting of `template_origin_name`, `name`, or
  other rendered record spellings; qualified rendered origins without structured
  owner metadata should fail to form a sidecar key.
- The direct `record_member_typedef_key_in_context()` production caller left by
  `rg` is now in `types/struct.cpp`; `types/base.cpp` no longer calls it.
- `qualified_key_in_context()` still contains the rendered compatibility branch
  for remaining callers.

## Proof

`cd /workspaces/c4c && { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'frontend_parser_tests|frontend_parser_lookup_authority_tests|cpp_hir_parser_member_typedef|cpp_hir_parser_type_base_|cpp_positive_sema_.*member_typedef|cpp_positive_sema_.*dependent_typename|cpp_positive_sema_.*typedef.*alias'; } > test_after.log 2>&1`

Passed. `test_after.log` is the canonical executor proof log. CTest matched
and ran 106 delegated parser/member-typedef/dependent-typename tests; all
passed after the build completed.
