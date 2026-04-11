# Frontend Source-Atom Symbol Table And Id-Keyed Semantic Maps

Status: Open
Last Updated: 2026-04-11

## Goal

Introduce a parser/sema-owned source-atom symbol table so identifier-like names
can gain stable `SymbolId` values at semantic use sites, then migrate
parser/HIR semantic tables from string-keyed lookups toward id-keyed maps and
sets where that keeps the model simpler and cheaper.

## Why This Idea Exists

The frontend currently uses many `std::string`-keyed tables for parser state,
name classification, and lowered-name lookup. That works, but it couples two
different concerns:

- raw source spelling identity
- semantic facts about that spelling in one stage or scope

This shows up most clearly in parser state such as typedef-name membership and
typedef/type lookup, where the system repeatedly compares and hashes source
strings even though many call sites only need stable identifier identity.

There is also a useful boundary here:

- source-level identifier atoms such as `std`, `vector`, or `foo`
- composed names such as `std::vector`
- synthesized/generated names such as anonymous-namespace internals or mangled
  template instantiations

The first category is a strong fit for a parser/sema-owned `SymbolId`. The
second should remain structured path/context data. The third should stay out of
scope for the initial slice so the design stays simple.

## Main Objective

Create a parser/sema-owned source-atom symbol table and use it to support
id-keyed semantic maps without forcing a full repo-wide string replacement or
pushing semantic identity into `Token`.

The intended direction is:

- assign stable `SymbolId` values to source identifier atoms when parser/sema
  needs semantic identity
- keep qualified names as structured segment lists, not one interned full name
- keep synthesized names and mangled names out of the first symbol-table slice
- let parser/HIR semantic state depend on ids rather than raw strings where the
  representation is truly identifier-keyed

## Current Code Shape

Today the lexer emits tokens with this effective shape:

```cpp
struct Token {
  TokenKind kind;
  std::string lexeme;
  std::string file;
  int line;
  int column;
};
```

The token layer is intentionally lexical. It tells the parser that something is
an identifier and preserves its spelling/source location, but it does not
itself carry semantic symbol identity today.

Today the parser also stores several name facts directly as strings:

```cpp
std::set<std::string> typedefs_;
std::set<std::string> user_typedefs_;
std::unordered_map<std::string, TypeSpec> typedef_types_;
std::unordered_map<std::string, TypeSpec> var_types_;
```

This matters for two reasons:

- typedef/type membership checks are frequent on parser hot paths
- heavy tentative snapshot/restore copies those tables directly today

The current representation is therefore simple but expensive to evolve: name
identity, semantic lookup, and speculative-state storage are all fused together.

## Primary Scope

- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/parser_core.cpp`
- parser implementation files that maintain name-keyed semantic state
- parser-local qualified-name helpers and semantic lookup wrappers
- selected frontend/HIR tables that naturally key on source names rather than
  full generated strings

## Candidate Directions

1. Add a compilation-scoped `SymbolInterner` or similar frontend-owned service
   that maps source identifier atoms to `SymbolId` for parser/sema use.
2. Keep tokens lexical and intern identifier spellings at parser-owned semantic
   entry points instead of attaching `symbol_id` to `Token`.
3. Convert parser-side semantic tables such as typedef-name membership and
   typedef/type lookup from `std::string` keys to `SymbolId` keys behind
   parser-local wrappers.
4. Represent qualified names as structured `SymbolId` segments rather than
   interning full strings like `std::vector` as one symbol.
5. Delay mangled/generated-name unification until a later idea so the first
   migration stays source-atom-only.

## Core Design Rules

- `SymbolId` identifies source-level identifier atoms only.
- `Token` remains lexical; semantic identity is assigned by parser/sema.
- `std::vector` is not one symbol; it is a qualified path of multiple symbols.
- Mangled names, anonymous internal names, and other synthesized strings remain
  ordinary strings in the first slice.
- Semantic tables are stage-local and may be rebuilt per phase; only source
  atom identity is shared across parser/sema surfaces that opt into it.
- Avoid a global process-wide singleton; prefer a compilation-scoped shared
  symbol table owned by parser/frontend session context.

## Proposed Data Model

Keep `Token` lexical:

```cpp
struct Token {
  TokenKind kind;
  std::string lexeme;
  std::string file;
  int line;
  int column;
};
```

Add semantic identity on the parser/sema side instead:

```cpp
using SymbolId = uint32_t;
constexpr SymbolId kInvalidSymbol = 0;
```

The symbol table itself should be compilation-scoped and frontend-owned:

```cpp
class SymbolInterner {
 public:
  SymbolId intern(std::string_view text);
  std::string_view lookup(SymbolId id) const;

