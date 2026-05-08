# Parser Sema Qualified Name Text Reparse Retirement

Status: Active
Source Idea: ideas/open/154_parser_sema_qualified_name_text_reparse_retirement.md

## Purpose

Retire parser/sema paths that rebuild qualified-name meaning by splitting or
injecting rendered strings such as `A::B::C`.

Goal: make semantic qualified-name lookup consume structured carriers when
they exist: qualifier `TextId` segments, parser symbols, global qualification,
source spans, `NamePathKey`, and `QualifiedNameKey`.

## Core Rule

Rendered qualified-name spelling may remain for diagnostics, debug output,
legacy AST mirrors, and explicit compatibility syntax reconstruction. It must
not be the primary semantic lookup authority when structured carriers are
available.

## Read First

- `ideas/open/154_parser_sema_qualified_name_text_reparse_retirement.md`
- Parser qualified-name helper definitions and call sites for:
  - `qualified_name_from_text`
  - `split_qualified_member_type_name`
  - `append_qualified_name_tokens`
  - `QualifiedNameRef::qualifier_segments`
  - `QualifiedNameRef::base_name`
- Recent closed idea history named by the source idea only when needed for
  context.

## Current Targets

- Parser helpers that accept or emit rendered qualified-name strings.
- Parser/sema boundaries where structured qualified-name carriers are dropped
  and later reconstructed from text.
- Tests that can prove stale or ambiguous rendered spelling cannot override
  structured qualified-name identity.

## Non-Goals

- Do not remove diagnostic, dump, or debug rendering of qualified names.
- Do not remove token injection needed to reparse deferred source syntax.
- Do not reopen HIR/LIR rendered-owner compatibility covered by idea 152.
- Do not treat a single rendered-owner `TextId` containing `A::B` as semantic
  compound identity.

## Working Model

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

## Execution Rules

- Prefer direct `QualifiedNameRef`, `NamePathKey`, `QualifiedNameKey`, or
  Sema-domain lookup over text splitting.
- If a rendered-string helper remains, classify it as compatibility, display,
  or syntax reconstruction, and record the removal condition in `todo.md`.
- Do not add new helpers whose main behavior is splitting rendered qualified
  names for semantic lookup.
- For code-changing steps, run `cmake --build --preset default` plus a focused
  CTest subset covering parser lookup, frontend parser, and nearby structured
  metadata tests. Escalate to broader validation before closure.

## Steps

### Step 1: Inventory Qualified-Name Text Authority

Goal: locate and classify all parser helpers and call sites that convert
rendered qualified-name strings into structured names or tokens.

Primary targets:

- `qualified_name_from_text`
- `split_qualified_member_type_name`
- `append_qualified_name_tokens`
- `QualifiedNameRef::qualifier_segments`
- `QualifiedNameRef::base_name`

Actions:

- Inspect helper definitions and direct callers.
- Classify each caller as semantic lookup, compatibility bridge, display/debug
  projection, or source-syntax reconstruction.
- Identify call sites where structured carriers were available earlier but
  dropped before lookup.
- Record the first code packet target and focused proof command in `todo.md`.

Completion check:

- `todo.md` names the owned helper family, the first semantic authority path to
  repair, retained compatibility paths, and the initial validation subset.

### Step 2: Retire Ordinary Semantic Reparse Callers

Goal: replace ordinary semantic qualified-name lookups that split rendered
strings even though structured carriers are available.

Actions:

- Pass or preserve `QualifiedNameRef`, qualifier `TextId` segments, global
  qualification, `NamePathKey`, or `QualifiedNameKey` through the affected
  parser/sema boundary.
- Keep rendered spelling only as display or compatibility mirror beside the
  structured carrier.
- Add or update focused coverage where a flattened spelling would pick a stale
  owner or wrong namespace/type/value.

Completion check:

- The selected semantic lookup path no longer depends on splitting
  `A::B::C` text when structured carrier data exists, and the focused tests
  prove structured authority.

### Step 3: Demote Qualified Member-Type String Splitting

Goal: ensure member-type qualified-name handling uses structured owner/path
carriers before string splitting.

Actions:

- Inspect uses of `split_qualified_member_type_name` and adjacent member-type
  lookup paths.
- Route covered semantic cases through structured owner or name-path metadata.
- Leave string splitting only as a bounded compatibility bridge when no
  structured carrier exists.
- Add tests with ambiguous same-spelling member names under different owners if
  the code surface supports it.

Completion check:

- Covered member-type lookup paths prefer structured owner/path carriers, and
  any retained splitter is documented as compatibility with a removal
  condition.

### Step 4: Bound Token Injection Helpers

Goal: keep `append_qualified_name_tokens` and related token synthesis helpers
available only for explicit syntax reconstruction or compatibility injection.

Actions:

- Audit token-injection callers for semantic lookup behavior.
- Replace semantic callers with direct structured carriers or Sema-domain
  lookups.
- Add comments or names that distinguish syntax reconstruction from semantic
  identity.

Completion check:

- Token injection is not the authority path for covered semantic
  qualified-name lookup.

### Step 5: Audit QualifiedNameRef Display Mirrors

Goal: keep `QualifiedNameRef::qualifier_segments` and `base_name` as display
mirrors rather than authoritative semantic path components.

Actions:

- Inspect uses of display segment strings.
- Prefer matching `TextId` segments, parser symbols, `NamePathKey`, or
  `QualifiedNameKey` where semantic identity is needed.
- Add tests where stale display spelling is present but structured identity
  must win, if an existing test seam can express that state without overfit.

Completion check:

- Semantic callers touched by this step do not use display strings without the
  corresponding structured carrier.

### Step 6: Closure Review And Broader Proof

Goal: verify the remaining rendered qualified-name helpers are explicitly
classified and cannot override structured identity.

Actions:

- Review all retained helper paths from Step 1 classification.
- Record owners, limitations, and removal/non-removal conditions for retained
  compatibility, display, or syntax reconstruction helpers.
- Run broader validation selected by the supervisor or lifecycle close gate.
- Ensure tests cover semantic authority, not only spelling stability.

Completion check:

- Source idea acceptance criteria are satisfied.
- Any retained rendered qualified-name reparse helper is classified with a
  bounded rationale.
- Fresh validation is available for closure review.
