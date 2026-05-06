# Move Record Tag Authority From Parser To Sema

Status: Active
Source Idea: ideas/open/145_move_record_tag_authority_from_parser_to_sema.md

## Purpose

Move semantic record/tag authority out of the parser and into Sema while keeping
parser-owned grammar and provisional source-carrier responsibilities explicit.

## Goal

Parser record/tag code should emit structured provisional metadata; Sema should
own semantic record identity, declaration/reference resolution, and
forward-declaration-to-definition merging.

## Core Rule

Do not replace rendered-string authority with renamed rendered-string authority.
`TextId` is spelling identity only; semantic record identity must come from a
Sema-owned record-domain key/table that cannot collide with typedef, value,
namespace, or function identity.

## Read First

- `ideas/open/145_move_record_tag_authority_from_parser_to_sema.md`
- `src/frontend/parser`
- `src/frontend/sema`

## Current Targets

- Parser record/tag authority:
  - `ParserDefinitionState::defined_struct_tags`
  - `ParserDefinitionState::struct_tag_def_map`
  - `resolve_record_type_spec`
  - parser `sizeof`, `alignof`, and `offsetof` record lookup paths
  - any rendered-tag spelling used as a record definition key
- Sema record/tag mirrors:
  - `SemaStructuredNameKey`
  - `struct_defs_by_key_`
  - `complete_structs_by_key_`
  - `complete_unions_by_key_`
  - `structured_record_keys_by_tag_`
  - string-keyed compatibility mirrors

## Non-Goals

- Do not move all parser typedef disambiguation to Sema; parser may retain the
  parse-time typedef knowledge needed to choose grammar branches.
- Do not reintroduce `TypeSpec::tag` or an equivalent rendered-string semantic
  field.
- Do not treat `TextId` alone as semantic record identity.
- Do not remove diagnostic text, dump text, source spelling, ABI/link spelling,
  final emitted names, or display-only strings.
- Do not mix broad HIR, LIR, BIR, backend, or codegen rewrites into this plan.
  Create a follow-up open idea if HIR needs a broader module-domain carrier.
- Do not weaken tests, mark supported paths unsupported, or rewrite
  expectations to avoid fixing record/tag identity.

## Working Model

- Lexer interns spelling into `TextId`.
- Parser recognizes tag-shaped syntax and produces provisional structured
  carriers, including record kind, tag `TextId`, namespace context, qualifier
  `TextId` path or equivalent key, source spelling, and declaration/reference
  site information.
- Sema builds the authoritative record/tag table, resolves semantic identity
  across scope and namespace context, keeps typedef/value/function identity
  separate, and merges incomplete declarations with definitions.
- HIR consumes sema-confirmed record identity. If this cannot happen without a
  broader HIR carrier refactor, split that downstream work into a separate
  `ideas/open/` initiative.

## Execution Rules

- Work one lookup family at a time.
- Keep parser typedef disambiguation visibly separate from record tag semantic
  authority.
- Keep compatibility mirrors clearly labeled when they must remain temporarily.
- Prefer structured keys and typed carriers over flattening or reparsing
  rendered strings such as `A::B::C`.
- Add focused proof for same-spelling records in different scopes or
  namespaces, typedef-to-record interactions, and incomplete record completion.
- For each code-changing step, run a fresh build or compile proof plus the
  narrow tests delegated by the supervisor.

## Ordered Steps

### Step 1: Inventory Current Authority

Goal: identify every parser and Sema surface that currently participates in
record/tag semantic authority.

Primary targets:
- `src/frontend/parser`
- `src/frontend/sema`

Actions:
- Inspect parser ownership of `defined_struct_tags`, `struct_tag_def_map`,
  `resolve_record_type_spec`, and parser record lookup paths for `sizeof`,
  `alignof`, and `offsetof`.
- Inspect Sema record/tag state around `SemaStructuredNameKey`,
  `struct_defs_by_key_`, `complete_structs_by_key_`,
  `complete_unions_by_key_`, `structured_record_keys_by_tag_`, and
  string-keyed compatibility mirrors.
- Classify each discovered surface as grammar disambiguation, provisional
  carrier production, semantic authority, diagnostics/display, compatibility,
  or testing support.

Completion check:
- `todo.md` records the authoritative inventory and names the first lookup
  family to migrate.
- No implementation files are changed unless the executor packet explicitly
  includes a small documentation-only classification update.

### Step 2: Define Parser Provisional Carrier Contract

Goal: make the parser-to-Sema record carrier explicit enough for Sema to
resolve tags without reparsing rendered spelling.

Primary targets:
- parser AST/type carrier definitions used for record declarations and
  references
- Sema entry points that consume parser record metadata

Actions:
- Identify the minimal structured carrier fields: record kind, tag `TextId`,
  namespace context, qualifier `TextId` sequence or equivalent key, source
  spelling, source location, and declaration/reference role.
- Keep display spelling separate from semantic identity.
- Preserve parser-local typedef disambiguation only where grammar needs it.

Completion check:
- The code has a documented structured carrier contract or a tightly scoped
  implementation path for that contract.
