Status: Active
Source Idea Path: ideas/open/198_parser_legacy_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Convert Parser Qualified-Name And Owner Recovery
你該做test baseline review了

# Current Packet

## Just Finished

Step 3 post-commit regression from `e7b807a28` is fixed for
`cpp_parser_debug_qualified_type_spelling_stack`.

Updated the parser-debug diagnostic expectation to the new qualified-owner
route: the stack now records the outer `try_parse_cpp_scoped_base_type` /
`try_parse_qualified_base_type` frames before entering the nested template
argument path. This keeps the compatibility proof meaningful because it
asserts the stronger structured qualified-owner route that replaced rendered
`struct_tag_def_map` owner recovery, while preserving the final committed
`parse_top_level_parameter_list` failure and nested template-argument frames.

## Suggested Next

Continue Step 3 with the next parser qualified-name/owner recovery surface, or
hand this regression slice back to the supervisor for commit after review of
the hook-added baseline reminder.

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
- The diagnostic expectation change is not a weakened parser compatibility
  proof: it requires additional structured qualified-owner frames introduced by
  the new route instead of accepting a shorter rendered-spelling stack.

## Proof

Passed:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_parser_lookup_authority_tests|frontend_parser_tests|cpp_parser_debug_qualified_type_spelling_stack)$"' > test_after.log 2>&1`

Proof log: `test_after.log`.

Review artifact: `review/step2_parser_tag_map_review.md`.
