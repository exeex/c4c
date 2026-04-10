# Frontend Source-Atom Symbol Table And Id-Keyed Semantic Maps

Status: Open
Last Updated: 2026-04-10

## Goal

Introduce a frontend-owned source-atom symbol table so identifier-like source
tokens gain stable `SymbolId` values early, then migrate parser/HIR semantic
tables from string-keyed lookups toward id-keyed maps and sets where that keeps
the model simpler and cheaper.

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

The first category is a strong fit for a universal frontend `SymbolId`. The
second should remain structured path/context data. The third should stay out of
scope for the initial slice so the design stays simple.

## Main Objective

Create a compilation-scoped source-atom symbol table and use it to support
id-keyed semantic maps without forcing a full repo-wide string replacement.

The intended direction is:

- assign stable `SymbolId` values to source identifier atoms
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

The central construction point is `Lexer::make_token(...)`, which means the
lexer is a practical place to attach source-atom symbol ids without forcing the
parser to perform ad hoc interning at every identifier consumer.

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

- `src/frontend/token.*`
- `src/frontend/lexer.*`
- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/parser_core.cpp`
- parser implementation files that maintain name-keyed semantic state
- selected frontend/HIR tables that naturally key on source names rather than
  full generated strings

## Candidate Directions

1. Add a compilation-scoped `SymbolInterner` or similar frontend-owned service
   that maps source identifier atoms to `SymbolId`.
2. Extend tokens so identifier-like tokens can carry an optional `symbol_id`
   while remaining backward-compatible with existing `lexeme` consumers.
3. Convert parser-side semantic tables such as typedef-name membership and
   typedef/type lookup from `std::string` keys to `SymbolId` keys behind
   parser-local wrappers.
4. Represent qualified names as structured `SymbolId` segments rather than
   interning full strings like `std::vector` as one symbol.
5. Delay mangled/generated-name unification until a later idea so the first
   migration stays source-atom-only.

## Core Design Rules

- `SymbolId` identifies source-level identifier atoms only.
- `std::vector` is not one symbol; it is a qualified path of multiple symbols.
- Mangled names, anonymous internal names, and other synthesized strings remain
  ordinary strings in the first slice.
- Semantic tables are stage-local and may be rebuilt per phase; only source
  atom identity is shared across the frontend pipeline.
- Avoid a global process-wide singleton; prefer a compilation-scoped shared
  symbol table owned by the frontend/session context.

## Proposed Data Model

The `lexeme + file + symbol_id` token shape is only a migration bridge, not the
target end-state.

The intended end-state token model is:

```cpp
using SymbolId = uint32_t;
using TextId = uint32_t;
using FileId = uint32_t;

constexpr SymbolId kInvalidSymbol = 0;
constexpr TextId kInvalidText = 0;
constexpr FileId kInvalidFile = 0;

struct Token {
  TokenKind kind;
  SymbolId symbol_id = kInvalidSymbol;
  TextId text_id = kInvalidText;
  FileId file_id = kInvalidFile;
  int line = 0;
  int column = 0;
};
```

That split exists because the frontend currently overloads one `lexeme` field
with multiple meanings:

- identifier atom identity
- raw literal spelling
- pragma payload text
- synthetic parser helper text

Those concerns should not share one id space.

The first landed change should add the new fields immediately while keeping the
legacy string fields only as a compatibility bridge:

```cpp
struct Token {
  TokenKind kind;
  SymbolId symbol_id = kInvalidSymbol;
  TextId text_id = kInvalidText;
  FileId file_id = kInvalidFile;
  int line = 0;
  int column = 0;

  [[deprecated("use symbol_id/text_id instead")]]
  std::string lexeme;

  [[deprecated("use file_id instead")]]
  std::string file;
};
```

This deprecation is intentional. In this migration, warnings are a convergence
tool: the purpose is to light up remaining legacy call sites quickly instead of
letting them remain invisible.

The field usage model should be:

- identifier tokens: `symbol_id` is populated
- literal/raw-text tokens: `text_id` is populated
- all tokens: `file_id` is populated
- legacy `lexeme` and `file` exist only to support staged migration

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

Raw token spellings and file identities should use separate tables:

```cpp
class TextTable {
 public:
  TextId intern(std::string_view text);
  std::string_view lookup(TextId id) const;
};

class FileTable {
 public:
  FileId intern(std::string_view path);
  std::string_view lookup(FileId id) const;
};
```

Using dedicated tables avoids conflating source-atom identifier identity with
non-identifier raw token text.

## Lexer Integration

The lexer is the best insertion point for source-atom symbols because:

- `Lexer::make_token(...)` is the central token creation path
- identifier lexemes are already isolated there
- the parser is currently broad and does not benefit from more repeated
  per-call-site string-to-id glue

The intended flow is:

1. add `SymbolInterner*` or equivalent frontend context ownership to the lexer,
   together with `TextTable*` and `FileTable*` or one shared frontend tables
   owner
2. in `scan_identifier_or_keyword()`, determine whether the spelling remains an
   `Identifier` or becomes a keyword
3. only if the token remains `Identifier`, assign `symbol_id =
   symbols_->intern(lexeme)`
4. for tokens that need raw spelling, assign `text_id = texts_->intern(lexeme)`
5. assign `file_id = files_->intern(current_file_)` for all emitted tokens
6. keep filling legacy `lexeme` and `file` during the migration window

That keeps the migration mechanically simple:

- adding the new fields is low-risk
- deprecation warnings expose old accesses immediately
- parser and test code can migrate incrementally without blocking token
  production changes

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

The parser-facing API should also stop exposing raw-container semantics
directly. A small wrapper is preferred:

```cpp
struct ParserNameTables {
  std::unordered_set<SymbolId> typedefs;
  std::unordered_set<SymbolId> user_typedefs;
  std::unordered_map<SymbolId, TypeSpec> typedef_types;
  std::unordered_map<SymbolId, TypeSpec> var_types;

  bool is_typedef(SymbolId id) const;
  bool has_typedef_type(SymbolId id) const;
  const TypeSpec* lookup_typedef_type(SymbolId id) const;
};
```

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

The parser may temporarily keep legacy string fields during migration, but the
target model is segment ids plus context-sensitive lookup rather than repeatedly
building and hashing composed strings.

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

This migration should be warning-driven rather than silent.

The intended sequence is:

1. add `symbol_id`, `text_id`, and `file_id` to `Token`
2. keep `lexeme` and `file` only as temporary legacy bridge fields
3. immediately mark those legacy fields `[[deprecated]]`
4. update the lexer so the new id fields are filled centrally
5. migrate parser hot paths and helper utilities toward id-based access
6. remove legacy string fields only after warning-driven cleanup reaches the
   desired floor

If a local development workflow wants less warning noise during the transition,
that can be handled by local flags. The design itself should still advertise the
legacy fields as deprecated so the codebase converges quickly.

## Candidate First Migrations

1. Parser typedef-name membership:
   add the new token ids, populate them in the lexer, then move `typedefs_` and
   nearby checks from string-keyed membership to `SymbolId` membership.
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
- lexer/parser unit coverage for identifier tokens carrying stable `symbol_id`
  values without changing keyword/literal token behavior

## Non-Goals

- no repo-wide replacement of all `std::string`
- no global singleton symbol table shared across unrelated compilations
- no attempt to intern full qualified names such as `A::B::C` as one symbol
- no mangled-name or generated-name redesign in the first slice
- no backend symbol/storage redesign
