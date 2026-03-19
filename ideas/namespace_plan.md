# Namespace Support Plan

## Goal

Implement C++ namespace support in a way that is structurally correct, not
just parser-tolerant.

This plan is explicitly **not**:

- "make `namespace` syntax stop crashing"
- "flatten names into `a::b::c` strings and call it done"

This plan **is**:

- build a namespace model that can survive real C++ code
- support correct lookup across nested scopes
- provide a foundation for `<vector>` and other standard headers
- avoid painting the frontend into a corner

## Problem Statement

The current parser groundwork proved that we can recognize:

- `namespace`
- `using X = Y;`
- `using ns::name;`
- `using namespace ns;`

But the current implementation is still mostly string-based:

- active namespace is tracked as a flattened string
- declarations are rewritten to names like `ns::foo`
- some lookup is done by mapping visible names back to qualified strings

That is useful as a short-term spike, but it is not a durable model.

## Core Conclusion

The real fix has to move toward the same broad shape used by Clang/LLVM:

- namespace is a scope/context object
- declarations know their enclosing context
- qualified names are structured, not raw strings
- lexical nesting and semantic ownership are separable concepts

We do **not** need to clone Clang exactly.
But we do need the same class of solution.

## Why String Flattening Is Not Enough

A flattened name like `a::b::c` loses important information:

1. It does not distinguish:
   - global-qualified `::a::b`
   - relative `a::b`

2. It does not model anonymous namespaces correctly:
   - `namespace { int x; }`
   - anonymous namespaces need identity, not empty-name hacks

3. It does not represent scope structure:
   - `namespace a { namespace b { ... } }`
   - lookup should walk scope/context, not only compare strings

4. It does not capture lexical vs semantic context:
   - out-of-line definitions and later extensions of a namespace need this

5. It leaks into later stages:
   - HIR symbol identity
   - LLVM IR naming
   - mangling / quoting / canonicalization

So string flattening can be a compatibility layer, but it cannot be the core
representation.

## Target Model

The frontend should gain a minimal namespace model with these properties.

### 1. Namespace Scope Stack

While parsing, track a stack of namespace contexts rather than a single
concatenated string.

Each stack entry should carry at least:

- namespace kind: named or anonymous
- source-level name if named
- stable internal id if anonymous
- pointer/index to parent scope

### 2. Structured Qualified Name

Qualified names should be parsed into structure, not stored only as text.

Minimum shape:

- `is_global_qualified`
- `segments`
- `base_name`

Examples:

- `::oo::a`
  - global-qualified = true
  - segments = [`oo`]
  - base = `a`

- `xx::some_xx_var`
  - global-qualified = false
  - segments = [`xx`]
  - base = `some_xx_var`

### 3. Declaration Context

Every declaration that can appear in namespace scope should know its enclosing
namespace context.

At minimum:

- global variables
- functions
- structs/classes
- enums
- typedef / using aliases
- templates

This can be represented as a scope id or pointer to a namespace context node.

### 4. Lookup By Context

Name lookup should stop depending primarily on flattened strings.

Instead:

- unqualified lookup should search the active scope chain
- qualified lookup should resolve the qualifier to a context, then search there
- `using` declarations/directives should alter visibility rules, not just
  rewrite strings

### 5. Anonymous Namespace Identity

Anonymous namespaces need a stable internal identity.

Requirements:

- two anonymous namespaces are never the same entity just because both are
  unnamed
- declarations inside an anonymous namespace remain reachable within the same
  TU through the correct scope rules
- later stages can derive internal linkage behavior from this representation

## Non-Goals For The First Refactor

The first proper namespace refactor does not need to solve everything.

Out of scope for the first pass:

- namespace alias declarations
- inline namespaces
- ADL correctness
- full Clang-equivalent lookup rules
- modules
- all mangling/linkage subtleties

But the design must leave room for them.

## Implementation Strategy

## Phase 0: Stop Digging

Objective:
Do not extend the string-flattening approach further than needed for temporary
compatibility.

Rules:

