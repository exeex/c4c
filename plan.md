# Template Instantiation Structured Argument Key Runbook

Status: Active
Source Idea: ideas/open/149_template_instantiation_structured_argument_key.md

## Purpose

Replace rendered-string template argument keys in template instantiation
metadata with structured type and value argument keys.

Goal: template instantiation identity must depend on type-domain, value-domain,
template-domain, or structured deferred argument identity, not on canonical
rendered spelling, debug strings, or reparsed text fragments.

## Core Rule

Do not claim progress by renaming, wrapping, or relocating a string-shaped
semantic key. `TemplateInstantiationKey::Argument` must distinguish type and
value arguments through structured domain-specific payloads, with any retained
rendering limited to diagnostics, dumps, or clearly marked compatibility
mirrors.

## Read First

- Source idea: `ideas/open/149_template_instantiation_structured_argument_key.md`
- Parent context:
  - `ideas/closed/139_parser_sema_rendered_string_lookup_removal.md`
  - `ideas/closed/145_move_record_tag_authority_from_parser_to_sema.md`
  - `ideas/closed/146_qualified_name_deferred_carrier_authority.md`
  - `ideas/closed/147_rendered_qualified_compatibility_bridge_removal.md`
  - `ideas/closed/148_hir_static_member_carrier_authority_decomposition.md`

## Current Scope

- `TemplateInstantiationKey::Argument` in
  `src/frontend/parser/impl/parser_state.hpp`
- `make_template_instantiation_argument_key` and related helpers in
  `src/frontend/parser/impl/types/types_helpers.hpp`
- construction, comparison, hashing, storage, and dump sites for template
  instantiation argument keys
- parser carriers for type arguments and value arguments
- Sema eager interpretation of non-dependent type and value arguments where
  available
- HIR/template-instantiation routes that consume deferred or dependent
  template argument metadata
- focused tests or probes that expose formatting-sensitive or ambiguous
  rendered argument identity

## Non-Goals

- Do not redesign full template instantiation semantics, overload resolution,
  or constant evaluation.
- Do not force all template argument resolution to finish in Sema before HIR.
- Do not treat raw `TextId` spelling as a complete semantic template argument
  key.
- Do not remove diagnostic or debug renderings of template arguments.
- Do not perform broad Sema, HIR, backend, or parser rewrites outside the
  narrow call-site changes needed to consume structured argument keys.
- Do not weaken tests, mark supported cases unsupported, or rewrite
  expectations to avoid fixing template argument identity.

## Working Model

1. Inventory every place where `canonical_key` or rendered/debug template
   argument text participates in semantic instantiation identity.
2. Introduce an explicit structured representation for template argument keys,
   separating type arguments from value arguments and deferred carriers.
3. Migrate construction and comparison sites one caller family at a time.
4. Preserve display rendering only as a diagnostic or compatibility mirror with
   clear removal criteria.
5. Prove that semantic identity is stable across formatting differences and
   ambiguous rendered text without expectation downgrades.

## Execution Rules

- Use `rg` first to locate `canonical_key`,
  `make_template_instantiation_argument_key`, rendered/debug argument
  formatting, and template-instantiation key comparison sites.
- Keep each implementation packet tied to one representation boundary or caller
  family.
- Prefer structured domain metadata over testcase-shaped matching.
- If dependent arguments cannot be resolved immediately, carry structured
  deferred type/value payloads instead of canonical strings.
- If a string field must survive temporarily, label it as display-only or a
  compatibility mirror and record removal criteria in `todo.md`.
- For code-changing packets, get fresh build or compile proof plus the
  supervisor-chosen focused test subset.
- Escalate to broader parser/Sema/HIR validation after representation changes
  reach HIR or after deleting the old string-key path.

## Steps

### Step 1: Inventory Template Argument String Authority

Goal: identify every current semantic route that can still depend on rendered
or debug strings for template argument identity.

Primary targets:
- `TemplateInstantiationKey::Argument`
- `canonical_key` reads, writes, comparisons, ordering, hashing, and dumps
- `make_template_instantiation_argument_key` and nearby helper overloads
- parser, Sema, and HIR call sites that construct or consume template
  instantiation keys
- tests that currently exercise formatting-sensitive or ambiguous template
  argument identity

Actions:
- Locate all string-key construction and comparison sites.
- Classify each occurrence as semantic identity, display/debug-only,
  compatibility mirror, or dead/unreachable code.
- Distinguish type-argument and value-argument caller families.
- Record the first narrow implementation packet candidate in `todo.md`.

Completion check:
- `todo.md` lists reachable semantic string-authority sites, display-only or
  dead exclusions, and the first caller family to migrate.

### Step 2: Define Structured Argument Key Representation

Goal: replace the one-string argument key contract with explicit structured
type and value variants.

