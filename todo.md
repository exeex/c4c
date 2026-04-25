Status: Active
Source Idea Path: ideas/open/98_parser_sema_post_cleanup_structured_identity_leftovers.md
Source Plan Path: plan.md
Current Step ID: 3B
Current Step Title: Populate Sema Enum Variant Mirrors

# Current Packet

## Just Finished

Step 3B - Populate Sema Enum Variant Mirrors is complete.

Global and local enum variant binding paths now use definition-time
`NK_ENUM_DEF::enum_name_text_ids` metadata to populate the existing structured
enum type mirrors and enum const-value text/key mirrors when the per-enumerator
`TextId` is valid. Legacy rendered enum type/value maps remain populated as
before, and existing lookup return behavior is unchanged.

## Suggested Next

Step 4 template NTTP validation binding packet: add validation-time structured
mirrors for template NTTP bindings using a parameter identity strategy, without
changing HIR/type/codegen identity or rewriting template expectations.

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
- Template NTTP/type-parameter and consteval NTTP mirror work needs a
  parameter-identity strategy first because current AST template parameter
  arrays do not carry `TextId`s.
- Enum mirror population now depends on parser-owned `enum_name_text_ids`; keep
  any future enum work on that definition-time metadata and do not re-derive
  stable identity from rendered strings.

## Proof

Passed.

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "(enum|frontend_parser_tests|positive_sema_ok_enum_scope_|negative_tests_bad_enum_)"' > test_after.log 2>&1`

Proof log: `test_after.log`. The delegated subset passed 17/17 tests.