 private:
  std::vector<std::string> text_by_id_;
  std::unordered_map<std::string, SymbolId> id_by_text_;
};
```

The first implementation can keep STL internally. The point of the slice is to
stabilize the identity model, not to immediately micro-optimize the interner.

Parser-facing helpers may wrap the interner so parser code can ask semantic
questions in terms of names rather than raw container operations:

```cpp
struct ParserNameTables {
  SymbolId intern_identifier(std::string_view text);

  std::unordered_set<SymbolId> typedefs;
  std::unordered_set<SymbolId> user_typedefs;
  std::unordered_map<SymbolId, TypeSpec> typedef_types;
  std::unordered_map<SymbolId, TypeSpec> var_types;

  bool is_typedef(SymbolId id) const;
  bool has_typedef_type(SymbolId id) const;
  const TypeSpec* lookup_typedef_type(SymbolId id) const;
};
```

## Parser Integration

The parser is the right insertion point for source-atom symbols because:

- it already distinguishes identifier tokens from non-identifier tokens
- it owns typedef/value/type semantic tables today
- it controls speculative snapshot/restore of those semantic tables
- it can keep raw token spelling for diagnostics while using ids for semantic
  identity

The intended flow is:

1. keep `Token` unchanged
2. when parser code sees an identifier token on a semantic path, intern
   `cur().lexeme` through a parser-owned interner/helper
3. use the resulting `SymbolId` for typedef membership, typedef type lookup,
   and similar identifier-keyed tables
4. continue to use raw strings for literals, pragmas, diagnostics, and
   composed/synthesized names that are out of scope for the first slice

## Semantic Table Shape

The intended semantic table migration is not "everything becomes one universal
table". The universal table only provides source-atom identity. Semantic facts
remain stage-local.

For parser typedef tracking, the current shape:

```cpp
std::set<std::string> typedefs_;
std::set<std::string> user_typedefs_;
std::unordered_map<std::string, TypeSpec> typedef_types_;
```

should move toward:

```cpp
using TypedefNameSet = std::unordered_set<SymbolId>;
using TypedefTypeMap = std::unordered_map<SymbolId, TypeSpec>;

