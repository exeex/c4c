# Parser Rendered Record Template Lookup Mirror Cleanup Runbook

Status: Active
Source Idea: ideas/open/132_parser_rendered_record_template_lookup_mirror_cleanup.md

## Purpose

Remove parser semantic dependence on rendered record and template lookup mirror
strings now that structured identity carriers exist nearby.

## Goal

Make parser record-definition lookup, template specialization lookup, template
instantiation, and NTTP default-expression handling structured-primary when a
`QualifiedNameKey`, `TextId`, parser symbol id, direct `StructDef*`, or
equivalent carrier is available.

## Core Rule

Rendered record or template spelling may remain as diagnostics, debug output,
generated artifact spelling, or explicit compatibility fallback, but it must
not silently decide parser semantic lookup or substitution after structured
identity is available.

## Read First

- `ideas/open/132_parser_rendered_record_template_lookup_mirror_cleanup.md`
- `src/frontend/`
- parser definition/template state declarations and consumers for:
  - `ParserDefinitionState::defined_struct_tags`
  - `ParserDefinitionState::struct_tag_def_map`
  - `ParserTemplateState::template_struct_defs`
  - `ParserTemplateState::template_struct_specializations`
  - `ParserTemplateState::instantiated_template_struct_keys`
  - `ParserTemplateState::nttp_default_expr_tokens`

## Current Targets

- Parser record definition and lookup paths.
- Parser template primary and specialization lookup paths.
- Parser template instantiation keying and substitution paths.
- Parser NTTP default-expression token storage and replay paths.
- Focused parser/frontend tests that exercise same-feature cases around
  namespace-qualified records, template specializations, and NTTP defaults.

## Non-Goals

- Do not redesign parser template instantiation.
- Do not remove generated mangled names or final artifact spelling.
- Do not bulk delete compatibility fields before all semantic consumers have
  structured replacements.
- Do not change AST, Sema, or HIR ingress behavior except where the parser
  producer contract requires a structured-primary replacement.
- Do not weaken tests or replace semantic cleanup with expectation rewrites.

## Working Model

- `QualifiedNameKey`, `TextId`, parser symbol ids, and direct parser record or
  template references are semantic carriers.
- Rendered names are compatibility mirrors unless a call site proves no
  structured producer data exists.
- Existing string-keyed containers should either become structured-keyed,
  become compatibility mirrors checked against structured authority, or be
  removed after consumers migrate.
- Any remaining rendered fallback must have mismatch visibility through a
  counter, assertion, diagnostic, or narrowly documented fallback contract.

## Execution Rules

- Work in small code-changing packets; each packet needs fresh build or narrow
  test proof chosen by the supervisor.
- Trace all consumers before deleting a rendered mirror field.
- Prefer structured lookup APIs over reparsing rendered spelling into a key at
  the use site.
- Add nearby same-feature coverage before declaring a suspicious path repaired.
- Keep routine implementation progress in `todo.md`; update this runbook only
  if the route or proof contract changes.

## Ordered Steps

### Step 1: Inventory Parser Record And Template Mirror Consumers

Goal: Classify each suspicious rendered record/template mirror as semantic
authority, compatibility fallback, display/debug text, or generated spelling.

Primary target: parser definition and template state declarations plus all
read/write consumers of the six suspicious fields named in the source idea.

Actions:

- Inspect declarations and consumers for `defined_struct_tags`,
  `struct_tag_def_map`, `template_struct_defs`,
  `template_struct_specializations`, `instantiated_template_struct_keys`, and
  `nttp_default_expr_tokens`.
- Identify the structured carrier already available at each producer and
  consumer site.
- Record which paths are safe display/fallback and which need code changes.
- Do not edit implementation until the authority map is clear enough to avoid
  testcase-shaped patches.

Completion check:

- `todo.md` records the classified authority map and the first code-changing
  packet target, including the proof command the supervisor should delegate.

### Step 2: Make Record Definition Lookup Structured-Primary

Goal: Remove semantic lookup dependence on rendered record tag mirrors in
parser record definition and lookup paths.

Primary target: `ParserDefinitionState::defined_struct_tags` and
`ParserDefinitionState::struct_tag_def_map` consumers.

Actions:

- Replace semantic membership and lookup decisions with structured keys,
  parser symbol ids, or direct `StructDef*` references where available.
- Keep rendered tag spelling only for diagnostics, debug output, or explicit
  fallback when structured identity is unavailable.
- Add mismatch visibility for any fallback that remains.
- Add or update focused parser/frontend tests for namespace-qualified records
  and nearby colliding-name cases.

Completion check:

- Record definition lookup follows a structured-primary path and narrow tests
  prove namespace-qualified record lookup is not relying on rendered spelling.

### Step 3: Make Template Primary And Specialization Lookup Structured-Primary

Goal: Remove semantic lookup dependence on rendered template primary and
specialization mirrors.

Primary target: `template_struct_defs`, `template_struct_specializations`, and
related template lookup helpers.

Actions:

- Move template primary and specialization lookup to `QualifiedNameKey`,
  `TextId`, parser template ids, direct definition references, or equivalent
  structured carriers.
- Quarantine rendered-name maps as compatibility mirrors or retire them after
  all semantic consumers migrate.
- Preserve generated specialization spelling only as final artifact or debug
  text.
- Add tests for namespace-qualified template primaries and specializations,
  including nearby same-feature cases.

Completion check:

- Template primary and specialization resolution is structured-primary, and
  rendered spelling cannot silently select a different semantic template.

### Step 4: Make Instantiation And NTTP Defaults Structured-Primary

Goal: Remove semantic substitution dependence on rendered instantiation keys
and NTTP default-expression token caches.

Primary target: `instantiated_template_struct_keys`,
`nttp_default_expr_tokens`, and their instantiation/substitution consumers.

Actions:

- Trace instantiation cache keys and NTTP default-expression replay from
  parser production through use.
- Prefer structured template identity, parameter ids, argument identity, or
  direct parser references over rendered key strings.
- Preserve token text only as source replay/debug data, not as semantic
  template identity.
- Add focused tests for NTTP defaults and repeated instantiation cases that
  previously crossed the boundary through rendered spelling.

Completion check:

- Instantiation and NTTP default handling have structured-primary semantic
  keys, and tests cover at least one nearby same-feature case beyond the first
  suspicious path.

### Step 5: Consolidate Compatibility Mirrors And Final Proof

Goal: Leave the parser record/template mirror family either removed or clearly
documented as compatibility-only.

Actions:

- Remove dead rendered lookup mirrors after structured consumers are in place.
- For any remaining mirror, document or name it as non-authoritative and ensure
  fallback/mismatch behavior is visible.
- Run the supervisor-selected broader validation if multiple parser template
  paths changed.
- Summarize final coverage and remaining follow-up risk in `todo.md`.

Completion check:

- The source idea acceptance criteria are satisfied or any residual blocker is
  explicitly recorded for plan-owner review.
