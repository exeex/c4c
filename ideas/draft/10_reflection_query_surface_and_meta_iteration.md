# Reflection Query Surface And Meta Iteration

Status: Draft
Last Updated: 2026-04-10

## Goal

Define the user-facing reflection and meta-iteration surface that sits on top
of the HIR query substrate, so code like:

```cpp
constexpr auto S = reflect(is_instruction && in_namespace("npu"));

for meta (InstructionSchema x : S) {
  x.operands();
  x.results();
  x.encoding();
}
```

becomes a real compile-time language feature rather than a design sketch.

## Why This Idea Exists

The previous idea established that replacing TableGen requires a query engine,
not just ad hoc type introspection.

That still leaves the next hard problem:

- how users spell reflective queries
- what `reflect(...)` returns
- when that result becomes a stable snapshot
- how `for meta` executes over that result
- how schema concepts participate in binding and validation
- how query results interact with `constexpr` and `consteval`

Without this layer, c4c may have an internal schema database but still lack
the language surface needed to consume it ergonomically.

## Relationship To The Query-Substrate Idea

This idea depends on
[09_hir_query_substrate_for_reflection_driven_schema_generation.md](/workspaces/c4c/ideas/draft/09_hir_query_substrate_for_reflection_driven_schema_generation.md).

The split is intentional:

- the query-substrate idea defines universe, entity identity, predicate
  semantics, and deterministic ordering
- this idea defines the language surface that lowers into those query
  facilities

In short:

- previous idea: compiler-internal query model
- this idea: user-facing reflection and `for meta`

## Main Objective

Provide a staged language design for:

- `reflect(...)` as query materialization syntax
- `for meta` as compile-time iteration over query snapshots
- concept-constrained meta bindings such as `for meta (InstructionSchema x : S)`
- query-result accessors for fields, traits, names, operands, and results
- `consteval` helpers as the replacement for embedded C++ fragments in TableGen

The key design constraint is that the reflection surface must feel like a
native compile-time query API, not like a disguised `.td` clone.

## Core Semantic Model

This idea assumes three semantic layers:

1. query universe
2. query materialization
3. meta execution

### 1. Query universe

The universe is compiler-owned and comes from normalized HIR plus compile-time
state. User code does not construct it manually.

### 2. Query materialization

`reflect(...)` does not yield a lazy cursor into mutable compiler state.
It yields a deterministic snapshot value representing the result of a query
against the current compile-time universe.

### 3. Meta execution

`for meta` executes at compile time over a snapshot and can:

- inspect each selected entity
- validate that each bound entity satisfies the declared meta concept
- invoke `consteval` helpers
- produce compile-time data or generated declarations

## Proposed Surface Syntax

The first surface should stay compact.

### Reflect query

```cpp
constexpr auto S = reflect(is_instruction &&
                           in_namespace("npu"));
```

Alternative surface forms may exist later, but the important first property is
that `reflect(...)` looks like query materialization, not like an attribute or
pragma.

### Meta iteration

```cpp
for meta (InstructionSchema x : S) {
  x.operands();
  x.results();
  x.encoding();
}
```

This should mean:

- iterate the snapshot in deterministic order
- bind `x` as an `InstructionSchema`-constrained reflection handle
- run the loop body in compile-time-only context
- validate each element against `InstructionSchema`
- emit a hard compile-time error if any element in `S` does not satisfy the
  declared concept

### Direct query-to-loop form

The language may later permit:

```cpp
for meta (InstructionSchema x :
          reflect(is_instruction && in_namespace("npu"))) {
  x.operands();
}
```

But the named-snapshot form is still important because it makes snapshot
semantics explicit and easier to reason about.

### Concept role

The intended split is:

- `reflect(...)` performs query selection and filtering
- the concept in `for meta (Concept x : S)` performs validation only

Concepts should not silently filter the range. They should behave like
constraints:

- if every element satisfies the concept, the loop is well-formed
- if any element does not satisfy it, the loop is ill-formed

