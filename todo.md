Status: Active
Source Idea Path: ideas/open/98_parser_sema_post_cleanup_structured_identity_leftovers.md
Source Plan Path: plan.md
Current Step ID: 3A
Current Step Title: Add AST/Parser Enumerator TextId Metadata

# Current Packet

## Just Finished

Step 3A - Add AST/Parser Enumerator TextId Metadata is complete.

`NK_ENUM_DEF` now exposes `enum_name_text_ids`, a parallel per-enumerator
parser-owned `TextId` array beside the existing rendered `enum_names` and
`enum_vals` arrays. `parse_enum` stores the already-computed `vname_text_id`
for each definition enumerator into that array. Forward enum references and
empty enum definitions remain safe with null arrays or invalid text metadata,
and rendered `enum_names`/`enum_vals` behavior is unchanged.

## Suggested Next

Step 3B sema enum mirror packet: use definition-time `NK_ENUM_DEF`
`enum_name_text_ids` metadata to dual-write enum variant type and const-value
text/key mirrors without changing lookup behavior.

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
- Enum variant mirrors should be populated from definition-time enumerator
  identity, not from lookup-time reference nodes or rendered-string hashing.
- Template NTTP/type-parameter and consteval NTTP mirror work needs a
  parameter-identity strategy first because current AST template parameter
  arrays do not carry `TextId`s.

## Proof

Passed.

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "(enum|frontend_parser_tests|positive_sema_ok_enum_scope_|negative_tests_bad_enum_)"' > test_after.log 2>&1`

Proof log: `test_after.log`. The delegated subset passed 17/17 tests.
