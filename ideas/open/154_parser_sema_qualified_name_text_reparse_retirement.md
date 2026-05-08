# Parser Sema Qualified Name Text Reparse Retirement

Status: Open
Created: 2026-05-08

Parent Ideas:
- `ideas/closed/146_qualified_name_deferred_carrier_authority.md`
- `ideas/closed/147_rendered_qualified_compatibility_bridge_removal.md`
- `ideas/closed/151_parser_out_of_class_owner_probe_token_sequence.md`

## Goal

Retire parser/sema paths that rebuild qualified-name meaning by splitting or
injecting rendered strings such as `A::B::C`.

Qualified names should originate from token sequences and flow as structured
carriers: qualifier `TextId` segments, `ParserSymbolId`s where available,
global qualification, source spans, `NamePathKey`, and `QualifiedNameKey`.
Rendered spelling may remain for diagnostics, debug output, legacy AST mirrors,
and injected compatibility syntax, but not as semantic lookup authority.

## Why This Idea Exists

After ideas 146, 147, and 151, the main qualified-name direction is clear:
compound name meaning is not a single rendered string. Parser still has helper
surfaces that can parse or synthesize qualified names from strings:

- `qualified_name_from_text`
- `split_qualified_member_type_name`
- `append_qualified_name_tokens`
- `QualifiedNameRef::qualifier_segments` and `base_name` display mirrors

Some of these are legitimate compatibility or syntax-injection helpers. Others
may still hide semantic reparse behavior at parser/sema boundaries.

## Responsibility Split

Parser owns lexical qualified-name carriers:

- token sequence
- qualifier/base `TextId`s
- global qualifier bit
- parser-local symbol ids
- AST projection strings for display only

Sema owns qualified-name resolution:

- namespace/type/value/tag disambiguation
- visible-name domain lookup
- alias/using resolution
- construction of semantic domain keys from parser carriers

HIR may consume resolved or deferred qualified-name carriers. It should not
split rendered qualified names to recover semantics.

## In Scope

- Inventory all parser helpers that accept a rendered qualified-name string and
  return a structured name or tokens.
- Identify call sites where rendered strings are used because structured
  carriers were dropped earlier.
- Replace in-scope semantic call sites with direct `QualifiedNameRef`,
  `NamePathKey`, `QualifiedNameKey`, or Sema-domain lookup.
- Keep string-to-token helpers only for explicit compatibility, diagnostics,
  or source-syntax reconstruction, with removal conditions.
- Audit `QualifiedNameRef::qualifier_segments` / `base_name` so they remain
  display mirrors beside `TextId` segments, not the authoritative path.
- Add focused coverage where flattened spelling would choose a stale owner,
  wrong namespace, or wrong member type.

## Out Of Scope

- Removing diagnostic or dump rendering of qualified names.
- Removing token injection needed to reparse deferred source syntax.
- Reopening HIR/LIR rendered-owner compatibility already handled by idea 152.
- Treating a single rendered-owner `TextId` containing `A::B` as semantic
  compound identity.

## Acceptance Criteria

- Semantic qualified-name lookup does not depend on splitting rendered
  `A::B::C` strings when token/TextId segment carriers are available.
- Any retained rendered qualified-name reparse helper is explicitly classified
  as compatibility, display, or syntax reconstruction.
- Sema consumes parser structured qualified-name carriers for covered paths.
- Tests prove stale or ambiguous rendered qualified spelling cannot override
  structured qualified-name identity.

## Reviewer Reject Signals

- A slice creates a new helper that still splits rendered qualified names as
  primary semantic lookup.
- A rendered `TextId` for `A::B::C` is treated as a complete semantic key.
- `qualifier_segments` strings are used without checking the matching
  `qualifier_text_ids` / `QualifiedNameKey`.
- Tests only assert prettier spelling and do not prove semantic authority.
