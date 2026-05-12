Status: Active
Source Idea Path: ideas/open/198_parser_legacy_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Fence Record Type And Layout Tag Map Compatibility

# Current Packet

## Just Finished

Step 2 - Fence Record Type And Layout Tag Map Compatibility started.

Fenced the `parse_base_type` static-member base traversal in
`src/frontend/parser/impl/types/base.cpp`. The duplicated base traversal now
uses `parse_base_type_static_member_base_record_definition`, which accepts a
complete direct `record_def`, searches complete `struct_defs` by structured
record metadata, and fails closed when that metadata misses. The retained
`struct_tag_def_map` compatibility tail is explicitly limited to TextId-less,
context-less legacy carriers.

No test expectations were changed. Existing focused stale-rendered coverage in
`tests/frontend/frontend_parser_lookup_authority_tests.cpp` was preserved by
the delegated proof.

## Suggested Next

Continue Step 2 by fencing the remaining metadata-rich
`resolve_record_type_spec_with_parser_tag_map_compatibility` owner-resolution
callers in `src/frontend/parser/impl/types/base.cpp`, preserving any
TextId-less legacy rendered compatibility as an explicit tail.

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
- The new `parse_base_type_static_member_base_record_definition` helper mirrors
  the existing `template.cpp` base-record fence; consider consolidating only if
  a later packet owns the shared helper boundary.

## Proof

Passed:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_parser_lookup_authority_tests|frontend_parser_tests)$"' > test_after.log 2>&1`

Proof log: `test_after.log`.
