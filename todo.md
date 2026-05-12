Status: Active
Source Idea Path: ideas/open/198_parser_legacy_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Closure Ledger And Broader Parser Proof

# Current Packet

## Just Finished

Step 5 completed the parser-owned template instantiated record lookup audit.

The route now materializes pending base records through structured template
instantiation keys and parsed argument metadata instead of starting from
rendered `base_mangled` tag-map lookup. Direct template-emission reuse now
distinguishes a complete structured direct-emission instantiation key from a
key that has merely been marked, so stale rendered
`definition_state_.struct_tag_def_map[mangled]` reuse is permitted only when
there is no complete structured instantiation key.

Reviewer report `review/step5_template_instantiation_lookup_review.md` found
no blocking issues, no testcase-overfit, and recommended advancing to Step 6.
The accepted full-suite baseline for commit `a654583f2` was green: 3137
passed, 0 failed, with the existing 12 disabled backend CLI tests.

## Suggested Next

Execute Step 6 by writing a reviewer-auditable closure ledger in `todo.md` for
parser legacy compatibility retirement.

The ledger must explicitly classify parser compatibility routes as deleted,
converted, fenced, or intentionally retained. It must cover parser record,
layout, qualified-name, owner, const-int, and template instantiated record
routes, and it must include the reviewer watch items:

- retained no-carrier rendered map fallbacks around base materialization and
  direct-emission reuse
- retained member typedef owner fallback gated by
  `parse_base_type_legacy_tag_if_no_metadata(ts)`
- any retained no-metadata/template-instantiation fallbacks from the Step 3
  owner-recovery review

State follow-up work outside the parser domain as a separate open-idea
candidate rather than expanding this parser-owned runbook. After the ledger is
complete, run the supervisor-selected broader parser/frontend validation for
the final compatibility retirement slice.

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
- Step 6 is a closure-ledger and proof step. Do not perform implementation
  cleanup unless the supervisor delegates a separate executor packet for a
  specific blocker found while building the ledger.
- The ledger must not hide retained parser bridges. Each retained compatibility
  path needs owner, limitation, and removal condition.
- Existing HIR/Sema template callers may still lack parser structured carriers.
  Preserve those as explicit legacy/no-metadata boundaries unless parser-owned
  metadata is proven complete.
- Do not treat `template_origin_name`, rendered candidate keys, parser
  diagnostics, AST display strings, source spelling, or final output text as
  semantic authority.
- Do not create backend, HIR, LIR, BIR, or Sema cleanup inside this plan. If the
  closure ledger finds non-parser residual work, record it as a separate
  follow-up idea candidate.

## Proof

Step 5 focused proof passed:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_parser_tests|frontend_parser_lookup_authority_tests)$"' > test_after.log 2>&1`

Proof log: `test_after.log`.

Step 5 review: `review/step5_template_instantiation_lookup_review.md`, no
blockers.

Accepted full-suite baseline for commit `a654583f2`: 3137 passed, 0 failed,
existing 12 disabled backend CLI tests.
