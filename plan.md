# Qualified Name Deferred Carrier Authority

Status: Active
Source Idea: ideas/open/146_qualified_name_deferred_carrier_authority.md

## Purpose

Remove rendered qualified-name strings as semantic authority while preserving
structured qualified-name carriers across parser, Sema, and HIR deferred lookup.

## Goal

Qualified names such as `A::B::C` should be represented and resolved through
token-sequence/domain metadata, or carried as structured deferred lookup data
when template/dependent context prevents eager Sema resolution.

## Core Rule

Do not flatten a qualified name into one rendered spelling and later split that
spelling to recover semantic identity. A single `TextId` containing `::` is not
semantic qualified-name authority.

## Read First

- `ideas/open/146_qualified_name_deferred_carrier_authority.md`
- `src/frontend/parser/impl/core.cpp`
- `src/shared/qualified_name_table.hpp`
- `src/frontend/sema`
- `src/frontend/hir`

## Current Targets

- Parser structured qualified-name machinery:
  - `peek_qualified_name()`
  - `parse_qualified_name()`
  - `qualified_name_key()`
  - `QualifiedNameRef`
  - `QualifiedNameKey`
  - `NamePathTable`
- Parser compatibility escape hatches:
  - `qualified_key_in_context()`
  - `find_compatibility_key_from_rendered_qualified_spelling()`
  - `intern_compatibility_key_from_rendered_qualified_spelling()`
  - `has_typedef_type(TextId)` / `find_typedef_type(TextId)` when spelling
    contains `::`
  - `register_known_fn_name_in_context()` compatibility registration
- Shared qualified-name string helpers:
  - `split_qualified_name_scope()`
  - `qualified_name_base_matches()`
- Sema and HIR paths that accept one `TextId` whose spelling may contain `::`
  and use it for typedef, tag, value, function, namespace, concept, template,
  or deferred lookup.

## Non-Goals

- Do not force all C++ name lookup to finish in Sema before HIR.
- Do not replace diagnostic, dump, display, ABI, link, or emitted spelling.
- Do not broaden HIR, LIR, BIR, backend, or codegen work beyond narrow carrier
  plumbing needed for structured deferred lookup.
- Do not change C++ lookup rules beyond replacing rendered-string authority
  with structured carriers.
- Do not treat one interned `TextId` for `A::B::C` as a valid semantic
  qualified name.
- Do not weaken tests, mark qualified-name cases unsupported, or use
  expectation-only proof as progress.

## Working Model

- Lexer interns spelling into `TextId`; `TextId` is spelling identity only.
- Parser recognizes qualified-name syntax and builds provisional structured
  carriers from source token order, including base `TextId`, qualifier
  `TextId` sequence, global-qualification state, namespace/context hints, and
  source spelling for diagnostics/display.
- Sema owns eager semantic lookup when enough information is available. It
  resolves qualified names through domain-specific structured tables and keeps
  typedef, record/tag, value, function, namespace, concept, and template
  identities separate.
- When template/dependent context prevents a final Sema answer, Sema emits a
  structured deferred lookup carrier with owner/name/domain/substitution
  metadata.
- HIR may complete deferred lookup, but only by consuming structured deferred
  carriers. HIR must not recover identity by splitting rendered qualified
  strings.

## Execution Rules

- Work one lookup family at a time.
- Classify every `TextId` qualified-name use before changing authority.
- Split APIs into unqualified `TextId` entry points and structured
  `QualifiedNameRef` / `QualifiedNameKey` entry points instead of widening
  string helpers.
- Keep parser grammar disambiguation separate from semantic identity lookup.
- Keep display-only or compatibility-only rendered-name helpers explicitly
  labeled with owner, reason, and removal condition.
- Add focused tests where a rendered/suffix split route would choose the wrong
  name but structured token-sequence/domain-key lookup chooses correctly.
- For each code-changing step, run a fresh build or compile proof plus the
  narrow tests delegated by the supervisor. Escalate to a broader frontend/HIR
  subset when a step changes cross-layer carrier contracts.

## Ordered Steps

