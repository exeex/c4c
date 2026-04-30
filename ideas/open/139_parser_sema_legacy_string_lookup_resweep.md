# Parser Sema Legacy String Lookup Resweep

Status: Open
Created: 2026-04-30

Parent Ideas:
- [123_parser_legacy_string_lookup_removal_convergence.md](/workspaces/c4c/ideas/closed/123_parser_legacy_string_lookup_removal_convergence.md)
- [135_sema_structured_owner_static_member_lookup_cleanup.md](/workspaces/c4c/ideas/closed/135_sema_structured_owner_static_member_lookup_cleanup.md)
- [137_parser_known_function_name_compatibility_spelling_cleanup.md](/workspaces/c4c/ideas/closed/137_parser_known_function_name_compatibility_spelling_cleanup.md)

Follow-up Series:
- [140_hir_legacy_string_lookup_metadata_resweep.md](/workspaces/c4c/ideas/open/140_hir_legacy_string_lookup_metadata_resweep.md)
- [141_lir_legacy_string_lookup_metadata_resweep.md](/workspaces/c4c/ideas/open/141_lir_legacy_string_lookup_metadata_resweep.md)
- [142_bir_backend_legacy_string_lookup_metadata_resweep.md](/workspaces/c4c/ideas/open/142_bir_backend_legacy_string_lookup_metadata_resweep.md)

## Goal

Run a second parser and Sema cleanup pass for legacy rendered-string lookup
authority after the 123 and 135/137 cleanup series. Parser/Sema lookup should
prefer `TextId`, namespace context ids, `QualifiedNameKey`, direct AST links,
`TypeSpec::record_def`, and Sema structured owner keys wherever those carriers
exist.

Legacy strings may remain as source spelling, diagnostics, display/final
rendering, compatibility input, or explicitly classified TextId-less fallback.
They should not silently decide parser or Sema semantic lookup when structured
metadata is available.

## Why This Idea Exists

The first cleanup rounds deliberately retained several compatibility mirrors:
parser rendered tag maps, template rendered-name mirrors, NTTP default rendered
caches, string-facing typedef/type helpers, and Sema rendered owner fallbacks.
Those were acceptable as classified leftovers, but the current tree still has
enough residual string lookup surfaces to justify another scan.

This pass starts at the frontend boundary because downstream HIR, LIR, BIR, and
backend cleanup can only remove fallbacks honestly when parser/Sema preserve
metadata through AST and semantic handoff surfaces.

## Scope

- Re-inventory `src/frontend/parser` and `src/frontend/sema` for
  `std::string` keyed lookup maps, rendered-name fallback helpers, and
  compatibility mirror reads.
- Demote or convert parser typedef, value, tag, template, NTTP-default, and
  known-function lookup paths where `TextId`, `QualifiedNameKey`, or direct AST
  links already exist.
- Recheck Sema owner/member/static lookup paths for fallback from structured
  keys to rendered names.
- Audit parser-to-Sema metadata handoff for fields that lose `TextId`, namespace
  context, record identity, or owner key metadata and force later string lookup.
- Add focused tests proving drifted rendered strings do not override structured
  metadata for covered parser/Sema paths.
- When a cleanup is blocked by missing cross-module metadata, create a separate
  `ideas/open/` follow-up instead of expanding this idea.

## Out Of Scope

- HIR, LIR, BIR, or backend implementation cleanup except for documenting a
  metadata handoff blocker as a new idea.
- Removing diagnostic, source-spelling, dump, or final emitted text strings.
- Weakening supported behavior, marking tests unsupported, or accepting
  testcase-shaped shortcuts.

## Conflict Policy

- `TextId` is text identity only; semantic lookup should use a domain key or
  direct semantic carrier when one exists.
- If a test protects rendered-name lookup authority after structured metadata
  is available, update the test contract.
- If the frontend cannot honestly provide the metadata needed downstream, write
  a new open idea for that metadata bridge.
- Keep compatibility fallbacks only when they are TextId-less, producer-boundary,
  diagnostic/display, or final spelling behavior.

## Acceptance Criteria

- Covered parser/Sema lookup paths are structured-primary.
- Retained parser/Sema string lookup surfaces are classified and guarded so they
  cannot silently override structured metadata.
- Parser-to-Sema metadata handoff gaps are either repaired or split into new
  open ideas.
- Focused frontend tests cover same-feature drifted-string cases, not only one
  named testcase.

