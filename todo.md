Status: Active
Source Idea Path: ideas/open/98_parser_sema_post_cleanup_structured_identity_leftovers.md
Source Plan Path: plan.md
Current Step ID: 4A
Current Step Title: Add AST/Parser Template Parameter TextId Metadata

# Current Packet

## Just Finished

Step 4A - Add AST/Parser Template Parameter TextId Metadata is complete.

`Node` now carries `template_param_name_text_ids` parallel to
`template_param_names`. Template declaration parsing stores definition-time
parser token `TextId` values for real type, NTTP, constrained, and
template-template parameter names, while anonymous synthetic names keep
`kInvalidText` in the AST metadata slot. Alias-template bookkeeping and
instantiated struct copies preserve the parallel identity array without
changing rendered names, defaults, flags, or template behavior.

## Suggested Next

Step 4B sema NTTP validation mirror packet: consume
`Node::template_param_name_text_ids` to populate sema-side structured mirrors
for template type parameters and NTTP validation, while preserving rendered
template parameter lookup bridges and avoiding HIR/type/codegen identity
changes.

## Watchouts

- Keep this plan limited to parser/sema cleanup; HIR module lookup migration
  belongs to idea 99.
- Preserve rendered-string bridges required by AST, HIR, consteval, diagnostics,
  codegen, and link-name output.
- Do not touch parser struct/tag maps, template rendered names, `TypeSpec::tag`
  outputs, or HIR/type/codegen identity surfaces.
- Do not downgrade expectations or add testcase-shaped exceptions.
- Do not treat the parser `struct_tag_def_map` argument to `eval_const_int` as a
  removable string leftover; it is still the rendered tag bridge used by
  `sizeof`, `alignof`, and `offsetof`.
- Step 4A intentionally did not add sema mirrors; Step 4B should validate
  `template_param_name_text_ids` presence before relying on structured identity
  and keep `kInvalidText` as the fallback for anonymous/synthetic parameters.
- Keep Step 4B sema-only: no HIR/type/codegen migration, no template
  rendered-name cleanup, and no expectation rewrites.
- Enum mirror population now depends on parser-owned `enum_name_text_ids`; keep
  any future enum work on that definition-time metadata and do not re-derive
  stable identity from rendered strings.

## Proof

Passed.

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "(frontend_parser_tests|cpp_parse_template_|cpp_positive_sema_template_.*nttp|cpp_positive_sema_template_type_context_nttp_parse_cpp|cpp_positive_sema_template_variadic_|cpp_positive_sema_consteval_nttp_cpp)"' > test_after.log 2>&1`

Proof log: `test_after.log`. The delegated subset passed 25/25 tests.