- No semantic record lookup depends on a newly introduced rendered-string field.
- Build or compile proof for touched frontend files is green.

### Step 3: Define Sema Record/Tag Table Contract

Goal: make Sema the semantic owner for record tag declarations, references,
and incomplete-to-complete merging.

Primary targets:
- `src/frontend/sema`

Actions:
- Establish or tighten a record-domain key/table that distinguishes record tag
  identity from typedef, value, namespace, and function identity.
- Route forward declarations and definitions of the same record through this
  table.
- Keep any string-keyed mirrors explicitly bounded as compatibility,
  diagnostics, testing, or temporary handoff support.

Completion check:
- Sema owns the semantic table used for record tag identity decisions.
- Same-spelling records in distinct scopes or namespaces can be represented
  without collision.
- Build or compile proof plus focused Sema/parser tests are green.

### Step 4: Migrate Parser Record Lookup Families Without Parser Identity Authority

Goal: remove parser-owned rendered-string and `TextId`-only maps as semantic
authority one lookup family at a time.

Primary targets:
- `resolve_record_type_spec`
- parser `sizeof`, `alignof`, and `offsetof` record lookup paths
- any remaining parser support helper that resolves record layout through
  rendered-tag maps

Actions:
- Execute this step in substeps so the reviewed route does not regress EASTL
  parse coverage while removing parser-map identity authority:
  - Step 4A: supply the missing parser/Sema carrier for record references that
    reach `resolve_record_type_spec` without `record_def`. The carrier must
    preserve enough authoritative metadata for the later lookup decision:
    record kind, namespace context or Sema-owned owner key, global qualification
    state, qualifier `TextId` sequence or equivalent canonical key, base tag
    `TextId`, source location, and declaration/reference role. The immediate
    proof target is the EASTL `vector` and `reverse_iterator` path that becomes
    incomplete when the unique same-`TextId` parser-map fallback is removed.
  - Step 4B: repair `resolve_record_type_spec` so it does not accept a
    context-defaulted unique same-`TextId` candidate from `struct_tag_def_map`
    as semantic identity. Delete/narrow that parser compatibility path to full
    structured carrier matches, or replace the final decision with a Sema-owned
    structured record lookup.
  - Step 4C: migrate the remaining parser `sizeof`, `alignof`, `offsetof`, and
    support-helper lookup families one executor packet at a time.
  - Step 4C repair route: if a migrated parser lookup exposes nested template
    record carriers that arrive in HIR as `struct<?>` or `*_T_void`, preserve
    the structured carrier graph through parser-to-HIR lowering. Carry
    `tpl_struct_origin_key` and nested `TemplateArgRef` metadata as typed data;
    do not rebuild semantic record identity from `TemplateArgRef::debug_text`,
    `@origin:args` strings, rendered module names, encoded template instance
    names, or `module_->struct_defs.find(rendered_name)` recovery.
- When namespace context is present, check the full structured metadata needed
  by the record-domain contract: record kind, namespace context,
  global-qualification state, qualifier `TextId` sequence or equivalent
  canonical key, and base tag `TextId`. Do not use namespace context alone as
  permission to ignore conflicting structured qualifier metadata unless Sema
  owns and documents that canonicalization.
- Replace final record identity decisions with Sema-confirmed identity or a
  clearly bounded compatibility handoff.
- Keep parser provisional metadata available for parse-time needs only.
- Do not add or widen parser-map fallback authority based on unique `TextId`
  matches, rendered key order, or absence of competing parser-map entries.
- Do not serialize a structured pending template record carrier into display
  text and later parse that text back into identity. `debug_text` may remain
  diagnostic/display metadata only.
- Keep codegen aggregate-store fallout out of Step 4C unless the executor
  proves it is directly required by the structured carrier handoff. Otherwise
  leave it for a separate supervisor-routed slice or follow-up idea.
- Do not weaken behavior while migration is incomplete.

Completion check:
- Step 4A is complete only when the EASTL record references that currently lack
  `record_def` also carry authoritative namespace/owner metadata or a
  Sema-owned record identity handoff, without restoring TextId-only lookup.
- The migrated lookup family no longer treats `defined_struct_tags` or
  `struct_tag_def_map` as authoritative semantic record identity.
- `resolve_record_type_spec` no longer resolves context-defaulted structured
  carriers through parser-map uniqueness by `TextId`, unless the path is an
  explicitly temporary compatibility hole recorded in `todo.md` with a bounded
  deletion packet.
- Structured lookup proof includes mismatched namespace/global/qualifier
  metadata cases and rejects them unless a Sema-owned canonical key proves they
  are the same record.
- Any remaining parser mirror is explicitly documented or named as temporary
  compatibility, diagnostics, testing, or parser-local support.
- Nested template record carriers such as `Box<Pair<int>>` or
  `Pair<Box<int>>` lower with concrete structured HIR record identity without
  reparsing `debug_text`, `@origin:args`, or rendered module names.
- Narrow behavior tests and build proof are green.

### Step 5: Add Identity-Focused Tests

Goal: prove the new boundary fixes identity cases that rendered spelling alone
cannot model safely.

