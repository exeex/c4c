# HIR To LIR Link-Name Table And Backend Symbol Ids

Status: Active
Source Idea: ideas/open/14_hir_to_lir_link_name_table_and_backend_symbol_ids.md

## Purpose

Introduce a dedicated `LinkNameId` / `LinkNameTable` contract at the HIR-to-LIR
boundary so final link-visible symbol identity stops flowing through the
pipeline as ad hoc strings.

## Goal

Materialize final logical symbol names in HIR, carry stable ids through at
least one real HIR->LIR path, and resolve spelling only at late text-emitting
consumer boundaries without collapsing the new id space into `TextId` or
parser-time `SymbolId`.

## Core Rule

Do not accept string shuffling as progress.

The run is only complete when the pipeline gains a real semantic id boundary:

- HIR owns `LinkNameId` materialization for final logical symbol names
- LIR forwards that identity through ids on real carriers
- late consumers resolve ids back to text at the final spelling boundary

## Read First

- `ideas/open/14_hir_to_lir_link_name_table_and_backend_symbol_ids.md`
- `src/frontend/string_id_table.hpp`
- `src/frontend/parser/parser.hpp`
- `src/frontend/hir/hir_ir.hpp`
- `src/frontend/hir/hir_build.cpp`
- `src/frontend/hir/hir_functions.cpp`
- `src/frontend/hir/hir_types.cpp`
- `src/frontend/hir/hir_lowerer_internal.hpp`
- `src/codegen/lir/ir.hpp`
- `src/codegen/lir/hir_to_lir.cpp`
- backend and LIR text-emission surfaces that currently consume final symbol
  strings

## Current Targets

- a `TextTable`-backed `LinkNameId` / `LinkNameTable` definition with a clear
  invalid sentinel
- HIR symbol carriers for emitted function/global/template-instantiation names
- HIR ownership/threading of the link-name table at the materialization
  boundary
- one bounded HIR->LIR path that forwards link-visible identity via ids
- late text-emitting consumers that need stable `LinkNameId -> TextId ->
  spelling` lookup

## Non-Goals

- replacing every string field across HIR, LIR, and backend code in one pass
- merging `LinkNameId` with parser/source-atom `SymbolId`
- merging `LinkNameId` with `TextId`
- pulling backend-private temporary names, SSA temps, block labels, or other
  non-link-visible names into the first slice
- moving target-specific decoration into HIR or LIR
- proving success only by downgrading tests or adding testcase-shaped
  dispatcher shortcuts

## Working Model

### Semantic split

- `TextTable` owns bytes
- `SymbolId` remains source-atom identity
- `LinkNameId` becomes final logical symbol identity for emitted code

### Materialization boundary

- HIR is the first stage allowed to intern final logical names into
  `LinkNameTable`
- HIR module/lowering ownership must keep the same logical-name space
  available when lowering into LIR
- later consumers resolve `LinkNameId` back to text only when they actually
  emit textual or target-spelled symbols

### Migration shape

- parallel id fields are acceptable in the first landing
- one real HIR->LIR path using ids is more important than broad string purge
- backend-private temporary naming stays out of scope unless a carrier is
  already link-visible

## Execution Rules

- keep the new table `TextTable`-backed; do not introduce a second owning
  string pool
- add semantic ids and table threading before attempting wide consumer
  migration
- prefer bounded carrier migration with explicit id forwarding over sweeping
  string rewrites
- keep validation semantic: build, prove a real HIR->LIR symbol path, then
  expand to broader frontend/backend checkpoints only when the blast radius
  justifies it
- reject testcase-overfit changes such as brittle emitted-text shape matching
  or named-case-only backend shortcuts

## Ordered Steps

### Step 1: Define The Link-Name Id Space

Goal:
Add the `LinkNameId` / `LinkNameTable` data model as a `TextTable`-backed
semantic id space with a clear invalid sentinel.

Primary Target:
- `src/frontend/string_id_table.hpp`
- any adjacent shared id-table helpers needed for the new table

