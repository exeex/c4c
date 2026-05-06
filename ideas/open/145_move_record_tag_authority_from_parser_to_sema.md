# Move Record Tag Authority From Parser To Sema

Status: Open
Created: 2026-05-06

Parent Ideas:
- `ideas/closed/139_parser_sema_rendered_string_lookup_removal.md`
- `ideas/closed/140_hir_legacy_string_lookup_metadata_resweep.md`
- `ideas/closed/141_typespec_tag_field_removal_metadata_migration.md`

## Goal

Move record tag semantic authority out of the parser and into Sema.

The parser should identify tag-shaped syntax and preserve structured source
carriers such as tag `TextId`, namespace context, qualifier `TextId` path,
record kind, and source spelling. It should not be the final authority for
whether a tag reference denotes a particular record declaration, whether a
forward declaration and definition merge, or whether a typedef routes to a
record type.

Sema should own the semantic record/tag table. That table should distinguish
record tag identity from typedef identity, value identity, and final display
spelling. HIR should consume sema-confirmed record identity and lower it into
module-domain record keys instead of rediscovering record identity from parser
string maps or rendered tags.

## Why This Idea Exists

Ideas 139 through 141 removed broad rendered-string lookup authority and
deleted `TypeSpec::tag`, but parser record handling still carries too much
semantic responsibility. The current parser keeps record definition state in
`defined_struct_tags` and `struct_tag_def_map`, and parser support helpers can
still resolve record layout from that rendered-tag map.

That makes tag handling hard to reason about because the parser is mixing three
different jobs:

- grammar recognition and parse-time disambiguation
- provisional AST/source metadata production
- semantic record/tag authority

The intended architecture is stricter:

- Lexer interns spelling into `TextId`.
- Parser recognizes tag syntax and emits structured provisional carriers.
- Sema builds the real record/tag table and resolves record identity across
  scope, namespace, typedef interaction, and incomplete-to-complete lifecycle.
- HIR receives sema-confirmed identity and produces HIR/module record keys.

This idea exists to establish that ownership boundary before the remaining
parser string-table cleanups are attempted.

## In Scope

- Inventory parser-owned record/tag authority under `src/frontend/parser`,
  especially `ParserDefinitionState::defined_struct_tags`,
  `ParserDefinitionState::struct_tag_def_map`, `resolve_record_type_spec`,
  parser `sizeof` / `alignof` / `offsetof` record lookup, and any code that
  treats rendered tag spelling as a record definition key.
- Inventory the current Sema record/tag mirrors under `src/frontend/sema`,
  especially `SemaStructuredNameKey`, `struct_defs_by_key_`,
  `complete_structs_by_key_`, `complete_unions_by_key_`,
  `structured_record_keys_by_tag_`, and string-keyed compatibility mirrors.
- Define a parser provisional record carrier contract that is sufficient for
  Sema to resolve tags without reparsing rendered spelling. The carrier should
  include record kind, tag `TextId`, namespace context, qualifier `TextId`
  sequence or equivalent key, source spelling for diagnostics/display, and a
  clear distinction between declaration and reference sites.
- Define the Sema-owned record/tag table contract. The semantic key must be
  record-domain specific and must not collide with typedef, value, namespace,
  or function tables. Forward declarations and definitions of the same record
  must merge through this table.
- Migrate one parser record lookup family at a time so parser-owned code uses
  provisional structured metadata only for parse-time needs, while final record
  identity decisions move to Sema.
- Preserve parser typedef disambiguation only where grammar requires it, and
  keep that responsibility visibly separate from record tag semantic authority.
- Add focused tests for same-spelling records in different scopes or
  namespaces, typedef-to-record interactions, and incomplete record completion
  where rendered spelling alone would choose the wrong record.
- Split downstream HIR carrier work into a separate idea if HIR cannot consume
  sema-confirmed record identity without a broader module-domain refactor.

## Out Of Scope

- Moving all parser typedef disambiguation to Sema. Parser may still need
  parse-time typedef knowledge to choose grammar branches.
- Broad HIR, LIR, BIR, backend, or codegen rewrites except to create a focused
  follow-up idea for consuming sema-confirmed record identity.
- Reintroducing `TypeSpec::tag` or any renamed rendered-string semantic field.
- Treating `TextId` alone as semantic record identity. `TextId` is spelling
  identity only; semantic identity comes from the Sema record/tag table.
- Removing diagnostic text, dump text, source spelling, ABI/link spelling,
  final emitted names, or display-only strings.
- Weakening supported tests, marking cases unsupported, or rewriting
  expectations to avoid fixing record/tag identity.

## Acceptance Criteria

- The parser/Sema boundary has a documented provisional record carrier and a
  documented Sema-owned record/tag table contract.
- Parser record/tag code no longer relies on `std::string` rendered tag maps as
  the semantic authority for metadata-backed record lookup paths.
- Sema owns the semantic table that resolves record tag declarations and
  references across namespace context, qualifier path, record kind, typedef
  interactions, and incomplete-to-complete declaration lifecycle.
- `struct_tag_def_map` and `defined_struct_tags`, if still present, are
  explicitly classified as compatibility, diagnostics, testing support, or
  temporary parser-local mirrors, not accepted as the authoritative record/tag
  table.
- Parser `sizeof` / `alignof` / `offsetof` behavior remains correct while any
  semantic record identity it requires is provided by structured metadata or
  by a clearly bounded compatibility path awaiting Sema/HIR handoff.
- Focused tests prove that same-spelling records in different scopes or
  namespaces do not collide, and that typedef names do not become tag-table
  identity.
- HIR handoff either consumes sema-confirmed record identity or has a separate
  open follow-up idea documenting the missing module-domain carrier.

## Reviewer Reject Signals

- A slice claims progress by renaming `struct_tag_def_map`,
  `defined_struct_tags`, or a rendered-tag helper while keeping the same
  string-keyed semantic authority.
- A route treats `TextId("A")` alone as record identity without a record-domain
  key that includes Sema ownership context.
- A change moves lookup code from parser to Sema but still resolves records by
  flattening or splitting rendered strings such as `A::B::C`.
- Parser typedef disambiguation and record tag semantic authority remain
  mixed in one table or helper after the slice claims the boundary is split.
- Forward declaration and definition merging is handled by named-case shortcuts
  rather than through the Sema record/tag table.
- Tests are weakened, marked unsupported, or rewritten around one known fixture
  instead of proving same-feature scope/namespace/typedef disagreement.
- A slice reintroduces `TypeSpec::tag` or a differently named rendered-string
  field as record identity.
- Broad HIR/codegen rewrites are mixed into this idea without proving they are
  required for the parser-to-Sema record tag authority boundary.
- The exact old failure mode remains behind a new abstraction name: a record
  lookup still succeeds primarily because a rendered tag string matches.
