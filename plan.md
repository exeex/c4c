# Typed Parser Record Identity Bridge Runbook

Status: Active
Source Idea: ideas/open/127_typed_parser_record_identity_bridge.md
Activated From: ideas/open/127_typed_parser_record_identity_bridge.md
Supersedes Active Runbook: ideas/open/123_parser_legacy_string_lookup_removal_convergence.md parked on 2026-04-29

## Purpose

Introduce typed parser record identity through `TypeSpec` so record semantic
lookup does not depend on rendered tag spelling.

## Goal

Let struct and union `TypeSpec` values carry a real record identity alongside
`TypeSpec::tag`, then convert parser record lookup users to prefer that typed
identity before compatibility spelling lookup.

## Core Rule

Do not replace one rendered-string authority path with another. Preserve
`TypeSpec::tag` as spelling, diagnostics, emitted text, and compatibility
payload while moving semantic record lookup to typed identity.

## Read First

- `ideas/open/127_typed_parser_record_identity_bridge.md`
- `ideas/open/123_parser_legacy_string_lookup_removal_convergence.md`
- `todo.md` blocker notes for `DefinitionState::struct_tag_def_map`
- `TypeSpec` definition and all struct/union `TypeSpec::tag` producers.
- Parser layout, `offsetof`, incomplete object, template instantiation, member
  typedef, and direct template emission paths.
- Parser/frontend tests around record layout, `offsetof`, template records, and
  direct template emission compatibility mirrors.

## Current Targets

- `TypeSpec` struct/union record identity payload and propagation.
- `DefinitionState::struct_tag_def_map` users that currently recover semantic
  record identity from rendered spelling.
- Parser support paths for `sizeof`, `alignof`, `offsetof`, incomplete record
  checks, const-eval layout, template-instantiated records, and member typedef
  lookup.
- Compatibility tests that mutate or assert `struct_tag_def_map`.

## Non-Goals

- Do not perform broad HIR, LIR, or BIR legacy lookup migration.
- Do not remove `TypeSpec::tag`.
- Do not make `TextId` the semantic record identity.
- Do not redesign unrelated type identity.
- Do not downgrade supported tests or replace semantic checks with
  string-shaped shortcuts.

## Working Model

- `TypeSpec::tag` remains spelling and compatibility output.
- Typed record identity is separate from spelling and may be represented by an
  existing record `Node*`, a stable parser record id, or another existing
  parser-domain key chosen during inventory.
- Parser semantic record lookup must prefer typed identity when present.
- `DefinitionState::struct_tag_def_map` remains a compatibility/final-spelling
  mirror until all converted users have typed identity.

## Execution Rules

- Start with inventory before changing the `TypeSpec` contract.
- Keep each packet bounded to one propagation or consumer family.
- Prefer existing parser records and AST nodes over new broad abstractions.
- Add fallback compatibility only where needed to preserve current behavior.
- Update tests only when they prove semantic identity beats stale rendered
  spelling, not to weaken supported behavior.
- Pair code-changing packets with fresh build or focused parser test proof
  selected by the supervisor.

## Ordered Steps

### Step 1: Inventory Record Identity Through TypeSpec

Goal: map every record-identity producer, carrier, and consumer currently
crossing through `TypeSpec::tag`.

Primary targets:
- `TypeSpec` definition and struct/union construction helpers.
- `DefinitionState::struct_tag_def_map` reads and writes.
- Parser support, declarations, expressions, struct, declarator, base, and
  template code named in the Step 3 blocker.
- Tests that rely on `struct_tag_def_map` compatibility mirrors.

Concrete actions:
- Inventory each `TypeSpec::tag` producer for struct/union types and whether a
  record `Node*`, tag declaration, or stable parser record key is available.
- Inventory each consumer that treats `TypeSpec::tag` as semantic record
  identity.
- Choose the smallest viable typed identity representation and record why it is
  stable enough for parser use.
- Identify the first propagation packet and focused proof command in `todo.md`.

Completion check:
- `todo.md` names the selected typed identity representation, the first bounded
  propagation or consumer packet, retained `TypeSpec::tag` categories, and the
  focused proof command.

### Step 2: Add TypeSpec Record Identity Payload

Goal: let struct and union `TypeSpec` values carry typed record identity
without changing their final spelling behavior.

Primary targets:
- `TypeSpec` storage and constructors/helpers.
- Parser record definition and tag parsing paths.

