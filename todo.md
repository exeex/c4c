Status: Active
Source Idea Path: ideas/open/198_parser_legacy_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Convert Parser Qualified-Name And Owner Recovery

# Current Packet

## Just Finished

Step 3 fenced the expression-construction route for
`Template<Args>::member` in `src/frontend/parser/impl/expressions.cpp`.

The parser now marks a template owner as requiring structured metadata before
owner recovery when the qualified owner has a TextId or any parsed template
argument carries structured identity. If complete owner construction still
misses, the expression route no longer falls through to rendered owner segment
splitting. The only remaining rendered segment split is documented as
legacy/no-metadata compatibility with owner, limitation, and removal condition.

## Suggested Next

Continue Step 3 with the next parser qualified-name/owner recovery surface, or
hand this expression-owner fence slice back to the supervisor for review and
commit.

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
- The expression template-owner route still permits the structured
  `qn.base_text_id` owner qualifier path when an instantiated owner TextId is
  unavailable; the fenced fallback is only the rendered `owner_name` segment
  split for no-metadata callers.
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
- The diagnostic expectation change is not a weakened parser compatibility
  proof: it requires additional structured qualified-owner frames introduced by
  the new route instead of accepting a shorter rendered-spelling stack.

## Proof

Passed:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_parser_lookup_authority_tests|frontend_parser_tests|cpp_parser_debug_qualified_type_spelling_stack)$"' > test_after.log 2>&1`

Proof log: `test_after.log`.