### Step 1: Inventory Qualified-Name Authority

Goal: identify every parser, Sema, shared, and HIR path that accepts one
`TextId` whose spelling may contain `::` and uses it for semantic lookup.

Primary targets:
- `src/frontend/parser/impl/core.cpp`
- `src/shared/qualified_name_table.hpp`
- `src/frontend/sema`
- `src/frontend/hir`

Actions:
- Inspect parser uses of `qualified_key_in_context()`,
  `find_compatibility_key_from_rendered_qualified_spelling()`,
  `intern_compatibility_key_from_rendered_qualified_spelling()`,
  qualified `has_typedef_type(TextId)` / `find_typedef_type(TextId)`, and
  `register_known_fn_name_in_context()` compatibility registration.
- Inspect shared helpers `split_qualified_name_scope()` and
  `qualified_name_base_matches()`.
- Inspect Sema and HIR APIs that use a single `TextId` for typedef, tag,
  value, function, namespace, concept, template, or late/deferred lookup.
- Classify each surface as structured semantic authority, unqualified-name
  authority, parse-time grammar disambiguation, diagnostics/display,
  compatibility, deferred carrier handoff, or test support.

Completion check:
- `todo.md` records the inventory, the classification for each discovered
  surface, and the first lookup family to migrate.
- No implementation files are changed unless the executor packet explicitly
  includes a small classification-only documentation update.

### Step 2: Separate Unqualified `TextId` APIs From Structured Qualified APIs

Goal: make semantic APIs that take `TextId` clearly unqualified-name APIs and
route compound names through structured carriers.

Primary targets:
- parser typedef/function/name lookup entry points
- Sema lookup entry points that currently accept spelling-only names
- `QualifiedNameRef` / `QualifiedNameKey` consumers

Actions:
- Add or tighten guards so unqualified `TextId` APIs reject or ignore spellings
  containing `::` as compound semantic authority.
- Introduce or route through structured `QualifiedNameRef` /
  `QualifiedNameKey` APIs for compound names.
- Preserve source spelling only for diagnostics/display.
- Keep parser grammar-required typedef disambiguation narrow and visible.

Completion check:
- Semantic compound-name lookup no longer enters through a `TextId` API that
  silently splits or interprets `::`.
- Build or compile proof for touched frontend files is green.
- Focused parser/Sema tests cover at least one `TextId` containing `::` being
  rejected as semantic authority where structured lookup is required.

### Step 3: Remove Parser Reliance On Rendered Qualified-Name Compatibility

Goal: demote parser compatibility helpers from semantic lookup authority to
explicitly bounded compatibility or display support.

Primary targets:
- `qualified_key_in_context()`
- `find_compatibility_key_from_rendered_qualified_spelling()`
- `intern_compatibility_key_from_rendered_qualified_spelling()`
- `register_known_fn_name_in_context()`

Actions:
- Replace semantic parser decisions based on rendered qualified spelling with
  structured token-sequence/name-path keys.
- Delete compatibility registrations that exist only to make later string
  splitting recover semantic structure, or label remaining uses with owner,
  reason, and removal condition.
- Preserve display and diagnostics routes without using them as lookup
  authority.

Completion check:
- Parser qualified-name construction produces structured carriers from source
  token `TextId` sequence without flattening and reparsing rendered spelling.
- Any remaining rendered-qualified-name compatibility path is explicitly
  bounded and is not the primary semantic lookup route.
- Build plus the delegated narrow parser lookup tests are green.

### Step 4: Route Eager Sema Qualified Lookup Through Domain Tables

Goal: make Sema the authority for non-dependent qualified-name semantic lookup
across relevant domains.

Primary targets:
- Sema typedef lookup
- Sema record/tag lookup
- Sema value/function/namespace/template/concept lookup paths that receive
  qualified names

Actions:
- Resolve non-dependent qualified names through domain-specific structured
  keys and tables.
- Preserve qualifier path, namespace/global metadata, lookup-domain
  expectation, and source location in lookup inputs.
- Keep typedef, record/tag, value, function, namespace, concept, and template
  identity distinct.