Primary targets:
- `TemplateInstantiationKey::Argument`
- key equality, ordering, hashing, and storage helpers
- diagnostic or dump rendering fields that must remain separate from semantic
  identity

Actions:
- Introduce structured variants for type arguments, value arguments, and
  deferred/dependent arguments where needed.
- Use type-domain identity or provisional/deferred type-domain keys for type
  arguments.
- Use value, constant, NTTP, expression, or deferred value metadata for value
  arguments.
- Keep any rendered text out of semantic equality and hashing.
- Preserve display rendering only through clearly named diagnostic or
  compatibility fields.

Completion check:
- The core key representation no longer exposes `std::string canonical_key` as
  the semantic template argument identity, and existing construction sites are
  either migrated or isolated behind explicit compatibility shims.

### Step 3: Migrate Parser and Type-Helper Construction Sites

Goal: make parser-side template argument key creation preserve structured
argument carriers instead of canonicalizing to rendered text.

Primary targets:
- `src/frontend/parser/impl/types/types_helpers.hpp`
- parser template argument parsing and instantiation-key construction callers
- structured `TypeSpec` or type-reference carriers
- structured expression, token, or NTTP carriers for value arguments

Actions:
- Thread structured type argument payloads into the new key variants.
- Thread structured value argument payloads into the new key variants.
- Preserve argument order and kind without merging type and value arguments
  into one debug-text path.
- Keep rendering helpers usable only for diagnostics, dumps, or explicit
  compatibility mirrors.
- Add or update focused probes for formatting-sensitive parser construction
  behavior.

Completion check:
- Parser/type-helper construction sites no longer derive semantic argument
  identity primarily from rendered `TypeSpec`, expression, or debug text, and
  focused parser-side proof is green.

### Step 4: Migrate Sema and Deferred Argument Handling

Goal: make Sema produce eager domain identities where possible and structured
deferred carriers where resolution must wait.

Primary targets:
- Sema template argument interpretation sites found in Step 1
- dependent type and value argument carriers
- owner template, parameter index/kind, and substitution context metadata

Actions:
- Resolve non-dependent type arguments into type-domain identity where the
  current infrastructure supports it.
- Resolve non-dependent value arguments into value, constant, or NTTP-domain
  identity where available.
- For dependent cases, preserve structured deferred carriers with enough
  context for late resolution.
- Avoid using `TextId` spelling alone as the complete semantic argument key.

Completion check:
- Sema-side callers can produce or preserve structured type/value argument keys
  without falling back to canonical rendered/debug strings for semantic
  equality.

### Step 5: Migrate HIR Late Instantiation Consumers

Goal: ensure HIR/template instantiation consumes structured resolved or
deferred argument keys without reparsing display text.

Primary targets:
- HIR template instantiation identity and lookup paths found in Step 1
- late deferred/dependent argument resolution routes
- any parser compatibility strings reachable from HIR semantic authority

Actions:
- Consume resolved Sema identities when present.
- Consume structured deferred type/value carriers otherwise.
- Remove or isolate HIR fallback paths that compare template arguments through
  canonical rendered strings.
- Add focused HIR/template probes that would fail if late instantiation splits
  or reparses formatted argument text.

Completion check:
- The migrated HIR/template-instantiation family carries structured argument
  identity end to end, and focused HIR/template proof is green without
  expectation weakening.

### Step 6: Remove or Label Remaining String Mirrors

Goal: make any surviving rendered template argument text explicitly
non-semantic.

Primary targets:
- compatibility shims introduced or retained during earlier steps
- dumps, diagnostics, debug rendering, and trace helpers
- obsolete comments that describe canonical strings as template argument
  authority

Actions:
- Delete obsolete string-key helpers after all semantic callers are gone.
- Rename or label surviving rendered fields as display-only or compatibility
  mirrors.
- Record remaining compatibility mirrors and concrete removal criteria in
  `todo.md` if they cannot be removed in this runbook.
- Inspect the diff for reviewer reject signals before requesting acceptance.

Completion check:
- No semantic parser, Sema, or HIR template instantiation identity route uses a
  rendered/debug string as the primary argument key, and any retained string is
  visibly non-semantic.

### Step 7: Prove Structured Argument Identity

Goal: validate the source idea acceptance criteria without broad unrelated
rewrites.

Actions:
- Run the supervisor-chosen focused parser/Sema/HIR template argument tests.
- Add or confirm probes where formatting differences would previously split one
  semantic argument or where ambiguous rendered text would collide.
- Run a broader parser/Sema/HIR validation checkpoint after the old string-key
  path is removed or fully isolated.
- Confirm the diff does not retain the old failure mode behind a renamed
  abstraction or weaker test contract.

Completion check:
- Focused tests prove template argument identity is structured rather than
  formatting-dependent.
- Broader parser/Sema/HIR validation remains green.
- The implementation satisfies the source idea acceptance criteria and avoids
  every reviewer reject signal.
