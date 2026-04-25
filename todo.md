Status: Active
Source Idea Path: ideas/open/98_parser_sema_post_cleanup_structured_identity_leftovers.md
Source Plan Path: plan.md
Current Step ID: 4A
Current Step Title: Add AST/Parser Template Parameter TextId Metadata

# Current Packet

## Just Finished

Step 3B - Populate Sema Enum Variant Mirrors is complete.

Global and local enum variant binding paths now use definition-time
`NK_ENUM_DEF::enum_name_text_ids` metadata to populate the existing structured
enum type mirrors and enum const-value text/key mirrors when the per-enumerator
`TextId` is valid. Legacy rendered enum type/value maps remain populated as
before, and existing lookup return behavior is unchanged.

## Suggested Next

Step 4A parser/AST template parameter metadata packet: add per-parameter
`TextId` metadata parallel to `Node::template_param_names`, populated from
definition-time parser tokens when available and left invalid for synthetic or
unsupported metadata. Do not add sema mirrors in this packet.

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
- Template NTTP/type-parameter and consteval NTTP mirror work must wait for
  Step 4A because current AST template parameter arrays carry rendered
  `template_param_names` without parallel per-parameter `TextId` metadata.
- Keep Step 4A parser/AST-only: no HIR/type/codegen migration, no template
  rendered-name cleanup, no sema mirror writes, and no expectation rewrites.
- Enum mirror population now depends on parser-owned `enum_name_text_ids`; keep
  any future enum work on that definition-time metadata and do not re-derive
  stable identity from rendered strings.

## Proof

Passed.

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "(enum|frontend_parser_tests|positive_sema_ok_enum_scope_|negative_tests_bad_enum_)"' > test_after.log 2>&1`

Proof log: `test_after.log`. The delegated subset passed 17/17 tests.
