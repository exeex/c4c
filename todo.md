Status: Active
Source Idea Path: ideas/open/198_parser_legacy_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Fence Record Type And Layout Tag Map Compatibility

# Current Packet

## Just Finished

Fenced the remaining template static-member owner-resolution callers in
`src/frontend/parser/impl/types/base.cpp`. Both metadata-rich owner
instantiation paths now use
`parse_base_type_static_member_base_record_definition`, so complete structured
misses do not re-enter `struct_tag_def_map`; the old unconditional
`owner_mangled` rendered-map retry was removed.

No test expectations were changed. Existing focused stale-rendered coverage in
`tests/frontend/frontend_parser_lookup_authority_tests.cpp` was preserved by
the delegated proof.

## Suggested Next

Review Step 2 for completeness against the source idea. The only remaining
`resolve_record_type_spec_with_parser_tag_map_compatibility` call in
`src/frontend/parser/impl/types/base.cpp` is the helper-local TextId-less,
context-less legacy compatibility tail.

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

## Proof

Passed:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_parser_lookup_authority_tests|frontend_parser_tests)$"' > test_after.log 2>&1`

Proof log: `test_after.log`.
