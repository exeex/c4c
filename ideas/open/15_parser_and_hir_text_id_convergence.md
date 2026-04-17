# Parser And HIR Text-Id Convergence

## Goal

Continue the repo's string-identity cleanup by replacing remaining
parser/HIR-owned textual identity fields with `TextId` where the data is
fundamentally "text in this TU" rather than:

- final link-visible symbol identity
- diagnostic prose
- serialized canonicalization text
- short-lived local working strings

This idea is specifically about converging parser and HIR onto the existing
TU-scoped `TextTable` model, not about emitted symbol ids. Final link-visible
names belong to the separate `LinkNameId` idea.

## Why This Is Separate

The repo now has three distinct text/name layers:

- `TextId` for TU-scoped text identity
- `SymbolId` for source-atom parser identity
- `LinkNameId` for final link-visible symbol identity

Those layers should not be collapsed together.

Recent work already moved target handling away from raw strings and introduced a
real `LinkNameTable` direction for final emitted names. But HIR and parser
still keep many persistent `std::string` fields that are really just textual
identity and would fit the existing `TextTable` model better.

This idea covers that middle ground:

- source-facing names and tags that persist in IR/state
- qualified-name segments that outlive one local parse helper
- HIR fields that store stable text identity but are not link-visible names

## Existing Repo Shape

Relevant files:

- [src/frontend/string_id_table.hpp](/workspaces/c4c/src/frontend/string_id_table.hpp)
- [src/frontend/parser/parser.hpp](/workspaces/c4c/src/frontend/parser/parser.hpp)
- [src/frontend/hir/hir_ir.hpp](/workspaces/c4c/src/frontend/hir/hir_ir.hpp)

Current useful properties already exist:

- `StringIdTable` is TU-scoped and append-only
- `Parser::SymbolTable` already maps `SymbolId -> TextId`
- HIR modules already own a TU-scoped `TextTable` for link-name interning

So the missing piece is not storage. The missing piece is migrating the
remaining long-lived parser/HIR string carriers onto `TextId`.

## Scope

### 1. HIR Text Carriers

The first HIR candidates are fields whose value is stable TU text identity but
not a final emitted symbol id.

Good early candidates:

- `Param.name`
- `MemberExpr.field`
- `InitListItem.field_designator`
- `DeclRef.user_name`
- `TemplateCallInfo.fn_name`
- `ConstevalCallInfo.fn_name`
- `HirTemplateDef.name`
- `HirStructDef.tag`
- `HirStructDef.base_tags`
- `DeclStmt.symbol`

These should migrate toward `TextId` or parallel `TextId` fields before any
attempt to delete legacy string fields.

### 2. HIR Indexes And Registries

Some HIR maps still key directly by `std::string` / `SymbolName` and should be
reviewed for `TextId` use when they represent plain text identity rather than
semantic id spaces.

Candidates:

- struct definition indexes
- template definition indexes
- insertion-order tag/name lists

Not every map must move immediately, but the goal is to stop using raw strings
as the default persistent key where `TextId` is sufficient.

### 3. Parser Persistent Text State

Parser has many temporary strings that should remain temporary, but it also has
persistent state that is really just text identity.

Good early parser candidates:

- `QualifiedNameRef.qualifier_segments`
- `QualifiedNameRef.base_name`
- `TemplateScopeFrame.owner_struct_tag`
- `last_resolved_typedef_`
- `current_struct_tag_`
- `current_namespace_`
- `last_using_alias_name_`

These are more plausible `TextId` targets than parse-failure/debug strings.

### 4. Explicit Non-Scope

This idea does **not** try to replace every `std::string` in parser/HIR.

Out of scope for the first slice:

- diagnostics and error text
- stack traces and debug summaries
- serialized canonicalization text such as specialization keys
- inline asm templates / constraint text
- local working strings used only inside one helper
- final emitted names that should instead go through `LinkNameId`

## Review Rule

Before converting a field, classify it as one of:

1. `TextId`
   Stable TU text identity with no separate semantic namespace.
2. `SymbolId`
   Source-atom identity in parser/source-facing logic.
3. `LinkNameId`
   Final emitted logical symbol identity.
4. `std::string`
   Diagnostic, serialization, or short-lived working text.

Do not convert by pattern matching on field names alone.

## First Landing Slice

The first patch should stay narrow and mechanical.

1. Add parallel `TextId` fields to a bounded HIR subset:
   - one or two decl/reference name carriers
   - one struct/tag carrier
2. Thread the relevant `TextTable` access through HIR construction helpers.
3. Add one bounded parser-side `TextId` carrier for qualified-name or current
   scope state.
4. Keep legacy string fields temporarily if needed for transition.
5. Prove at least one end-to-end parser->HIR path uses `TextId` instead of
   storing only strings.

## Constraints

- do not collapse `TextId`, `SymbolId`, and `LinkNameId`
- do not turn this into a repo-wide string purge
- do not rewrite diagnostics/debug text into ids
- do not use `TextId` where final emitted symbol identity should instead be a
  dedicated `LinkNameId`
- do not force backend-private names into this idea

## Acceptance Criteria

- [ ] at least one bounded parser state carrier moves from persistent string
      storage to `TextId`
- [ ] at least one bounded HIR text carrier moves from persistent string
      storage to `TextId`
- [ ] the migration preserves the distinction between `TextId`, `SymbolId`, and
      `LinkNameId`
- [ ] diagnostic/debug/serialization strings remain out of scope unless there
      is a specific reason to keep them

## Validation

- parser regression suite
- frontend/HIR regression suite
- one or more focused parser->HIR tests proving the migrated carrier still
  round-trips correctly

## Non-Goals

- replacing all strings in parser or HIR
- changing final link-visible naming policy
- rewriting backend/private labels through `TextId`
- changing parser diagnostics to id-based storage

## Good First Patch

Start with one parser qualified-name carrier and one HIR field/name carrier that
obviously represent stable TU text identity, wire them to the existing
`TextTable`, and prove that the resulting path no longer depends on owning raw
strings for those fields.