This keeps query semantics and type-constraint semantics separate.

## What `reflect(...)` Returns

The first version should return an ordinary compile-time value type rather than
a magical implicit compiler list.

Suggested direction:

```cpp
namespace meta {

class query_result {
 public:
  constexpr size_t size() const;
  constexpr bool empty() const;
  constexpr entity operator[](size_t i) const;
};

}
```

Important properties:

- value-semantic snapshot
- deterministic ordering
- safe to bind to `constexpr auto`
- read-only from user code
- generic with respect to schema concepts; concept refinement happens at the
  `for meta` binding site

This should not expose mutable access to compiler state.

## What An Entity Handle Looks Like

The base entity object exposed by `query_result` should be a generic reflective
handle, not a raw HIR pointer and not a string name.

Possible direction:

```cpp
namespace meta {

class entity {
 public:
  constexpr entity_kind kind() const;
  constexpr string_view name() const;
  constexpr bool in_namespace(string_view) const;

  constexpr bool has_trait<Trait>() const;
  constexpr auto fields() const;
  constexpr auto operands() const;
  constexpr auto results() const;
  constexpr auto bases() const;
};

}
```

The exact method set can grow over time, but the principle should stay stable:

- entity queries are field/relationship oriented
- entity handles are compiler-owned views
- user code reads metadata, it does not mutate HIR directly

When a concept-constrained meta binding is used:

```cpp
for meta (InstructionSchema x : S) {
  x.operands();
  x.results();
  x.encoding();
}
```

the bound `x` should behave like a refined schema view over the generic entity.
That is where schema-specific APIs become available.

In other words:

- `query_result` holds generic entities
- `for meta (InstructionSchema x : S)` refines each entity to the schema view
- the concept check guards access to schema-specific attributes

## Snapshot Semantics

This is the most important semantic rule.

`reflect(...)` must produce a stable snapshot.

That means:

- iteration order does not change while iterating
- results are not affected mid-loop by newly discovered entities
- repeated `reflect(P)` in the same compile-time state may produce equivalent
  snapshots

This avoids a whole class of confusing behavior where compile-time execution
changes the set it is currently traversing.

### Initial rule

The first implementation should define:

- a query observes the normalized compile-time universe at the point the
  `reflect(...)` expression is evaluated
- entities discovered later require a new `reflect(...)` evaluation to appear
- `for meta` iterates a frozen result set, not a live view
- concept validation happens against the frozen result set, not a changing
  universe

This is slightly stricter than the most dynamic possible design, but it is far
easier to reason about and aligns better with deterministic code generation.

## Execution Rules For `for meta`

The loop body should be a compile-time-only evaluation context.

Allowed directions for the first slice:

- `consteval` function calls
- compile-time branching on reflected metadata
- generation of compile-time tables / declarations / helper definitions
- concept-constrained bindings that expose schema-specific APIs after validation

Disallowed or deferred directions:

- arbitrary runtime side effects
- mutation of the query snapshot being iterated
- undefined ordering through parallel expansion
- silent concept-based filtering of loop elements

If the language later supports declaration synthesis inside `for meta`, that
should still preserve source-order and deterministic-expansion rules.

## `consteval` As The Replacement For Embedded C++ In TableGen

TableGen often relies on backend-specific emitters or embedded C++ snippets to
compute derived values.

The c4c-native replacement should be:

- schema declarations represented as C++ types or declarations
- reflection query results represented as generic `meta::entity` handles
- schema-specific loop bindings represented as concept-checked meta views
- derived computations implemented as `consteval` functions

Example direction:

```cpp
consteval auto build_encoding(InstructionSchema auto op) {
  ...
}

constexpr auto ops = reflect(is_instruction && in_namespace("npu"));

for meta (InstructionSchema op : ops) {
  constexpr auto enc = build_encoding(op);
  ...
}
```

This keeps the escape hatch inside the host language instead of tunneling out
to an external emitter script.

