# Current Packet

Status: Active
Source Idea Path: ideas/open/147_rendered_qualified_compatibility_bridge_removal.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Replace Parser Rendered Qualified `TextId` Handoffs

## Just Finished

Completed Step 2 helper-boundary fencing for
`Parser::alias_template_key_in_context()` and
`Parser::record_member_typedef_key_in_context()`. The alias-template lookup and
registration caller family in `types/template.cpp` now preserves structured
`QualifiedNameRef`/`QualifiedNameKey` authority: resolved visible names continue
through `VisibleNameResult::key`, qualified-name lookups try the structured
namespace-context registration key, and template registration overloads use
node-owned unqualified metadata only when their delegated `TextId` is rendered
qualified.

## Suggested Next

Have the supervisor run the matching regression guard for this Step 2 slice,
then commit the bounded helper-fence, template lookup/registration migration,
focused parser tests, and `todo.md` update if the before and after logs are
monotonic.

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
- The direct `record_member_typedef_key_in_context()` production caller in
  `types/struct.cpp` is classified as a safe unqualified registration path
  because its `record_text_id` source is filtered before the call.
- Template specialization registration must keep an already-unqualified primary
  name authoritative; node-owned unqualified metadata is only a fallback for
  rendered or invalid delegated names.
- `qualified_key_in_context()` still contains the rendered compatibility branch
  for caller families outside this helper-fence slice.

## Proof

`cd /workspaces/c4c && { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'frontend_parser_tests|frontend_parser_lookup_authority_tests|cpp_hir_parser_|cpp_positive_sema_.*(template|alias|member_typedef|dependent_typename)'; } > test_after.log 2>&1`

Passed. `test_after.log` is the canonical executor proof log. CTest matched
and ran 328 delegated parser/HIR/template/member-typedef/dependent-typename
tests; all passed after the build completed.
