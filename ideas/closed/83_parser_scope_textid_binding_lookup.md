# Parser Scope TextId Binding Lookup

Status: Complete
Last Updated: 2026-04-24

## Closure Note

The runbook reached step 7, and the broader parser/frontend proof passed
(`267/267`). The remaining work was lifecycle handling, so the idea is now
archived as complete.

## Goal

Add a parser lexical-scope binding system that uses `TextId`-native lookup as
the primary semantic path, while keeping namespace traversal as a separate
system and using `src/shared/local_name_table.hpp` as the main scope-local
lookup substrate.

## Why This Idea Exists

Idea 82 improves namespace ownership and qualified namespace traversal, but the
parser's visible-name lookup still mixes several responsibilities:

- lexical visibility is not modeled as its own scope-local binding system
- visible type/value/concept lookup still relies heavily on string-shaped
  bridge paths
- several parser binding tables still use `std::string` where semantic
  identity is already available as `TextId`
- namespace traversal and local visibility are not yet cleanly separated

That keeps `TextId` available but not fully on the hot path for parser lookup.
It also leaves many queries depending on composed spelling such as `"A::B"` or
other rendered-name bridges when the parser should instead query by structured
identity.

## Main Objective

Make parser-visible lookup `TextId`-first by introducing a dedicated lexical
scope lookup layer and migrating suitable binding tables away from
`std::string` keys toward `TextId`, `TextId` sequences, or interned path ids.

The intended direction is:

- keep namespace ownership and namespace-qualified traversal in the namespace
  tree introduced by idea 82
- add a separate parser scope-local binding model for lexical visibility
- use `src/shared/local_name_table.hpp` as the main scope-local lookup and
  binding strategy
- treat single-segment semantic names as `TextId` keys by default
- treat multi-segment names as `TextId` sequences or interned path ids rather
  than composed strings
- demote rendered strings to debug, diagnostics, and compatibility-only use

## Primary Scope

- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/parser_state.hpp`
- `src/frontend/parser/parser_core.cpp`
- parser helper files that currently route visible type/value/concept lookup
- parser binding tables that still use `std::string` where `TextId` is already
  the real semantic identity
- `src/shared/local_name_table.hpp`
- `src/shared/qualified_name_table.hpp`
- `src/shared/text_id_table.hpp`

## Candidate Directions

1. Add a dedicated parser scope state with explicit push/pop behavior for
   lexical visibility, separate from namespace push/pop state.
2. Use `LocalNameTable` as the main scope-local binding substrate for parser
   type/value/concept lookup instead of growing more ad-hoc string-keyed local
   maps.
3. Audit parser binding tables and replace `std::string` keys with `TextId`
   wherever the table really represents single-segment semantic identity.
4. For multi-segment names, use `TextId` sequence keys or interned path ids
   instead of composing new `"A::B"` strings for semantic lookup.
5. Introduce a unified parser lookup facade so new `TextId`-first scope lookup
   can run in parallel with legacy bridge-based lookup during migration.
6. Keep `TextTable` as spelling storage and interning infrastructure, but do
   not treat rendered strings or raw hash values as semantic identity.

## Binding Table Inventory And Migration Targets

Current parser lookup state is split across several table families. Idea 83
should treat them differently instead of attempting a blind repo-wide
`std::string` to `TextId` rewrite.

### A. Already `TextId`-Native Or Close Enough

These tables already use `TextId` or parser-symbol ids backed by `TextId`.
They are not the first key-migration problem. The likely follow-on work is to
change their storage shape or merge them into scope-local lookup payloads.

- `ParserNameTables::typedefs`
- `ParserNameTables::user_typedefs`
- `ParserNameTables::typedef_types`
- `ParserNameTables::var_types`
- `ParserBindingState::non_atom_typedefs`
- `ParserBindingState::non_atom_user_typedefs`
- `ParserBindingState::non_atom_typedef_types`
- `ParserBindingState::non_atom_var_types`
- `ParserBindingState::typedef_fn_ptr_info`

Representative code today:

```cpp
struct ParserNameTables {
  ParserSymbolTable* symbols = nullptr;
  std::unordered_set<ParserSymbolId> typedefs;
  std::unordered_set<ParserSymbolId> user_typedefs;
  std::unordered_map<ParserSymbolId, TypeSpec> typedef_types;
  std::unordered_map<ParserSymbolId, TypeSpec> var_types;
};

