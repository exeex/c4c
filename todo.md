Status: Active
Source Idea Path: ideas/open/198_parser_legacy_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Convert Parser Qualified-Name And Owner Recovery

# Current Packet

## Just Finished

Step 3 started at `src/frontend/parser/impl/types/base.cpp:3141`.
Qualified template member-typedef owner recovery now builds a structured
template-instantiation key and `tpl_struct_args` carrier for the owner, consumes
the structured instantiated record when present, and no longer recovers the
owner through the rendered `struct_tag_def_map.find(mangled)` fallback in this
block.

Added lookup-authority coverage proving `Owner<int>::type` does not recover a
member typedef through a stale rendered mangled owner map entry.

## Suggested Next

Continue Step 3 with the next parser qualified-name/owner recovery surface in
`src/frontend/parser/impl/types/base.cpp`; prefer another narrow stale-rendered
proof before widening beyond parser-owned lookup authority.

## Watchouts

- Keep the work parser-owned; do not expand into Sema, HIR, LIR, BIR, or
  backend cleanup.
- Do not downgrade supported tests or replace semantic fixes with expectation
  rewrites.
- Do not treat parser diagnostics, AST display strings, source spelling, or
  final output text as semantic identity.
- Do not treat a raw `TextId` alone as complete semantic identity across
  scopes.
- Newly retained parser bridges need `legacy` or `deprecated` comments with
  owner, limitation, and removal condition.
- For Step 2, avoid the constant-layout
  `parser_record_layout_compatibility_tag_map` route at first unless the
  packet explicitly targets layout; it is already fenced to direct
  `record_def` plus TextId-less legacy carriers.
- Do not start with `eval_const_int_with_rendered_named_const_compatibility`;
  that route belongs to Step 4 and is a separate named-const surface.
- `owner_mangled` strings are still passed to
  `ensure_template_struct_instantiated_from_args` because that API requires an
  output buffer; they are no longer used for base.cpp owner-definition lookup
  fallback.
- `qualified_template_member_typedef_base` still passes a local `mangled`
  buffer because `ensure_template_struct_instantiated_from_args` requires it;
  this block no longer uses that rendered string for owner recovery.

## Proof

Passed:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_parser_lookup_authority_tests|frontend_parser_tests)$"' > test_after.log 2>&1`

Proof log: `test_after.log`.

Review artifact: `review/step2_parser_tag_map_review.md`.
