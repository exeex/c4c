Status: Active
Source Idea Path: ideas/open/198_parser_legacy_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Parser Compatibility Routes

# Current Packet

## Just Finished

Lifecycle activation created the active runbook from
`ideas/open/198_parser_legacy_compatibility_retirement.md`.

## Suggested Next

Execute Step 1 from `plan.md`: inventory parser-owned compatibility routes,
classify retained rendered-string paths, and identify the first narrow stale
rendered parser conversion or fencing target.

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

## Proof

Lifecycle-only activation; no build or code validation required.
