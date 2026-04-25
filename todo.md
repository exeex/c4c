Status: Active
Source Idea Path: ideas/open/98_parser_sema_post_cleanup_structured_identity_leftovers.md
Source Plan Path: plan.md
Current Step ID: 8
Current Step Title: Demote Only Proven Parser/Sema Legacy Fallbacks

# Current Packet

## Just Finished

Step 8 - Demote Only Proven Parser/Sema Legacy Fallbacks is complete.

`review/98_step8_route_review.md` now includes the Step 8 final audit. No broad
fallback demotion was accepted: every remaining rendered-name parser/sema
fallback inspected from Steps 2-7 is classified as a compatibility bridge,
proof-required fallback, or downstream-blocked/unsupported-metadata path.

## Suggested Next

Supervisor broader-proof packet for the accumulated idea 98 route, followed by
plan-owner lifecycle closure review if that proof stays green.

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
- Step 5 added sema-only template type-parameter `TextId` mirrors in
  `validate.cpp`; it did not alter parser metadata, HIR/type/codegen identity,
  template rendering, diagnostics, expectations, or consteval NTTP call
  binding maps.
- Enum mirror population now depends on parser-owned `enum_name_text_ids`; keep
  any future enum work on that definition-time metadata and do not re-derive
  stable identity from rendered strings.
- Step 6 added no parser metadata, HIR/type/codegen identity migration,
  template rendering changes, diagnostics changes, expectation rewrites, or
  broad compile-time engine call-site migration outside the owned sema path.
- Step 7 added no parser metadata, HIR/type/codegen identity migration,
  template rendering changes, diagnostics changes, expectation rewrites, or
  testcase-shaped exceptions.
- Step 8 made no implementation or test changes. Preserve the final audit
  decision: no future fallback removal should be accepted unless it names one
  directly proven parser/sema-owned fallback and preserves AST/HIR/consteval,
  diagnostics, codegen, link-name bridges, and unsupported `kInvalidText`
  metadata paths.
- Preserve the unrelated untracked `review/parser_step8_demotion_route_review.md`
  artifact.

## Proof

Source audit only.

No code changes were made, so the delegated packet did not require a fresh
`test_after.log`.
