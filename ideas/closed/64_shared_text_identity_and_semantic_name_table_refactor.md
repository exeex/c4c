# Shared Text Identity And Semantic Name Table Refactor

Status: Closed
Created: 2026-04-19
Last-Updated: 2026-04-19
Closed: 2026-04-19
Blocks:
- idea 62 prealloc CFG generalization and authoritative control-flow facts
- idea 63 complete phi legalization and parallel-copy resolution

Closure note:
- Shared text identity now lives above the frontend, semantic name-table/id
  types exist for backend-visible symbolic domains, and the first prepared
  control-flow, addressing, liveness, and join/compare surfaces carry typed
  ids instead of raw strings or bare `TextId` contracts.
- Close gate accepted on the canonical backend subset with
  `--allow-non-decreasing-passed`: `69` passed before and after, `4` known
  backend-route failures before and after, and no new failures or timeout
  regressions.
- Follow-on CFG ownership and phi work remain separate in ideas 62 and 63; no
  additional lifecycle scope is carried in this closed slice.

## Intent

`src/frontend/string_id_table.hpp` currently lives under the frontend even
though the underlying text-identity mechanism is a cross-pipeline concern.
Backend and prepare work now need stable symbolic identities for control-flow,
phi, and prepared metadata, but raw `std::string` is too heavy and raw
`TextId` is too weak because it carries storage identity without domain
meaning.

This idea extracts the shared text-id table into a layer above the frontend and
adds semantic name tables, similar in spirit to `LinkNameTable`, so passes can
traffic in domain ids such as `BlockLabelId` rather than bare strings or bare
text ids.

## Problem

The current layering has two structural issues:

- the text-id table sits in `src/frontend/`, which pushes backend/shared users
  toward an upside-down dependency
- prepared and backend metadata often want meaningful symbolic identity
  (`BlockLabelId`, `FunctionNameId`, `ValueNameId`, `SlotNameId`) rather than a
  plain text-storage key

Using raw `TextId` everywhere would only canonicalize bytes. It would not make
the ids carry domain meaning or prevent accidental cross-domain misuse.

## Goal

Move the text-id table into a shared layer and establish semantic name tables
that map shared text ids into domain-specific ids for control-flow, values,
slots, and related backend-visible names.

## Core Rule

Do not replace `std::string` with raw `TextId` as the final contract. The
refactor succeeds only when symbolic identities carry domain meaning through
named id types and dedicated tables.

## Primary Surfaces

- `src/frontend/string_id_table.hpp`
- the destination shared/support layer for the extracted text-id table
- `LinkNameTable`-adjacent naming utilities and their users
- backend/prealloc metadata that currently stores symbolic strings

## Concrete Direction

Expected end-state ingredients include:

```cpp
using FunctionNameId = ...;
using BlockLabelId = ...;
using ValueNameId = ...;
using SlotNameId = ...;

class BlockLabelTable {
 public:
  BlockLabelId intern(TextId text_id);
  TextId text(BlockLabelId id) const;
};
```

The exact representation may differ, but the model should be:

- one shared text table for canonical byte storage
- one or more semantic name tables that assign domain ids from shared text ids
- downstream structs carry semantic ids, not `std::string` and not bare
  `TextId`

## Desired End State

- the text-id table no longer lives under `src/frontend/`
- frontend, mid-pipeline, and backend code can depend on the shared text table
  without layering inversion
- semantic tables provide typed identities such as `BlockLabelId`
- prepared/backend contracts use semantic ids where they currently use symbolic
  strings

## Acceptance Shape

This idea is satisfied when the shared text-id infrastructure has been moved
out of the frontend, semantic name tables exist for the needed symbolic
domains, and downstream users can adopt typed ids without depending on raw
strings or raw text ids as their public contract.

## Execution Order Note

This idea should run before idea 62 and idea 63. Those follow-on ideas should
consume semantic ids such as `BlockLabelId` and `FunctionNameId` instead of
inventing new string-keyed prepared contracts first and migrating later.

## Non-Goals

- changing frontend parsing behavior
- conflating symbolic-id refactoring with unrelated CFG or phi algorithm work
- introducing one mega-id type that erases domain meaning