struct ParserBindingState {
  std::unordered_map<TextId, ParserFnPtrTypedefInfo> typedef_fn_ptr_info;
  std::unordered_set<TextId> non_atom_typedefs;
  std::unordered_set<TextId> non_atom_user_typedefs;
  std::unordered_map<TextId, TypeSpec> non_atom_typedef_types;
  std::unordered_map<TextId, TypeSpec> non_atom_var_types;
};
```

Migration direction:

- keep the `TextId`-native key path
- avoid re-stringifying these names during lookup
- later move the truly lexical parts into scope-local `LocalNameTable`
  ownership

### B. Strong Candidates For Direct `TextId` Replacement

These tables still use `std::string`, but many entries are really single-name
semantic identities and should be audited first for direct `TextId` migration.

- `ParserBindingState::concept_names`
- `ParserBindingState::enum_consts`
- `ParserBindingState::const_int_bindings`
- template-scope parameter name tracking inside `template_scope_stack`

Representative code today:

```cpp
struct ParserBindingState {
  std::set<std::string> concept_names;
  std::unordered_map<std::string, long long> enum_consts;
  std::unordered_map<std::string, long long> const_int_bindings;
};

struct ParserTemplateScopeParam {
  const char* name = nullptr;
  bool is_nttp = false;
};
```

Expected replacement shape:

```cpp
struct ParserTemplateScopeParam {
  TextId name_text_id = kInvalidText;
  const char* name = nullptr;  // bridge / debug only during migration
  bool is_nttp = false;
};

struct ParserBindingState {
  std::unordered_set<TextId> concept_names;
  std::unordered_map<TextId, long long> enum_consts;
  std::unordered_map<TextId, long long> const_int_bindings;
};
```

Migration rule:

- if the semantic identity is one unqualified parser name, prefer `TextId`
- keep string spelling only when diagnostics or compatibility code still need
  it

### C. Candidates For Scope-Local `LocalNameTable`

These are the bindings that should stop living only in parser-wide flat tables
and should instead move toward lexical-scope ownership.

- local typedef visibility
- local variable visibility
- local concept visibility
- local function-name visibility where the key is still a single visible name
- template parameter visibility as a lexical scope kind

Representative target shape:

```cpp
struct ParserScopeLookupState {
  LocalNameTable<TextId, TypeSpec> visible_types;
  LocalNameTable<TextId, TypeSpec> visible_values;
  LocalNameTable<TextId, ConceptBinding> visible_concepts;
};
```

Possible parser-facing helpers:

```cpp
void push_scope();
void pop_scope();

void bind_local_typedef(TextId name_text_id, const TypeSpec& type);
void bind_local_value(TextId name_text_id, const TypeSpec& type);

const TypeSpec* find_local_visible_typedef(TextId name_text_id) const;
const TypeSpec* find_local_visible_value(TextId name_text_id) const;
```

Migration rule:

- if the binding is lexical and unqualified, prefer `LocalNameTable`
- do not keep adding new parser-wide flat tables for names that follow lexical
  push/pop lifetime

### D. Not Suitable For Plain `TextId`; Require Sequence Or Path Keys

These tables or call paths represent qualified or owner-scoped identities.
They should not be migrated by naively replacing `std::string` with one
`TextId`.

- `ParserBindingState::known_fn_names`
- `ParserBindingState::struct_typedefs`
- any owner-qualified or namespace-qualified lookup that currently depends on
  rendered names like `"A::B::f"`

Representative code today:

```cpp
struct ParserBindingState {
  std::set<std::string> known_fn_names;
  std::unordered_map<std::string, TypeSpec> struct_typedefs;
};
```

Expected replacement direction:

```cpp
using LocalNameKey = std::vector<TextId>;

