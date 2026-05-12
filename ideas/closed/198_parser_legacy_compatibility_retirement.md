# Parser Legacy Compatibility Retirement

Status: Closed
Created: 2026-05-12

Depends On:
- `ideas/open/195_frontend_to_bir_legacy_string_lookup_closure_gate.md`
- `ideas/open/196_hir_pending_consteval_structured_identity.md`

## Goal

Retire or hard-fence the remaining parser-owned legacy compatibility paths so
parser output carries structured syntax and identity facts without letting
rendered strings act as a second semantic route.

## Why This Idea Exists

The parser side of the identity migration already routes ordinary record,
qualified-name, and source-spelling identity through `TextId`, qualifier
segment carriers, direct `record_def` links, and Sema-owned record-domain
lookups. Some compatibility bridges remain intentionally available for older
callers, parser-local probes, and display/debug spelling.

This idea is the next parser-only cleanup pass after the frontend-to-BIR
closure gates. It should remove safe retained bridges, fence unavoidable ones,
and make any newly discovered leftovers grep-friendly through explicit
`legacy` or `deprecated` comments.

## In Scope

- Inventory parser-owned compatibility routes already called out by earlier
  closure notes and source comments, including:
  - `resolve_record_type_spec_with_parser_tag_map_compatibility`
  - `parser_record_layout_compatibility_tag_map`
  - `eval_const_int_with_rendered_named_const_compatibility`
  - retained `struct_tag_def_map` / `defined_struct_tags` compatibility probes
  - parser-local record setup that still lacks structured qualified-name
    metadata and falls back to rendered tag sets
  - template instantiated record lookup that still derives candidate keys from
    `template_origin_name`
  - rendered owner/tag recovery in parser expression construction when
    structured owner metadata is incomplete
- Delete compatibility entry points that no production parser caller still
  needs after ideas 195 and 196.
- For retained parser bridges, require an explicit no-metadata or parser-local
  compatibility boundary.
- Add focused tests or probes where stale rendered parser spelling cannot
  override complete structured carriers.
- When a newly discovered parser compatibility path cannot be removed in this
  idea, add a nearby source comment containing `legacy` or `deprecated`, plus
  owner, limitation, and removal condition.

## Out Of Scope

- Rewriting Sema-owned record-domain lookup.
- Reworking HIR, LIR, BIR, or backend consumers.
- Removing parser diagnostics, AST display strings, source spelling payloads,
  or final output text.
- Treating a raw `TextId` alone as complete semantic identity across scopes.
- Weakening tests or changing expectations to hide an unresolved parser
  compatibility path.

## Acceptance Criteria

- Parser metadata-rich routes do not recover record, owner, qualified-name, or
  const-int identity through rendered strings after a complete structured miss.
- Any remaining parser rendered-string bridge is named as compatibility,
  no-metadata, display, diagnostic, or parser-local scratch state.
- Newly discovered retained parser bridges have source comments containing
  `legacy` or `deprecated` and a concrete removal condition.
- Focused proof covers at least one stale-rendered parser path that would have
  passed under the old compatibility authority.
- The final ledger states which parser compatibility routes were deleted,
  fenced, or split into follow-up work.

## Reviewer Reject Signals

- A slice renames a compatibility helper while preserving the same rendered
  lookup authority on metadata-rich parser paths.
- A structured miss can still fall through to `struct_tag_def_map`,
  `defined_struct_tags`, rendered qualified spelling, or rendered named const
  lookup without an explicit no-metadata boundary.
- A newly discovered retained parser bridge lacks a `legacy` or `deprecated`
  comment.
- Tests are weakened, deleted, or narrowed to one fixture instead of proving
  parser-domain behavior.
- The route expands into Sema/HIR/backend cleanup instead of staying parser
  owned.

## Closure Notes

Closed: 2026-05-12

The parser-owned compatibility retirement runbook completed. The final ledger
in `todo.md` before closure classified deleted, converted, fenced, and
intentionally retained parser compatibility routes for record/layout lookup,
qualified-name and owner recovery, named-constant const-int lookup, and
template instantiated record lookup.

Parser metadata-rich routes now use structured carriers such as direct
`record_def`, scoped `TextId`, qualifier/owner metadata, structured template
instantiation keys, or structured named-constant tables. Covered complete
structured misses no longer recover semantic identity through stale rendered
parser spelling.

Retained parser bridges are fenced as legacy/no-metadata, display/diagnostic,
or parser-local scratch behavior with owner, limitation, and removal condition.
Residual HIR or Sema template callers that still require rendered record,
owner, template-origin, or named-constant compatibility are outside this
parser-owned idea and should be tracked by a separate open idea if pursued.

Close proof:

- Broader parser/frontend close scope passed 7/7 in `test_after.log`.
- Regression guard passed with matching `test_before.log` and `test_after.log`
  using non-decreasing pass-count mode: 7 passed before, 7 passed after, 0 new
  failures.
- Latest accepted full-suite baseline at `a654583f2` recorded 3137 passed, 0
  failed, with the existing 12 disabled backend CLI tests.
