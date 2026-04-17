# Parser Tentative State Journal And Checkpoint Rollback

Status: Draft
Last Updated: 2026-04-17

## Goal

Replace heavy `TentativeParseGuard` whole-state snapshot copying with a narrow
checkpoint + mutation-journal rollback model for the parser semantic state that
speculative parsing actually needs to restore.

The immediate objective is not to redesign all parser state management. The
immediate objective is to stop paying repeated full-copy cost for the current
heavy rollback surface when speculative parses touch typedef / var binding
state.

## Why This Idea Exists

The parser currently has two tentative modes:

- `TentativeParseGuardLite` for token-position and similarly cheap rollback
- `TentativeParseGuard` for speculative parses that may touch parser semantic
  state

The lite path already removed several unnecessary heavy probes, but the heavy
path still snapshots parser state by value. That is safe, but it scales poorly
when speculative parses repeatedly copy typedef/type/var tables.

Recent parser work also clarified an important point:

- the real cost is not some growing "type kind" enum
- the real cost is copying mutable parser semantic tables

So the next serious optimization direction is not "compress `TypeKind`". It is
"replace heavy parser semantic-state copying with rollback of recorded
mutations".

## Main Objective

Introduce a journaled rollback layer for the current heavy tentative surface:

1. speculative parse takes a checkpoint
2. all writes to the heavy rollback surface go through narrow mutation APIs
3. those APIs append undo information to a journal when a checkpoint is active
4. rollback restores only the touched entries since the checkpoint
5. successful commit discards the recorded delta

This should preserve current semantics while removing the need to copy the
entire heavy state on every guarded probe.

## Hard Scope Boundary

The first version must only cover the same state that the current heavy
snapshot already covers.

That means this idea is **not**:

- a general parser-state framework
- a repo-wide journaling abstraction
- a redesign of all parser persistent state
- a migration of every string carrier to ids

The first version **is** specifically about replacing the current heavy
`ParserSnapshot` surface with journaled rollback for exactly the same semantic
state.

## Current Heavy Rollback Surface

Primary files:

- [parser.hpp](/workspaces/c4c/src/frontend/parser/parser.hpp)
- [parser_support.cpp](/workspaces/c4c/src/frontend/parser/parser_support.cpp)

Today `ParserSnapshot` stores:

```cpp
struct ParserSnapshot {
  ParserLiteSnapshot lite;
  ParserSymbolTables symbol_tables;
  std::unordered_set<TextId> non_atom_typedefs;
  std::unordered_set<TextId> non_atom_user_typedefs;
  std::unordered_map<TextId, TypeSpec> non_atom_typedef_types;
  std::unordered_map<TextId, TypeSpec> non_atom_var_types;
};
```

And `ParserSymbolTables` currently covers:

- `typedefs`
- `user_typedefs`
- `typedef_types`
- `var_types`

That is the exact rollback surface the journaled design must initially match.

## Why "Just Store A Pointer" Is Not Enough

This heavy state is not a single append-only vector. It is a collection of:

- sets
- maps
- "insert if missing" operations
- overwrite/update operations
- occasional erase operations

So rollback cannot be reduced to a single tail pointer unless the parser state
model itself is first rewritten into append-only storage.

The practical first step is a mutation journal that records:

- which table changed
- which key changed
- whether the key existed before
- what the old value was when relevant

## Required Design Rule

To avoid route drift, the journaled design must be API-driven rather than
container-driven.

Do **not** expose generic helpers such as:

- `journal_set(table, key, value)`
- `journal_insert(table, key)`

Those are too easy to misuse and too easy to drift over time.

Instead, journal entry points should be semantic and narrow, for example:

- `record_typedef_presence(...)`
- `record_user_typedef_presence(...)`
- `record_typedef_type(...)`
- `record_var_type(...)`
- `record_non_atom_typedef_presence(...)`
- `record_non_atom_typedef_type(...)`
- `record_non_atom_var_type(...)`

The goal is for future parser code to mutate the heavy rollback surface only
through a small set of meaning-specific APIs.

## Drift Rejection Rule

The main failure mode here is API drift:

- new parser code writes directly to rollback-owned maps/sets
- journal coverage silently becomes incomplete
- rollback correctness degrades without obvious compile failures

The first implementation must reject that route by design:

1. keep the covered surface identical to current `ParserSnapshot`
2. funnel all writes for that surface through a few semantic helpers
3. document that those underlying tables are rollback-owned
4. avoid growing the covered surface until the first journaled slice is stable