- no new features should depend solely on `current_namespace_` string joins
- new tests are allowed
- temporary bridging code is allowed only if it is isolated and removable

Exit condition:

- namespace work is guided by explicit context-based milestones

## Phase 1: Introduce Namespace Context Objects

Objective:
Create an internal representation for namespace scopes.

Required slices:

- add a namespace context structure
- assign a stable root/global context
- assign stable ids for named and anonymous namespace entries
- track parser namespace stack using these contexts

Suggested minimum structure:

```cpp
struct NamespaceContext {
  int id;
  int parent_id;
  bool is_anonymous;
  const char* display_name;
};
```

Exit condition:

- parser no longer needs a single flattened `current_namespace_` string as the
  primary namespace representation

## Phase 2: Represent Qualified Names Structurally

Objective:
Teach the parser to retain qualified-id structure.

Required slices:

- parse leading `::`
- parse namespace segment chains as structure
- attach qualified-name structure to relevant AST nodes

Candidate places:

- expression variable references
- type references
- declarations written with qualified names

Exit condition:

- parser can distinguish `::oo::a` from `oo::a`
- AST no longer has to reconstruct this from flat text

## Phase 3: Declaration Ownership By Context

Objective:
Bind declarations to namespace contexts explicitly.

Required slices:

- global var/function/type declarations record owning namespace context
- namespace blocks merge into the same logical context when reopened
- anonymous namespaces create unique child contexts

Exit condition:

- a declaration's namespace ownership can be queried without parsing its name

## Phase 4: Context-Based Lookup

Objective:
Move name resolution rules onto contexts.

Required slices:

- unqualified lookup walks local scope, then namespace context chain
- qualified lookup resolves qualifier context first
- `using ns::name;` imports a declaration into visible lookup state
- `using namespace ns;` imports a context into visible lookup state

Exit condition:

- lookup behavior is no longer implemented mainly as string remapping

## Phase 5: Lowering And Codegen Integration

Objective:
Make later stages consume namespace structure cleanly.

Required slices:

- HIR carries namespace ownership or canonical symbol scope
- codegen emits stable legal symbol names for namespace-owned entities
- LLVM IR naming does not depend on raw `::` text surviving as-is

Exit condition:

- runtime namespace tests pass without emitter hacks that depend on raw source
  spelling

## Test Strategy

We should keep two classes of tests:

### Parse Tests

Used to lock down syntax shape and AST survivability.

Examples:

- nested named namespaces
- anonymous namespace
- `using` alias / declaration / directive
- cross-namespace qualified references
- leading `::`

### Runtime / Frontend Tests

Used to prove the model is semantically useful and not just parse-tolerant.

These are the real completion signal.

Examples:

- namespace global variable access
- namespace function calls
- `::global_name` from inside a namespace
- `::oo::a` self-reference
- cross-namespace variable/function lookup
- anonymous namespace intra-TU visibility

## Current Evidence

The current codebase already demonstrates the weakness of the string approach:

- parser state still contains:
  - `current_namespace_`
  - `using_value_aliases_`
  - `using_namespace_prefixes_`
- these are transitional conveniences, not the target architecture
- codegen has already started feeling the pain because raw qualified names
  bleed into LLVM symbol naming

That is a sign we should refactor at the representation layer, not keep adding
exceptions downstream.

## Success Criteria

This plan is successful when:

1. namespace support is based on explicit scope/context data, not primarily on
   concatenated strings
2. qualified-id structure is preserved in parser/AST
3. anonymous namespaces have stable internal identity
4. namespace runtime tests pass, not just parse tests
5. later work for `<vector>` can build on this model without adding more
   namespace-specific hacks

## Failure Modes To Avoid

- adding more parser-side string rewriting to fake lookup
- teaching codegen to special-case raw `a::b::c` names instead of fixing symbol
  identity upstream
- making anonymous namespace support depend on empty strings
- declaring namespace support "done" when only parse-only tests pass

## Immediate Next Step

Start Phase 1 directly:

- define namespace context objects
- replace `current_namespace_` as the primary source of truth
- keep string qualification only as a temporary compatibility layer during the
  migration
