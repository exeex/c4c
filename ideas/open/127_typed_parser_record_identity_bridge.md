# Typed Parser Record Identity Bridge

Status: Open
Created: 2026-04-29

Parent Ideas:
- [123_parser_legacy_string_lookup_removal_convergence.md](/workspaces/c4c/ideas/open/123_parser_legacy_string_lookup_removal_convergence.md)

## Goal

Add a typed parser record identity bridge through `TypeSpec` and the downstream
type consumers that currently receive only `TypeSpec::tag` as record identity.

The bridge should let parser-owned semantic record lookup use a real record
identity, such as a parsed record definition `Node*` or stable parser record id,
while preserving `TypeSpec::tag` as final spelling and compatibility output.

## Why This Idea Exists

The parser string lookup cleanup found that `DefinitionState::struct_tag_def_map`
is not a pure parser text map. It is the compatibility bridge that recovers
record definitions for `sizeof`, `alignof`, `offsetof`, incomplete object
checks, template-instantiated records, member typedef lookup, and direct
template emission.

Those call paths receive record identity through `TypeSpec::tag`, so deleting
or renaming the string map would only move the legacy spelling authority rather
than replacing it with semantic record identity.

## Scope

- Inventory every `TypeSpec::tag` producer and consumer that participates in
  record identity, layout, `offsetof`, incomplete object checks, template
  instantiation, member typedef lookup, or parser/HIR boundary type surfaces.
- Define the smallest typed record identity payload that can travel with
  struct and union `TypeSpec` values without breaking final spelling,
  diagnostics, emitted text, or compatibility bridges.
- Add parser-side population and propagation for that typed identity where a
  record definition, tag declaration, template-instantiated record, or stable
  record key is known.
- Convert parser record lookup users to prefer typed identity before falling
  back to `TypeSpec::tag` compatibility lookup.
- Keep `DefinitionState::struct_tag_def_map` as an explicit compatibility and
  final-spelling mirror until all covered semantic users have typed identity.
- Update focused parser/frontend tests so drifted rendered tags cannot override
  the typed record identity for the converted paths.

## Out Of Scope

- HIR, LIR, or BIR legacy string lookup cleanup beyond preserving the required
  type boundary contract.
- Broad type-system redesign unrelated to struct/union record identity.
- Removing `TypeSpec::tag` as spelling, diagnostics, emitted text, or
  compatibility payload.
- Replacing semantic record identity with `TextId` alone.
- Weakening supported behavior or marking tests unsupported to make the bridge
  pass.

## Acceptance Criteria

- Struct and union `TypeSpec` values can carry typed record identity separately
  from `TypeSpec::tag` spelling.
- Parser layout, `offsetof`, incomplete-object, template-instantiated record,
  and member typedef lookup paths prefer typed record identity when available.
- `DefinitionState::struct_tag_def_map` remains only as compatibility/final
  spelling fallback for paths not yet carrying typed identity.
- Focused tests prove a stale or drifted rendered tag cannot override a covered
  typed record identity lookup.
- The original parser cleanup idea can resume with `struct_tag_def_map`
  classified as compatibility/final-spelling mirror instead of semantic
  authority.
