# Parser Qualified Name Structured Lookup

Status: Closed
Last Updated: 2026-04-23

## Goal

Move parser qualified-name and namespace-side semantic lookup off
rendered-string bridges onto structured identity built from `TextId`
segments and interned path ids, while keeping lexical scope lookup and
namespace traversal as separate systems.

## Why This Idea Exists

Idea 82 moved namespace ownership and qualified namespace traversal onto a
`TextId`-driven context tree. Idea 83 is finishing the parser lexical-scope
side by making unqualified local visibility `TextId`-first through
`LocalNameTable`.

That still leaves a separate family of parser lookups that use structured
parsing on the front end but fall back to rendered spellings such as
`"A::B"` or `"A::B::f"` as the semantic lookup key:

- owner-qualified function-name bookkeeping still uses flat string keys
- some struct / typedef identity paths still use canonical rendered names
- `using` import and alias paths still convert qualified names back into
  strings before probing parser bindings
- compatibility helpers such as `qualified_name_text(...)` and
  `compatibility_namespace_name_in_context(...)` still sit on semantic hot
  paths instead of staying bridge-only

That means parser lookup is now split in a useful way, but not yet cleanly:

- lexical scope is moving toward `TextId`-native lookup
- namespace traversal is already segment-based
- qualified semantic identity between those two layers still often flattens
  back into strings

As long as that middle layer still depends on rendered names, parser lookup
keeps paying a compatibility tax and the boundary between semantic identity
and spelling remains blurry.

## Main Objective

Make parser qualified-name semantic lookup use structured identity as the
primary path.

The intended direction is:

- keep lexical-scope binding on the separate local-scope path from idea 83
- keep namespace ownership and traversal on the namespace context tree from
  idea 82
- represent qualified semantic identity with `TextId` sequences,
  `NamePathId`, or explicit structured keys instead of canonical strings
- demote rendered-name helpers to diagnostics, bridge code, and
  compatibility-only paths

This idea is about the lookup identity used after parsing a qualified name,
not about changing how the parser tokenizes or spells names for user-facing
output.

## Primary Scope

- `src/frontend/parser/parser.hpp`
- `src/frontend/parser/parser_state.hpp`
- `src/frontend/parser/parser_declarations.cpp`
- `src/frontend/parser/parser_expressions.cpp`
- `src/frontend/parser/parser_types_base.cpp`
- `src/frontend/parser/parser_types_struct.cpp`
- `src/shared/qualified_name_table.hpp`
- parser helper code that currently routes semantic lookup through rendered
  qualified-name strings

## Specific Problems To Remove

### A. Flat String Keys For Qualified Semantic Identity

These parser binding paths represent qualified or owner-scoped identities but
still store them as rendered text:

- `ParserBindingState::known_fn_names`
- `ParserBindingState::struct_typedefs`
- other parser tables where the semantic key is effectively
  `owner path + base name` but the implementation stores `"A::B::name"`

Those tables should move to structured keys such as:

- `NamePathId + TextId`
- `std::vector<TextId>`
- another explicit qualified-key type that keeps owner path and base segment
  separate

Migration rule:

- if the semantic identity is qualified or owner-scoped, do not model it as
  one composed `std::string`

### B. Semantic Lookup Through Compatibility Rendering

These helper families are still useful, but they should not remain the main
semantic path:

- `qualified_name_text(...)`
- `compatibility_namespace_name_in_context(...)`
- other helper code that rebuilds canonical `A::B::C` text before querying
  parser-owned state

Expected direction:

- parse qualified names into structured identity
- use structured identity for parser binding lookup and registration
- render text only when diagnostics, bridge code, or legacy surfaces still
  require spelling

### C. `using` / Import / Alias Paths That Flatten Back To Strings

Several `using`-related and alias-registration flows already know the target
base segment and namespace context, but they still synthesize a string key
before checking visible parser state.

Expected direction:

- keep namespace visibility and import ownership separate from lexical lookup
- store imported semantic identity in structured form
- avoid reconstituting canonical names just to re-enter parser binding tables

### D. Record / Type Identity Paths That Still Depend On Canonical Names

Record tags, typedef-style lookups, and related parser-visible type identity
should stop depending on canonical rendered names when structured owner-path
identity is already available.

This does not require removing every compatibility string in one pass. It
does require that semantic lookup stop depending on those strings as the
authoritative key.

## Expected Design Direction

Representative replacement shapes include:

```cpp
struct QualifiedLookupKey {
  NamePathId owner_path_id = kInvalidNamePath;
  TextId base_text_id = kInvalidText;
  bool is_global_qualified = false;
};
```

or:

```cpp
using QualifiedLookupSegments = std::vector<TextId>;
```

The exact key form can vary by table, but the governing rule is stable:

- keep qualified semantic identity structured
- do not rebuild `"A::B"` strings as the primary lookup substrate

## Validation

At minimum:

- `cmake --build --preset default`
- `ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'`

Focused proof should include parser/frontend cases that exercise:

- namespace-qualified type and value references
- `using` declarations and namespace imports
- owner-qualified constructors / operators / member declarations
- nested namespace and nested record references

Escalate to broader checks if the slice expands beyond parser lookup plumbing
or starts touching shared qualified-name helpers in a way that affects more
than the parser frontend.

## Non-Goals

- no re-merging lexical scope lookup with namespace traversal
- no giant repo-wide string-to-`TextId` migration
- no requirement to delete every compatibility rendering helper in one pass
- no backend or HIR identity redesign outside what parser lookup directly
  needs
- no testcase-shaped shortcuts that only special-case a few known qualified
  names

## Suggested Execution Decomposition

1. Introduce or normalize structured qualified-name key types for parser-owned
   semantic lookup tables.
2. Migrate one narrow qualified binding family at a time away from flat
   rendered-string keys.
3. Retarget `using` / alias / import lookup paths to structured identity
   without collapsing namespace visibility into lexical lookup.
4. Reduce `qualified_name_text(...)` and
   `compatibility_namespace_name_in_context(...)` to bridge-only support on
   the touched paths.
5. Re-run focused parser proof and escalate only if the blast radius extends
   beyond the parser qualified-lookup surface.

## Follow-On Relationship

This idea intentionally follows:

- `ideas/closed/82_parser_namespace_textid_context_tree.md`
- `ideas/open/83_parser_scope_textid_binding_lookup.md`

The boundary is deliberate:

- idea 82: namespace ownership and traversal
- idea 83: lexical scope-local unqualified binding
- idea 84: qualified semantic identity that still bridges through rendered
  names between those systems

## Completion Notes

Completed on 2026-04-23.

This runbook finished the parser qualified-lookup slice by:

- migrating parser-owned qualified lookup paths to structured identity built
  from owner-path context and `TextId`-based segments
- retargeting alias/import and declaration bridge paths away from rendered
  qualified-name strings as the primary semantic key
- demoting remaining compatibility rendering on the touched typedef and
  concept paths to bridge-only fallback support

The remaining compatibility string storage is intentionally limited to
untouched bridge and diagnostics surfaces rather than the primary qualified
semantic lookup path.