## TableGen Alignment

The long-term mapping should be explicit.

### Core alignment

- `.td` → C++ schema declarations
- `class` / `def` → schema types or schema-bearing declarations
- `let` → `constexpr` field override or trait attachment
- `foreach` → `for meta`
- embedded C++ in `.td` → `consteval` functions over reflective entities

### Database and expansion alignment

- record database → HIR query universe
- `defset` / global record collection → `reflect(query)`
- `multiclass` / `defm` expansion → `consteval` or meta-driven schema expansion
- backend emitter pass → compile-time consumer over query snapshots

The point is not to mimic TableGen syntax. The point is to cover the same power
surface with a better host-language model.

## Suggested Minimal Feature Slice

The first slice should be intentionally narrow.

### Phase 1. Read-only reflective queries

- `reflect(predicate)` produces a `query_result`
- `for meta` iterates query results
- `for meta (Concept x : S)` validates each element against the concept
- entity handles support `kind`, `name`, namespace membership, and a small set
  of generic field accessors

### Phase 2. Trait / relation predicates

- `has_trait<T>()`
- `derived_from<T>()`
- schema-specific bindings and accessors such as operands/results/encoding
  fields after concept validation

### Phase 3. Meta-generated output

- declaration or table synthesis from `for meta`
- consteval helpers that lower reflection data into generated IR/schema tables

This keeps syntax and semantics moving together without blocking on the final
code-generation story.

## Concrete Injection Points

Likely ownership areas:

- [src/frontend/hir/hir_ir.hpp](/workspaces/c4c/src/frontend/hir/hir_ir.hpp)
- new frontend/meta or reflection surface headers under `src/frontend/`
- compile-time evaluation machinery adjacent to
  [src/frontend/hir/compile_time_engine.hpp](/workspaces/c4c/src/frontend/hir/compile_time_engine.hpp)
- parser/sema work that recognizes `reflect(...)` and `for meta`

The important architectural boundary is:

- parser/sema owns syntax recognition and type-checking of reflection surface
- query engine owns entity discovery and filtering
- compile-time engine owns evaluation and meta execution

## Open Semantic Questions

These should be resolved before implementation hardens:

- Is `reflect(...)` permitted in any `constexpr` context, or only in
  compile-time/meta contexts?
- Does `for meta` introduce declarations into the surrounding scope directly,
  or through a generated intermediate form?
- How are cross-TU and module boundaries exposed to reflection queries?
- What is the error model when a query predicate references unsupported
  metadata?
- Which entity kinds are in scope for the first shipped slice?
- What exact internal object model backs concept-refined meta bindings?

## Constraints

- prioritize deterministic semantics over maximal dynamism
- keep `reflect(...)` snapshot-based, not live-view based
- keep the first version read-only from user code
- avoid forcing users to understand raw HIR internals
- keep concepts as validators, not filters
- do not import TableGen syntax into the language when native C++ surface is
  clearer

## Validation

At minimum:

- parser/sema tests for `reflect(...)` and `for meta` syntax
- compile-time tests proving stable snapshot iteration
- regression tests showing repeated queries are deterministic
- tests that concept-constrained meta bindings reject mismatched elements
- tests that `consteval` helpers can consume concept-refined entities
- targeted golden tests for a small TableGen-like schema example lowered
  through the reflection surface

## Non-Goals

- no requirement to ship full standardized runtime reflection
- no requirement to support every possible schema transform in the first slice
- no requirement to synthesize arbitrary syntax before read-only reflection is
  stable
- no attempt to exactly emulate every corner of TableGen syntax

## Success Criteria

This idea succeeds when c4c can support a believable end-to-end pattern such
as:

```cpp
constexpr auto ops = reflect(is_instruction && in_namespace("npu"));

for meta (InstructionSchema op : ops) {
  constexpr auto encoding = build_encoding(op);
  ...
}
```

and when that surface has a clear, explicit mapping to the TableGen use cases
it is meant to replace.
