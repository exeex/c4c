# Local Block Enum Scope Static Eval Structured Mirrors

Status: Open
Created: 2026-05-11

Parent Ideas:
- `ideas/closed/164_sema_type_utils_static_eval_structured_lookup.md`

## Goal

Retire or narrow the remaining local/block enum-scope static-eval dependence on
mutable rendered enum mirrors by giving those scopes structured or TextId-aware
enum metadata where the surrounding sema/HIR context can provide it.

## Why This Idea Exists

Idea 164 converted the covered global/static-member `type_utils` static-eval
paths and fenced rendered lookup as a compatibility bridge. During closure,
local/block enum-scope mirrors were identified as adjacent residual work: they
can still depend on mutable `enum_consts_` save/restore behavior rather than a
structured enum-domain carrier.

This is separate from idea 164 because local/block enum scopes may require
different ownership and lifetime handling than global/static-member lookup.

## In Scope

- Inventory local and block enum paths that feed static-eval enum constants
  through rendered compatibility maps or mutable mirror state.
- Identify which local/block paths can carry structured enum-domain metadata,
  scoped `TextId` metadata, or neither.
- Convert metadata-capable local/block paths to structured lookup without
  fabricating incomplete enum identity.
- Keep no-metadata paths on an explicit compatibility bridge with a written
  owner, limitation, and removal condition.
- Add focused tests where same-spelled local/block enum constants must not
  collide during static evaluation when structured metadata is complete.

## Out Of Scope

- Reopening global/static-member lookup conversion completed by idea 164.
- Reworking the full consteval interpreter.
- Removing diagnostic or source spelling strings.
- Broad backend or ABI changes.

## Acceptance Criteria

- Local/block enum static-eval callers are inventoried and classified by
  structured metadata capability.
- Metadata-capable local/block paths use structured or scoped TextId-aware
  lookup and fail closed on complete structured misses.
- Any retained mutable rendered mirror or string map path is documented as
  compatibility with a removal condition.
- Focused tests prove local/block same-spelled enum constants do not collide in
  covered structured paths.

## Reviewer Reject Signals

- The change only renames or relocates the mutable `enum_consts_` mirror while
  preserving rendered spelling as ordinary local/block enum authority.
- A raw `TextId` is treated as sufficient for local/block enum lookup without
  enough enum/domain context to distinguish scopes.
- Tests prove only a named testcase while nearby local/block same-spelled enum
  cases remain unexamined.
- Unsupported expectations are downgraded or weakened to avoid local/block
  enum-scope behavior.
- The route reopens idea 164's global/static-member conversion instead of
  focusing on local/block ownership and lifetime.