struct ParserBindingState {
  LocalNameTable<LocalNameKey, FunctionBinding> scoped_functions;
  LocalNameTable<LocalNameKey, TypeSpec> scoped_struct_typedefs;
};
```

or, when reuse of shared path interning is better:

```cpp
struct ScopedFunctionKey {
  NamePathId owner_path_id = kInvalidNamePath;
  TextId base_text_id = kInvalidText;
};
```

Migration rule:

- if semantic identity is multi-segment, use `TextId` sequence or path-id key
- do not rebuild `"A::B"` strings as the primary semantic lookup key
- qualified or owner-scoped tables such as `ParserBindingState::known_fn_names`
  and `ParserBindingState::struct_typedefs` are intentionally deferred to
  idea 84 rather than extended further in this idea

### E. Namespace-Side Tables That Should Stay Separate From Lexical Scope

These tables participate in lookup, but they belong to the namespace system,
not the lexical scope-local system.

- `ParserNamespaceState::named_namespace_children`
- `ParserNamespaceState::anonymous_namespace_children`
- `ParserNamespaceState::using_namespace_contexts`
- `ParserNamespaceState::using_value_aliases`

Representative code today:

```cpp
struct ParserNamespaceState {
  std::unordered_map<int, std::unordered_map<TextId, int>>
      named_namespace_children;
  std::unordered_map<int, std::vector<int>> anonymous_namespace_children;
  std::unordered_map<int, std::unordered_map<TextId, std::string>>
      using_value_aliases;
  std::unordered_map<int, std::vector<int>> using_namespace_contexts;
};
```

Migration direction:

- keep namespace traversal separate from lexical scope lookup
- `using_namespace_contexts` already carries structural links and should stay
  namespace-owned
- `using_value_aliases` should keep its `TextId` key path but should eventually
  return structured binding targets instead of rendered strings

Possible follow-on replacement:

```cpp
struct UsingValueAliasTarget {
  int target_namespace_context_id = -1;
  TextId target_text_id = kInvalidText;
};

std::unordered_map<int, std::unordered_map<TextId, UsingValueAliasTarget>>
    using_value_aliases;
```

## Constraints

- preserve parser behavior while changing lookup structure
- keep namespace traversal and lexical scope lookup as separate systems with a
  clear boundary
- prefer `TextId` keys for single names and `TextId`-native sequence/path keys
  for multi-segment names
- use `src/shared/local_name_table.hpp` as the preferred strategy for new
  scope-local parser lookup
- treat string rendering as a bridge layer, not the primary semantic lookup
  path
- audit before migrating: not every existing `std::string` table should be
  replaced in the first packet
- do not widen this slice into sema, HIR, or backend identity redesign
- do not use raw hash values alone as semantic keys

## Validation

At minimum:

- `cmake --build build -j --target c4c_frontend c4cll`
- focused parser/frontend tests that exercise:
  - local scope shadowing
  - block and function lexical visibility
  - namespace-qualified type/value lookup continuing to work through the
    separate namespace system
  - `using namespace` and `using` alias behavior through the unified lookup
    facade
  - template parameter and record-owner cases that depend on visible lookup
- broader `ctest` only if the slice crosses beyond parser lookup plumbing

## Non-Goals

- no full C++ name-lookup conformance pass in one idea
- no one-shot removal of every legacy string bridge
- no forced repo-wide `std::string` to `TextId` migration
- no collapse of namespace traversal and lexical visibility into one data
  structure
- no reliance on composed strings or raw hash values as the long-term semantic
  identity path
- no sema, HIR, or backend naming redesign

## Suggested Execution Decomposition

1. Inventory parser binding tables and classify which ones should stay string
   keyed, which should become `TextId` keyed, and which should move to
   `LocalNameTable` scope-local storage.
   Expected output:
   - a short table-by-table decision list in `todo.md` or review notes
   - at least one concrete migration packet chosen from sections B or C above
2. Add parser lexical scope state plus `LocalNameTable`-based local bindings
   for the simplest single-segment type/value/concept cases.
   Example target:

   ```cpp
   struct ParserScopeState {
     LocalNameTable<TextId, TypeSpec> visible_types;
     LocalNameTable<TextId, TypeSpec> visible_values;
   };
   ```

3. Introduce a unified `TextId`-first lookup facade that queries new
   scope-local bindings first and then falls back to the legacy bridge path.
   Example shape:

   ```cpp
   const TypeSpec* find_visible_type_binding(TextId name_text_id) const {
     if (const auto found =
             scope_state_.visible_types.find_nearest_visible_text_ids(
                 &name_text_id, 1)) {
       return found.value;
     }
     return find_legacy_visible_type_binding(name_text_id);
   }
   ```

4. Move unqualified visible lookup onto the new scope-local path while keeping
   namespace-qualified lookup on the separate namespace tree.
5. Replace remaining suitable single-name `std::string` binding tables with
   `TextId`-based storage and isolate any multi-segment semantic queries to
   `TextId` sequence or path-id keys.
6. Reduce legacy composed-string lookup to compatibility, debug, and
   diagnostic helpers once the new path proves equivalent on the focused test
   set.
