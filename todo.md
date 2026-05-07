# Current Packet

Status: Active
Source Idea Path: ideas/open/147_rendered_qualified_compatibility_bridge_removal.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Replace Parser Rendered Qualified `TextId` Handoffs

## Just Finished

Completed Step 2 classification/migration for the remaining
`types/struct.cpp` production caller of
`record_member_typedef_key_in_context()`. The record member typedef registration
path now derives its owner `TextId` only from unqualified record metadata and
rejects `::`-bearing `unqualified_text_id`, `unqualified_name`, or
`template_origin_name` spellings instead of passing them into the rendered
qualified compatibility bridge.

## Suggested Next

Have the supervisor run the matching regression guard for this Step 2 slice,
then commit the bounded `types/struct.cpp` and `todo.md` update if the before
and after logs are monotonic.

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
- The direct `record_member_typedef_key_in_context()` production caller still
  exists in `types/struct.cpp`, but it is now classified as a safe unqualified
  registration path because its `record_text_id` source is filtered before the
  call.
- `qualified_key_in_context()` still contains the rendered compatibility branch
  for remaining callers.

## Proof

`cd /workspaces/c4c && { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'frontend_parser_tests|frontend_parser_lookup_authority_tests|cpp_hir_parser_member_typedef|cpp_hir_parser_type_base_|cpp_positive_sema_.*member_typedef|cpp_positive_sema_.*dependent_typename|cpp_positive_sema_.*typedef.*alias'; } > test_after.log 2>&1`

Passed. `test_after.log` is the canonical executor proof log. CTest matched
and ran 106 delegated parser/member-typedef/dependent-typename tests; all
passed after the build completed.