## Proposed Data Model

### Checkpoint token

```cpp
struct TentativeStateCheckpoint {
  size_t journal_size = 0;
};
```

### Journal entry shape

```cpp
enum class TentativeMutationKind : uint8_t {
  TypedefPresence,
  UserTypedefPresence,
  TypedefType,
  VarType,
  NonAtomTypedefPresence,
  NonAtomUserTypedefPresence,
  NonAtomTypedefType,
  NonAtomVarType,
};

struct TentativeMutation {
  TentativeMutationKind kind{};
  uint32_t key = 0;
  bool old_exists = false;
  TypeSpec old_type{};
};
```

Notes:

- key type can be normalized to `uint32_t` because current rollback-owned ids
  are `SymbolId` or `TextId`
- only value-carrying mutations need `old_type`
- set-like mutations only need `old_exists`

### Parser-owned journal storage

```cpp
std::vector<TentativeMutation> tentative_state_journal_;
int tentative_checkpoint_depth_ = 0;
```

The exact names can differ. What matters is:

- nested checkpoints are supported
- journal writes are cheap
- rollback walks backward to the checkpoint size

## Required Helper Surface

The journal should not be called directly from scattered parse code.

Instead, the parser should expose a small internal mutation surface for the
heavy rollback-owned tables.

For example:

```cpp
void set_typedef_presence(SymbolId id, bool is_user_typedef);
void erase_typedef_presence(SymbolId id);
void set_typedef_type(SymbolId id, const TypeSpec& type);
void erase_typedef_type(SymbolId id);

void set_var_type(SymbolId id, const TypeSpec& type);
void erase_var_type(SymbolId id);

void set_non_atom_typedef_presence(TextId id, bool is_user_typedef);
void erase_non_atom_typedef_presence(TextId id);
void set_non_atom_typedef_type(TextId id, const TypeSpec& type);
void erase_non_atom_typedef_type(TextId id);
void set_non_atom_var_type(TextId id, const TypeSpec& type);
void erase_non_atom_var_type(TextId id);
```

Current high-level parser entry points such as:

- `register_typedef_name`
- `register_typedef_binding`
- `unregister_typedef_binding`
- `cache_typedef_type`
- `register_var_type_binding`

should be rewritten to use those helpers internally.

## Migration Strategy

### Phase 1. Journal scaffolding

- add checkpoint token + mutation journal
- keep old snapshot implementation in place
- add semantic mutation helpers for the current rollback surface

### Phase 2. Route writes through helpers

- update all current write sites for the covered surface
- ensure no direct writes remain for the covered tables

### Phase 3. Journaled rollback under heavy tentative mode

- `TentativeParseGuard` records a checkpoint instead of copying heavy state
- rollback undoes journal entries to the checkpoint
- commit drops journal tail ownership for that checkpoint

### Phase 4. Snapshot retirement

- remove heavy whole-state copy only after journaled rollback proves equivalent

## Validation Strategy

This idea needs stronger validation than a normal parser field migration.

### Functional proof

- existing parser regression subsets
- rollback-sensitive parse-only cases already covering tentative probes
- targeted tests that force nested speculative parses touching typedef state

### Equivalence proof

During migration, keep a temporary debug-only validation mode that compares:

- journaled rollback result
- old snapshot restore result

for the covered surface after representative speculative parse failures.

The first landing can keep the old snapshot logic available behind a debug gate
for this comparison before removing it.

## Constraints

- do not broaden the covered surface beyond current `ParserSnapshot`
- do not introduce a generic "any container" journal API
- do not rely on direct writes to rollback-owned tables after helper routing
- do not mix parser semantic journaling with unrelated diagnostics/debug state
- do not claim performance wins without confirming that the heavy rollback path
  is actually exercising the covered mutation surface

## Acceptance Criteria

- [ ] the first journaled implementation covers exactly the current heavy
      rollback surface and nothing more
- [ ] writes to that surface are routed through a narrow semantic helper API
- [ ] heavy tentative rollback can restore the covered surface without copying
      the full state by value
- [ ] nested checkpoints remain correct
- [ ] rollback-sensitive parser tests remain green

## Non-Goals

- replacing every parser `std::string`
- journalizing all parser state
- redesigning namespace/template registries in the same slice
- changing frontend/HIR type semantics
- inventing a generic reusable state-management framework for the whole repo