TypedefNameSet typedefs_;
TypedefNameSet user_typedefs_;
TypedefTypeMap typedef_types_;
```

The first slice does not need a custom container. It only needs id-keyed
membership and lookup.

This wrapper gives the parser a stable interface and keeps later representation
changes local.

## Qualified Name Representation

The universal symbol table should only track source identifier atoms.

That means:

- `std` is one symbol
- `vector` is one symbol
- `std::vector` is not one symbol

Qualified names should therefore evolve toward a structured representation:

```cpp
struct QualifiedNameRef {
  bool is_global_qualified = false;
  std::vector<SymbolId> qualifier_segments;
  SymbolId base_name = kInvalidSymbol;
};
```

The parser may temporarily keep legacy string fields in some helper structs
during migration, but the target model is segment ids plus context-sensitive
lookup rather than repeatedly building and hashing composed strings.

## Representation Of Existing Typedef State

The current parser uses `typedefs_` for simple membership tests:

- `is_typedef_name(cur().lexeme)`
- tentative parsing disambiguation
- typedef registration and cleanup in declarations/struct parsing

It uses `typedef_types_` for resolved type lookup:

- `typedef_types_.find(name)`
- `typedef_types_.count(resolve_visible_type_name(name))`
- typedef seeding for builtin and platform aliases

The first migration should preserve that split rather than merging them:

- `typedefs_`: membership only
- `typedef_types_`: semantic type lookup
- `user_typedefs_`: ownership/provenance tracking for redeclaration rules

The change is only to replace string keys with source-atom ids where the key is
actually an atom identifier.

## Names That Stay String-Keyed Initially

Not every existing string-keyed table should move in the first slice.

The following are intentionally allowed to remain string-based at first:

- canonical namespace names such as `std::vector`
- imported/scoped aliases currently materialized as full strings
- struct-member typedef keys such as `StructTag::TypeName`
- mangled names
- anonymous synthesized names such as `__anon_ns_N`

Those are either composed names or generated names, not plain source-atom
identifiers. They deserve a later dedicated design instead of being forced into
the atom-symbol slice.

## Tentative Parsing Implications

Because heavy tentative parsing currently snapshots parser typedef/type tables by
copying them, the first id-keyed migration should explicitly preserve snapshot
behavior:

- `ParserSnapshot` continues to store typedef and var-type tables
- only the key type changes from `std::string` to `SymbolId`
- no journaled rollback model is required in this first idea

That keeps the behavioral risk small while still shrinking the payload carried
by speculative parser state.

## Migration Strategy

This migration should be parser-driven rather than token-driven.

The intended sequence is:

1. add `SymbolId` and a parser-owned interner/name-table helper
2. keep `Token` unchanged and continue reading identifier spelling from
   `Token::lexeme`
3. migrate parser hot paths and helper utilities toward id-based access
4. preserve snapshot/restore semantics while changing parser table key types
5. consider later token/text/file interning only if a separate idea justifies
   moving that concern lower in the stack

## Candidate First Migrations

1. Parser typedef-name membership:
   add parser-owned symbol interning, then move `typedefs_` and nearby checks
   from string-keyed membership to `SymbolId` membership.
2. Parser typedef/type lookup:
   move `typedef_types_` and closely related tables to `SymbolId` keys for
   cases where the lookup target is a plain source atom.
3. Parser variable/type disambiguation:
   migrate the small set of parser-side tables used on speculative parse paths
   where string hashing/comparison currently sits on hot lookup edges.
4. Qualified-name parsing surface:
   update parser-local qualified-name structs so segment atoms can carry
   `SymbolId` values while preserving current behavior and diagnostics.
5. Parser-name-table wrapper:
   introduce a parser-local wrapper around typedef/type/value tables before
   considering any denser storage backend.

## Constraints

- do not force a repo-wide migration from `std::string` to `SymbolId`
- do not redesign `Token` as part of this slice
- do not treat composed names as single symbols in the universal table
- do not mix mangling redesign into the first source-atom symbol-table slice
- preserve parser correctness and current diagnostics first
- keep the first landed slice small enough to validate without broad frontend
  churn

## Expected Validation

- existing parser regression suite
- rollback-sensitive parser tests
- targeted frontend/HIR tests for the migrated name tables
- parse-only or frontend-heavy timing checks when a migrated table sits on a
  known hot path
- parser/unit coverage for identifier spellings interning to stable `SymbolId`
  values without changing keyword/literal token behavior

## Non-Goals

- no repo-wide replacement of all `std::string`
- no global singleton symbol table shared across unrelated compilations
- no attempt to intern full qualified names such as `A::B::C` as one symbol
- no token `symbol_id` / `text_id` / `file_id` redesign in this slice
- no mangled-name or generated-name redesign in the first slice
- no backend symbol/storage redesign
