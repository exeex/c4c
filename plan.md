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

### Step 4: Migrate Parser Record Lookup Families

Goal: remove parser-owned rendered-string maps as semantic authority one lookup
family at a time.

Primary targets:
- `resolve_record_type_spec`
- parser `sizeof`, `alignof`, and `offsetof` record lookup paths
- any remaining parser support helper that resolves record layout through
  rendered-tag maps

Actions:
- Pick one lookup family per executor packet.
- Replace final record identity decisions with Sema-confirmed identity or a
  clearly bounded compatibility handoff.
- Keep parser provisional metadata available for parse-time needs only.
- Do not weaken behavior while migration is incomplete.

Completion check:
- The migrated lookup family no longer treats `defined_struct_tags` or
  `struct_tag_def_map` as authoritative semantic record identity.
- Any remaining parser mirror is explicitly documented or named as temporary
  compatibility, diagnostics, testing, or parser-local support.
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
  follow-up instead of expanding this plan.

Completion check:
- HIR no longer needs to rediscover record identity from parser string maps for
  the migrated paths, or a separate open idea documents the missing
  module-domain carrier.
- Build proof and the supervisor-selected broader validation checkpoint are
  green.

### Step 7: Final Boundary Review

Goal: verify the source idea acceptance criteria before closure is considered.

Actions:
- Confirm parser/Sema boundary documentation and code behavior match the
  provisional-carrier and Sema-table contracts.
- Confirm `struct_tag_def_map` and `defined_struct_tags`, if still present, are
  not accepted as semantic authority.
- Confirm focused tests cover namespace/scope collision, typedef interaction,
  and incomplete completion.
- Confirm no testcase-shaped shortcuts, unsupported downgrades, or rendered
  string identity fields were introduced.

Completion check:
- Acceptance criteria in the source idea are satisfied or remaining work is
  explicitly captured in `todo.md` for supervisor review.
- Supervisor can route reviewer and regression-guard checks before lifecycle
  closure.
