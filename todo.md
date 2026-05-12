Status: Active
Source Idea Path: ideas/open/198_parser_legacy_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Retire Rendered Named-Const Compatibility

# Current Packet

## Just Finished

Step 4 hard-fenced
`eval_const_int_with_rendered_named_const_compatibility`.

The compatibility bridge now accepts an optional structured named-constant
`TextId` table. When that table is supplied and the node carries parser
named-constant metadata, structured lookup is authoritative: a hit returns the
`TextId` value, and a miss fails closed before rendered named-constant lookup.
The retained rendered lookup is explicitly documented as legacy/no-metadata
compatibility for callers such as HIR template probes that still pass rendered
NTTP names, with the removal condition tied to those callers carrying TextIds.

The direct parser-support residual metadata test now proves that stale rendered
named constants cannot override complete parser metadata in the compatibility
bridge, including a qualified metadata miss, while preserving explicit
rendered no-metadata behavior.

## Suggested Next

Hand this Step 4 named-constant fence slice back to the supervisor for review
and commit. If the supervisor wants additional Step 4 coverage, inspect
parser-side `eval_const_int` call sites for any remaining rendered
named-constant handoff before asking the plan owner to advance.

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
- Existing HIR template callers still use the rendered bridge without a
  structured table. That behavior is intentionally preserved by the optional
  parameter and should be retired only when those callers carry TextIds.
- The compatibility helper treats parser name metadata as authoritative only
  when a structured table is supplied. Rendered no-metadata behavior remains
  available for explicit legacy callers.
- Step 3 reviewer watch item: retained no-metadata/template-instantiation
  fallbacks in `base.cpp` should be accounted for in Step 5 and the Step 6
  closure ledger.

## Proof

Step 4 proof passed:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^cpp_hir_parser_support_residual_structured_metadata$"' > test_after.log 2>&1`

Proof log: `test_after.log`.