Primary targets:
- existing frontend/parser/Sema test suites

Actions:
- Add focused tests for same-spelling records in different scopes or
  namespaces.
- Add tests for typedef-to-record interactions where typedef identity must not
  become tag identity.
- Add tests for incomplete record completion through the Sema record/tag table.
- Avoid expectation rewrites that merely hide failures.

Completion check:
- Tests fail on the old rendered-string authority path or directly exercise the
  new structured/Sema-owned path.
- Narrow test proof is green after the implementation.
- No supported-path test is downgraded without explicit approval.

### Step 6: Decide HIR Handoff

Goal: either make HIR consume sema-confirmed record identity for this boundary
or split downstream carrier work into a separate source idea.

Primary targets:
- HIR record carrier entry points touched by parser/Sema record identity
  handoff

Actions:
- Inspect whether HIR can consume sema-confirmed record identity without a
  broad module-domain refactor.
- If a small handoff is sufficient, implement only that bounded handoff.
- If broader HIR carrier work is required, create a separate `ideas/open/`
  follow-up instead of expanding this plan. The follow-up must include
  reviewer reject signals against debug-text reparsing, rendered module-name
  identity recovery, testcase-shaped fixture shortcuts, and broad backend/codegen
  rewrites claimed as record-carrier progress.

Completion check:
- HIR no longer needs to rediscover record identity from parser string maps for
  the migrated paths, or a separate open idea documents the missing
  module-domain carrier.
- Build proof and the supervisor-selected broader validation checkpoint are
  green.

### Step 7: Repair Sema TypeSpec Record Key Width Before Closure

Goal: close the reviewer-identified Sema record completion key-width gap before
the source idea is considered complete.

Primary targets:
- `src/frontend/sema/validate.cpp`
- `src/frontend/sema/validate.hpp`
- focused Sema/frontend tests that can construct or observe mismatched
  namespace/base-`TextId` records with different global qualification or
  qualifier metadata

Trigger:
- `review/step7_final_boundary_review.md` found that
  `structured_record_key_for_type()` and
  `structured_record_key_for_type_metadata()` rebuild record completion keys
  from only `namespace_context_id` plus `tag_text_id`, while
  `SemaStructuredNameKey` is documented as also carrying
  `is_global_qualified` and `qualifier_text_ids`.

Actions:
- Inspect the record completion lookup path that consumes TypeSpec record
  metadata for `complete_structs_by_key_`, `complete_unions_by_key_`, and
  related structured record tables.
- Preserve `is_global_qualified` and the qualifier `TextId` sequence when
  extracting a `SemaStructuredNameKey` from `TypeSpec` / TypeSpec metadata, or
  explicitly document and test a Sema-owned canonicalization rule that proves
  those fields are redundant for this completion path.
- Prefer reusing the generic structured-key extraction behavior that already
  preserves global and qualifier metadata when it fits the record-domain
  contract.
- Add focused proof where the same namespace context and base tag `TextId` do
  not complete against a record when global qualification or qualifier metadata
  disagrees, unless the executor implements a documented Sema-owned
  canonicalization rule that makes that disagreement impossible or redundant.
- Keep this packet inside Sema key extraction and its direct proof. Do not
  widen parser map authority, recover identity from rendered strings, or mix in
  unrelated HIR/backend/codegen work.

Completion check:
- Sema record completion keys no longer drop qualifier/global metadata from
  metadata-backed `TypeSpec` record references, or the code contains a
  documented Sema-owned canonicalization rule with focused tests proving why the
  dropped fields cannot change identity in this path.
- Focused tests cover mismatched qualifier/global metadata for same
  namespace/base-`TextId` record completion and fail on the weaker key-width
  behavior.
- Existing namespace/scope collision, typedef interaction, and incomplete
  completion coverage remains green.
- Build proof plus the supervisor-selected narrow Sema/frontend subset is
  green and written to canonical `test_after.log`.
- No expectations are weakened, no supported path is marked unsupported, and no
  named-case shortcut or rendered-string recovery is introduced.

### Step 8: Final Boundary Review

Goal: verify the source idea acceptance criteria before closure is considered.

Actions:
- Confirm parser/Sema boundary documentation and code behavior match the
  provisional-carrier and Sema-table contracts.
- Confirm `struct_tag_def_map` and `defined_struct_tags`, if still present, are
  not accepted as semantic authority.
- Confirm Sema record completion key extraction preserves or explicitly
  canonicalizes qualifier/global metadata for metadata-backed `TypeSpec`
  records.
- Confirm focused tests cover namespace/scope collision, typedef interaction,
  incomplete completion, and qualifier/global mismatch on record completion.
- Confirm no testcase-shaped shortcuts, unsupported downgrades, or rendered
  string identity fields were introduced.
- Normalize canonical closure proof logs: `test_before.log` and
  `test_after.log` should cover matching acceptance scope before lifecycle
  closure is requested.

Completion check:
- Acceptance criteria in the source idea are satisfied or remaining work is
  explicitly captured in `todo.md` for supervisor review.
- Supervisor can route reviewer and regression-guard checks before lifecycle
  closure.