- Avoid reusing parser compatibility maps as final semantic authority.

Completion check:
- Non-dependent qualified lookup uses structured domain keys or tables, not
  rendered qualified-name splitting.
- Same-spelling or suffix-collision cases in different scopes/namespaces can
  be represented without collision.
- Build plus focused Sema/parser lookup tests are green.

### Step 5: Define Structured Deferred Lookup Carrier For Dependent Cases

Goal: preserve unresolved qualified names as structured deferred carriers when
Sema cannot finish lookup before HIR.

Primary targets:
- Sema output carriers for template/dependent qualified names
- HIR input/lowering surfaces that consume unresolved qualified names

Actions:
- Identify the minimal deferred payload: qualifier token sequence or name-path
  key, base `TextId`, global-qualification state, namespace/context metadata,
  lookup-domain expectation, source location, and substitution/dependent
  context.
- Emit the deferred carrier from Sema instead of a rendered spelling when final
  lookup must wait.
- Keep diagnostic/display spelling separate from the deferred semantic payload.

Completion check:
- Dependent/template cases can leave Sema with structured unresolved
  qualified-name metadata.
- No deferred carrier serializes semantic identity into display text that HIR
  later parses.
- Build plus the delegated dependent/template frontend tests are green.

### Step 6: Consume Deferred Qualified Lookup Structurally In HIR

Goal: make HIR late lookup consume structured deferred carriers and convert
them into HIR/module-domain keys.

Primary targets:
- HIR deferred lookup consumers
- HIR/module-domain key construction for qualified names

Actions:
- Replace HIR late lookup that splits rendered strings with structured carrier
  consumption.
- Complete late lookup using owner/name/domain/substitution metadata.
- Convert resolved results into HIR/module-domain keys without treating a
  rendered qualified spelling as compound identity.
- Keep HIR display, dump, or diagnostic text separate from semantic lookup.

Completion check:
- HIR late lookup does not split rendered qualified-name strings.
- Structured deferred carriers are sufficient for the migrated dependent cases.
- Build plus focused frontend/HIR deferred lookup tests are green.

### Step 7: Add Collision And Stale-Route Proof

Goal: prove structured token-sequence/domain-key lookup fixes cases where
rendered or suffix splitting would choose the wrong semantic name.

Primary targets:
- frontend parser/Sema lookup tests
- frontend HIR deferred lookup tests

Actions:
- Add at least one collision or stale-route test where rendered splitting,
  suffix matching, or one-`TextId` compound-name lookup would resolve the wrong
  entity.
- Cover the migrated eager path and at least one deferred/template path if
  Step 5 or Step 6 changed dependent lookup.
- Keep tests semantic and nearby-feature oriented; do not shape proof around
  one named fixture.

Completion check:
- Tests fail on the old rendered/suffix authority route and pass through
  structured qualified-name lookup.
- No tests are weakened, marked unsupported, or changed only by expectation
  rewrite.
- Build plus the delegated narrow tests are green.

### Step 8: Final Compatibility Boundary Review

Goal: ensure rendered qualified-name helpers are removed from semantic paths or
documented as bounded display/compatibility helpers.

Primary targets:
- `src/frontend/parser/impl/core.cpp`
- `src/shared/qualified_name_table.hpp`
- migrated Sema/HIR lookup paths
- tests added under this plan

Actions:
- Re-scan all inventoried surfaces from Step 1.
- Confirm `find_compatibility_key_from_rendered_qualified_spelling()` and
  `split_qualified_name_scope()` are either removed from semantic paths or
  explicitly labeled as compatibility/display helpers.
- Confirm all remaining one-`TextId` lookup APIs are unqualified-name APIs.
- Run the supervisor-selected broader frontend/HIR validation subset.

Completion check:
- Acceptance criteria in the source idea are satisfied or any remaining
  compatibility path has a named owner, reason, and removal condition.
- No rendered qualified spelling remains the primary semantic lookup authority.
- Broader validation selected by the supervisor is green and recorded in
  `test_after.log`.