Concrete actions:
- Add the chosen typed identity field or wrapper with behavior-preserving
  defaults.
- Populate it when a parser path already has the record definition or stable
  record key.
- Preserve `TypeSpec::tag` exactly for spelling, diagnostics, and emitted text.
- Add focused proof that existing tag-only compatibility still works.

Completion check:
- Struct/union `TypeSpec` values can carry typed record identity, legacy
  tag-only paths still compile and pass focused tests, and `TypeSpec::tag`
  output remains unchanged.

### Step 3: Convert Parser Record Consumers To Typed Identity

Goal: make parser record semantic users prefer typed identity before
`struct_tag_def_map` spelling lookup.

Primary targets:
- `support.cpp` layout and `offsetof` helpers.
- `declarations.cpp` incomplete object checks and constexpr evaluation bridge.
- `expressions.cpp` `NK_OFFSETOF` constant folding.
- `types/declarator.cpp` and `types/base.cpp` record lookup consumers outside
  template propagation.

Concrete actions:
- Convert one consumer family at a time to use typed identity when present.
- Keep `struct_tag_def_map` fallback explicit for tag-only compatibility.
- Add tests where a stale rendered tag would previously select the wrong record
  but typed identity now wins.
- Do not delete compatibility mirrors while unconverted consumers remain.

Completion check:
- Covered parser consumers resolve record semantics through typed identity
  before spelling fallback, with focused tests proving drifted rendered tags do
  not override the typed record.

### Step 4: Propagate Typed Record Identity Through Template Records

Goal: carry `TypeSpec::record_def` through template-instantiated records and
template-only lookup paths before demoting rendered tag mirrors.

Primary targets:
- `types/template.cpp` template struct instantiation and specialization reuse.
- Injected template instantiation fallback and template base `TypeSpec`
  construction.
- Deferred member typedef propagation and template static-member base
  recursion.
- Direct template emission record map population where it currently preserves
  rendered compatibility entries.

Concrete actions:
- Inventory the template producers that create or forward struct/union
  `TypeSpec` values without `record_def`.
- Populate or preserve `record_def` when the instantiated record `Node*` is
  known, without changing rendered instantiation keys or dedup behavior.
- Convert template-only lookup users to prefer typed record identity before
  rendered `struct_tag_def_map` fallback.
- Add focused tests where a stale rendered template record tag cannot override
  the instantiated record identity.
- Keep compatibility map writes that are required for final spelling, emitted
  text, and direct template emission.

Completion check:
- Template-instantiated record paths and template-only member/static lookup
  prefer typed record identity when available, rendered-template compatibility
  keys still work, and focused tests cover stale rendered tag resistance.

### Step 5: Demote struct_tag_def_map To Compatibility Mirror

Goal: make remaining rendered-tag map use visibly compatibility or final
spelling data.

Primary targets:
- `DefinitionState::struct_tag_def_map` writers and remaining readers.
- Direct template emission and testing compatibility hooks.
- Comments or helper names near retained spelling fallback paths.

Concrete actions:
- Rename, narrow, or document retained rendered-tag helpers if needed.
- Keep direct-emission compatibility tests but assert they are fallback mirrors,
  not primary semantic authority.
- Record any remaining unconverted cross-module boundary as a separate open
  idea if it cannot be handled in this bridge.

Completion check:
- Remaining `struct_tag_def_map` use is discoverably classified as
  compatibility/final spelling, and tests distinguish retained spelling from
  typed record authority.

### Step 6: Reprove And Return To Parser Cleanup

Goal: validate the bridge and decide whether the parent parser cleanup can
resume.

Primary targets:
- Focused parser/frontend tests changed during this bridge.
- Parser-adjacent frontend validation selected by the supervisor.
- Parent parser cleanup idea 123.

Concrete actions:
- Re-run focused proofs for typed record identity and compatibility fallback.
- Run broader frontend validation appropriate for `TypeSpec` contract changes.
- Review expectation updates to ensure no supported behavior was weakened.
- Record any remaining downstream bridge work before asking for lifecycle
  closure.
- Ask the supervisor to return to or regenerate the parent parser cleanup plan
  once `struct_tag_def_map` is no longer semantic authority.

Completion check:
- Focused and broader proof are recorded in `todo.md`, overfit risk is
  reviewed, retained `TypeSpec::tag` boundaries are documented, and the plan
  owner can decide whether to close this bridge and reactivate idea 123.
