Status: Active
Source Idea Path: ideas/open/198_parser_legacy_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Convert Parser Qualified-Name And Owner Recovery

# Current Packet

## Just Finished

Step 2 was accepted by `review/step2_parser_tag_map_review.md` with no
blocking findings. The review judged the record/layout tag-map fencing
route-aligned, narrow proof sufficient, and complete enough to advance.

Accepted Step 2 state: metadata-rich static-member base and owner record routes
in `src/frontend/parser/impl/types/base.cpp` now use structured
`record_def`/`TextId`/context/qualifier carrier lookup, while TextId-less,
context-less helper-local tails remain explicit legacy compatibility. No test
expectations were changed.

## Suggested Next

Start Step 3 at `src/frontend/parser/impl/types/base.cpp:3141`: the review
identified this as a non-blocking owner/qualified-name watch where a qualified
template member-typedef owner can still recover through
`definition_state_.struct_tag_def_map.find(mangled)` when
`ensure_template_struct_instantiated_from_args` does not return
`resolved_owner.record_def`.

Convert that route to consume structured owner or `record_def` carriers where
complete, or fence the fallback as parser-local compatibility with owner,
limitation, and removal condition.

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
- Treat `src/frontend/parser/impl/types/base.cpp:3141` as the first Step 3
  target/watch; it is owner/qualified-name recovery scope, not a Step 2
  record/layout blocker.

## Proof

Passed:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_parser_lookup_authority_tests|frontend_parser_tests)$"' > test_after.log 2>&1`

Proof log: `test_after.log`.

Review artifact: `review/step2_parser_tag_map_review.md`.
