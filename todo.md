Status: Active
Source Idea Path: ideas/open/198_parser_legacy_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Retire Rendered Named-Const Compatibility

# Current Packet

## Just Finished

Step 3 is complete. The qualified template member typedef route now resolves
owner records through structured template instance keys and direct
`record_def` carriers instead of retrying `struct_tag_def_map` with rendered
owner spelling after a complete structured miss.

The expression-construction route for `Template<Args>::member` now marks an
owner as requiring structured metadata when the qualified owner has a TextId or
any parsed template argument carries structured identity. If complete owner
construction still misses, the expression route no longer falls through to
rendered owner segment splitting. The retained rendered segment split is
documented as legacy/no-metadata compatibility with owner, limitation, and
removal condition.

Reviewer report `review/step3_parser_owner_recovery_review.md` found no
blocking issues, no testcase-overfit, and recommended advancing to Step 4. It
kept retained no-metadata/template-instantiation fallbacks as a Step 5/Step 6
watch item, not a Step 3 blocker.

## Suggested Next

Begin Step 4 by inspecting
`eval_const_int_with_rendered_named_const_compatibility` and nearby
parser-facing named-constant lookup or consteval handoff surfaces. Identify
which callers already have structured named-constant metadata or
domain-scoped TextId carriers, then choose one narrow stale-rendered
named-const proof before changing behavior.

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
- Step 4 owns named-const identity only. Do not drift back into record layout,
  owner recovery, template instantiated record lookup, or backend consteval
  work unless a separate lifecycle packet says so.
- The target helper is explicitly named in the runbook:
  `eval_const_int_with_rendered_named_const_compatibility`.
- Do not treat rendered const names, AST display names, diagnostics, or source
  spelling as semantic identity when structured named-constant metadata is
  complete.
- A retained rendered named-const path must be fenced as no-metadata
  compatibility or deleted when no production caller still needs it.
- Step 3 reviewer watch item: retained no-metadata/template-instantiation
  fallbacks in `base.cpp` should be accounted for in Step 5 and the Step 6
  closure ledger.

## Proof

Step 3 proof passed:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_parser_lookup_authority_tests|frontend_parser_tests|cpp_parser_debug_qualified_type_spelling_stack)$"' > test_after.log 2>&1`

Proof log: `test_after.log`.

Step 3 review: `review/step3_parser_owner_recovery_review.md`, no blockers.
