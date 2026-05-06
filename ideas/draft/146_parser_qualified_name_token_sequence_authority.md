# Parser Qualified Name Token Sequence Authority

Status: Draft
Created: 2026-05-06

Parent Ideas:
- `ideas/closed/139_parser_sema_rendered_string_lookup_removal.md`
- `ideas/open/145_move_record_tag_authority_from_parser_to_sema.md`

## Goal

Stop treating rendered qualified-name strings as semantic qualified-name
authority in parser compatibility paths.

The parser should build compound name identity from the source token `TextId`
sequence and the relevant domain table key, such as `NamePathTable` /
`QualifiedNameKey`. Rendered spelling like `A::B::C` may remain available for
diagnostics, dumps, debug display, and temporary compatibility, but semantic
lookups should not flatten a qualified name into one spelling and later split
that spelling back into parts.

## Why This Idea Exists

The current parser still has qualified-name compatibility paths that flatten
`A::B::C` into a rendered spelling or one `TextId`, then recover structure by
splitting strings later. Current anchors include
`src/frontend/parser/impl/core.cpp`, especially compatibility helpers around
`qualified_key_in_context` and
`find_compatibility_key_from_rendered_qualified_spelling`, and
`src/shared/qualified_name_table.hpp`, especially
`split_qualified_name_scope`.

That keeps a rendered string in the semantic path even after the broader
string-authority cleanup. The intended policy is:

- Lexer interns spelling into `TextId`.
- `TextId` carries no semantics beyond spelling identity.
- Semantic meaning lives in domain tables.
- Compound names are indexed by `TextId` sequences and domain keys, not by
  rendered strings.

This idea exists to make qualified names obey that policy at their source,
instead of preserving a string-shaped compatibility channel as the practical
authority.

## In Scope

- Inventory parser qualified-name construction and lookup paths that build or
  consume rendered spellings, including helpers around `qualified_key_in_context`
  and `find_compatibility_key_from_rendered_qualified_spelling` in
  `src/frontend/parser/impl/core.cpp`.
- Inventory shared qualified-name helpers such as
  `split_qualified_name_scope` in `src/shared/qualified_name_table.hpp` and
  classify whether each use is semantic, diagnostic, debug, or compatibility.
- Build `NamePathTable` / `QualifiedNameKey` entries directly from the token
  `TextId` sequence observed in source, preserving qualifier segmentation
  without rendering and reparsing.
- Keep rendered qualified spelling as display metadata only, or as a clearly
  bounded compatibility mirror while call sites migrate.
- Add focused tests or probes where same rendered suffixes, nested namespaces,
  and local/context-qualified names would collide or resolve incorrectly if
  flattened strings were authoritative.
- Document any remaining string-splitting compatibility path with the exact
  owner and removal condition.

## Out Of Scope

- Replacing every diagnostic, dump, or source display spelling with structured
  formatting.
- Treating a single `TextId` for `A::B::C` as semantic identity. A single
  interned spelling is still spelling identity, not a compound name key.
- Broad Sema, HIR, LIR, BIR, backend, or codegen rewrites except for narrow
  call-site updates needed to consume the structured qualified-name key.
- Changing C++ name lookup rules beyond removing rendered-string authority
  from the parser compatibility route.
- Weakening tests, marking qualified-name cases unsupported, or accepting
  expectation-only proof as capability progress.

## Acceptance Criteria

- Parser qualified-name construction can produce a domain-specific
  `QualifiedNameKey` or equivalent key from the source token `TextId` sequence
  without flattening and later splitting a rendered spelling.
- Semantic qualified-name lookup paths no longer depend on
  `find_compatibility_key_from_rendered_qualified_spelling` or
  `split_qualified_name_scope` as their authoritative route.
- Any remaining rendered qualified spelling is classified as diagnostics,
  dumps, display, or temporary compatibility with a bounded removal path.
- Tests or focused probes cover at least one case where flattened spelling or
  suffix-oriented splitting would choose the wrong compound name.
- The implementation preserves the policy that `TextId` is only spelling
  identity and that compound semantic names live in domain tables keyed by
  token sequences.

## Reviewer Reject Signals

- A slice claims progress by renaming a string key or helper while still
  resolving qualified names by rendering `A::B::C` and splitting it later.
- A route treats one interned `TextId` containing `A::B::C` as equivalent to a
  structured compound-name key.
- Compatibility helpers such as
  `find_compatibility_key_from_rendered_qualified_spelling` remain the primary
  semantic lookup path after the slice claims authority moved.
- Tests are weakened, marked unsupported, or rewritten to avoid cases where
  segmentation, namespace context, or same-spelling components matter.
- The implementation adds named-case shortcuts for one failing qualified-name
  fixture instead of constructing domain keys from token sequences.
- Broad lookup or backend rewrites are mixed into this idea without proving
  they are required to remove parser rendered-string authority.
- The exact old failure mode remains behind a new abstraction name: semantic
  success still depends on a rendered qualified spelling matching.
