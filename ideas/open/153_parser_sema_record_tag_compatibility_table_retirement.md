# Parser Sema Record Tag Compatibility Table Retirement

Status: Open
Created: 2026-05-08

Parent Ideas:
- `ideas/closed/145_move_record_tag_authority_from_parser_to_sema.md`
- `ideas/closed/151_parser_out_of_class_owner_probe_token_sequence.md`
- `ideas/closed/152_hir_lir_rendered_owner_compatibility_retirement.md`

## Goal

Retire parser rendered-record-tag compatibility tables as semantic authority by
making parser record carriers consistently feed Sema-owned record-domain tables.

Parser may keep rendered record spelling for diagnostics, AST legacy mirrors,
template instantiation display names, and compatibility probes. It should not
use `std::string` record tags or mangled rendered names as the primary table key
for record identity or completion when a structured record carrier is available.

## Why This Idea Exists

The parser has already moved many record paths toward `TypeSpec::record_def`,
`tag_text_id`, namespace context, qualifier `TextId`s, and owner keys. The
remaining obvious compatibility surface is still visible in
`ParserDefinitionState`:

- `defined_struct_tags`
- `struct_tag_def_map`

Those maps are documented as rendered-tag compatibility mirrors, but template
instantiation and member typedef paths can still insert or probe mangled
rendered names. This is the next record-identity boundary after ideas 145,
151, and 152.

## Responsibility Split

Parser owns partial syntax carriers:

- record kind
- source spelling `TextId`
- namespace/global qualifier context
- qualifier `TextId` sequence
- provisional `record_def` pointer when the definition node is locally known
- rendered spelling mirrors for diagnostics and compatibility

Sema owns record-domain meaning:

- final record identity and completion
- tag/typedef/namespace disambiguation
- owner-indexed record tables
- canonical lookup for parser-produced record carriers

HIR may consume Sema-resolved record identity or structured deferred carriers.
HIR should not require parser rendered tags to rediscover record meaning.

## In Scope

- Inventory every parser read/write of `struct_tag_def_map` and
  `defined_struct_tags`.
- Classify each use as semantic authority, compatibility mirror, final/display
  spelling, test hook, or temporary fallback for incomplete metadata.
- Route record identity checks through `TypeSpec::record_def`, `tag_text_id`,
  qualifier metadata, or Sema record-domain lookup before rendered tag maps.
- Audit template instantiated record insertion and lookup where `mangled`
  rendered names are used as keys.
- Add focused tests or probes where same-spelling or stale rendered record tags
  would choose the wrong record if the compatibility map were authoritative.
- Document any retained rendered tag map use with owner, limitation, and
  removal condition.

## Out Of Scope

- Removing all `Node::name`, `Node::unqualified_name`, or diagnostic record
  spellings.
- Treating `TextId` alone as final record semantics across scopes.
- Full HIR/LIR aggregate layout cleanup already covered by idea 152.
- Broad Sema redesign unrelated to record-domain table authority.
- Weakening tests or marking supported record cases unsupported.

## Acceptance Criteria

- Parser record semantic lookup prefers structured record carriers or Sema
  record-domain tables before rendered tag maps.
- `struct_tag_def_map` and `defined_struct_tags`, if retained, are classified
  compatibility/display/test mirrors with removal conditions.
- Template instantiated record identity does not depend on a rendered mangled
  tag string when structured owner/key metadata is available.
- Tests cover at least one stale or ambiguous rendered-tag case.
- The route preserves parser/sema responsibility: parser carries partial
  structure; Sema owns final record meaning.

## Reviewer Reject Signals

- A slice renames `struct_tag_def_map` while still using rendered strings as
  record identity authority.
- A change replaces rendered strings with raw `TextId` alone and claims that is
  semantic record identity.
- Template instantiated records remain keyed only by rendered mangled spelling
  after the slice claims structured record authority.
- Tests are weakened or narrowed around one fixture instead of proving record
  domain behavior.