Actions:
- define `LinkNameId` and its invalid value
- add `LinkNameTable` with `TextId`-backed interning and lookup surfaces
- keep ownership with the existing TU text table rather than a second byte
  store

Completion Check:
- the repo has an explicit link-name id space distinct from both `TextId` and
  `SymbolId`
- lookup from `LinkNameId` to `TextId` and spelling is available through one
  stable table API

### Step 2: Materialize Link Names In HIR

Goal:
Make HIR the first stage that interns final logical symbol names and stores the
resulting ids on emitted symbol carriers.

Primary Target:
- `src/frontend/hir/hir_ir.hpp`
- `src/frontend/hir/hir_build.cpp`
- `src/frontend/hir/hir_functions.cpp`
- `src/frontend/hir/hir_types.cpp`
- `src/frontend/hir/hir_lowerer_internal.hpp`

Actions:
- add parallel `link_name_id` fields for the first HIR carriers that already
  represent final emitted symbol identity
- thread `LinkNameTable` through HIR ownership/lowering state
- ensure HIR template instantiation mangling interns the final logical name via
  the new table at materialization time

Completion Check:
- HIR carriers for emitted functions/globals/templates can hold stable
  `LinkNameId` values
- HIR owns the interning boundary for final logical symbol names

### Step 3: Forward One Real HIR->LIR Path Through Ids

Goal:
Prove the contract is real by forwarding link-visible identity through ids on
at least one bounded HIR->LIR path.

Primary Target:
- `src/codegen/lir/ir.hpp`
- `src/codegen/lir/hir_to_lir.cpp`

Actions:
- extend the relevant LIR carrier(s) with parallel link-name id fields
- forward ids from HIR to LIR explicitly on one bounded direct-symbol path
- keep bridging string fields only where still required by incomplete
  downstream migration

Completion Check:
- at least one real HIR->LIR path carries link-visible symbol identity as ids
  rather than only raw strings
- the new path is explicit in code and does not depend on a side-channel string
  reconstruction

### Step 4: Resolve At Late Consumer Boundaries

Goal:
Use the shared table to resolve `LinkNameId` back to text only where text or
target spelling is actually emitted.

Primary Target:
- LIR text emission and/or backend symbol spelling helpers that currently
  consume final logical symbol strings

Actions:
- thread access to the shared link-name table into the late consumer boundary
- resolve `LinkNameId -> TextId -> spelling` at the consumer
- keep target-specific decoration backend-only

Completion Check:
- at least one late consumer resolves final logical symbol names via
  `LinkNameId`
- target spelling differences still apply only at the backend/text edge

### Step 5: Raise Validation To A Route Checkpoint

Goal:
Validate that the new id boundary works across the intended symbol families
without expanding into a repo-wide string rewrite.

Actions:
- keep packet-level proof focused on the changed build and the narrowest test
  that exercises the new id path
- add or update coverage for ordinary function/global names and whichever
  bounded HIR->LIR path was migrated
- before treating the route as near-close, run broader frontend/HIR/LIR/backend
  coverage that exercises template-instantiation, operator-name, qualified-name,
  or anonymous-namespace symbol flows as appropriate

Completion Check:
- focused proof demonstrates a real id-carrying HIR->LIR symbol path
- broader validation has exercised the route families needed for near-close
  confidence

## Validation

### Focused Proof

- `cmake --build --preset default -j4`
- the narrowest repo-native test that proves the chosen HIR->LIR link-name id
  path

### Broader Checkpoint

- `cmake --build --preset default -j4`
- matching frontend/HIR/LIR/backend coverage for final symbol-name handling,
  expanded only after a meaningful shared-pipeline slice lands

## Done Condition

This runbook is complete only when:

- HIR has an explicit `LinkNameId` / `LinkNameTable` concept for final logical
  symbol names
- at least one real HIR->LIR path forwards link-visible symbol identity as ids
- later text-emitting consumers can resolve `LinkNameId` through a stable
  lookup boundary
- target-specific decoration remains isolated to the backend/text spelling edge
- backend-private temporary names remain out of scope for the first slice
