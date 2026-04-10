# HIR Query Substrate For Reflection-Driven Schema Generation

Status: Draft
Last Updated: 2026-04-10

## Goal

Define a HIR-owned query substrate that can enumerate, filter, and inspect
compiler-known schema objects at compile time, so future reflection / `@meta`
features can replace TableGen-style record databases with a native query model.

## Why This Idea Exists

The hard part of replacing TableGen is not emission. The hard part is giving
the language a compile-time way to answer questions like:

- what schema-bearing declarations exist in the current universe
- which of them satisfy trait or inheritance predicates
- which fields / operands / results belong to each selected item
- how to iterate those results deterministically

Today c4c already has important pieces of that foundation:

- HIR is the typed frontend IR and already carries stable typed ids for many
  entities
- HIR `Module` owns deterministic vectors for functions, globals, expressions,
  and ordered struct-definition emission
- compile-time normalization already owns registries for template definitions,
  specializations, and realized instantiations
- parser / frontend state already preserves namespace context and concept-ish
  visibility information

But those pieces are still exposed as ad hoc containers and stage-local helper
lookups. There is no first-class query layer that turns them into a stable
schema universe.

## Main Objective

Introduce a narrow, explicit query layer between raw HIR storage and future
reflection syntax:

- a compiler-owned universe of queryable entities
- composable predicates over those entities
- deterministic snapshot iteration
- field/trait accessors that read HIR metadata instead of reparsing source text

The immediate goal is not to ship full language reflection. The immediate goal
is to make HIR and compile-time state capable of serving as the query engine
that reflection will later expose.

## Current Code Shape

The current frontend already has several useful attachment points.

### 1. HIR has stable entity identity, but not a query surface

Primary file:

- [hir_ir.hpp](/workspaces/c4c/src/frontend/hir/hir_ir.hpp)

Today `hir::Module` already owns:

- `std::vector<Function> functions`
- `std::vector<GlobalVar> globals`
- `std::vector<Expr> expr_pool`
- `std::unordered_map<SymbolName, FunctionId> fn_index`
- `std::unordered_map<SymbolName, GlobalId> global_index`
- `std::unordered_map<SymbolName, HirStructDef> struct_defs`
- `std::vector<SymbolName> struct_def_order`
- `std::unordered_map<SymbolName, HirTemplateDef> template_defs`

And core entity structs already carry metadata that future queries will need:

- `Function.id`, `Function.name`, `Function.ns_qual`, `Function.template_origin`,
  `Function.spec_key`
- `GlobalVar.id`, `GlobalVar.name`, `GlobalVar.ns_qual`
- `HirStructDef.tag`, `HirStructDef.ns_qual`, `HirStructDef.fields`,
  `HirStructDef.base_tags`
- `HirTemplateDef.name`, `template_params`, `param_is_nttp`, `instances`

This is enough to say "the data exists", but not enough to say "the data is
queryable". Callers still need to know which container to scan and which
storage-specific conventions define identity and ordering.

### 2. Compile-time normalization already has a partial universe registry

Primary file:

- [compile_time_engine.hpp](/workspaces/c4c/src/frontend/hir/compile_time_engine.hpp)

`CompileTimeState` and `InstantiationRegistry` already provide:

- template function definition registration
- template struct primary/specialization registration
- consteval function registration
- explicit specialization lookup
- realized instantiation enumeration via `all_instances()`
- owner-based specialization keys and parity checking

This is close to a queryable schema database, but it is still shaped as engine
internals for template realization, not as a general-purpose reflective
universe. The exposed APIs are mostly name-keyed maps and specialized lookup
helpers rather than query combinators.

### 3. Namespace and visibility metadata already exist before HIR

Primary files:

- [parser_core.cpp](/workspaces/c4c/src/frontend/parser/parser_core.cpp)
- [parser.hpp](/workspaces/c4c/src/frontend/parser/parser.hpp)

The parser already tracks:

- namespace context ids
- canonical namespace names
- using-directive visibility edges
- concept-name visibility probes

HIR lowering preserves part of this through `NamespaceQualifier` and
`context_id`. That means future query predicates such as `in_namespace(...)`
have a real substrate already; they do not need to start from textual
reconstruction.

## Gap Analysis

What is missing today is not more metadata. What is missing is query semantics.

Specifically, c4c does not yet define:

- a single "queryable universe" spanning relevant HIR and compile-time
  registries
- a common entity reference type for functions, structs, globals, templates,
  instantiations, and future schema-bearing declarations
- composable predicates over those entities
- deterministic query result ordering rules
- snapshot/materialization rules for when a query sees newly discovered
  compile-time entities

Without those pieces, reflection would degrade into isolated
"inspect this one type" helpers instead of a TableGen-replacing query engine.

## Proposed Data Model

The first slice should stay simple and explicit.

### Queryable entity references

```cpp
enum class QueryEntityKind : uint8_t {
  Function,
  Global,
  StructDef,
  TemplateDef,
  TemplateInstance,
};

struct QueryEntityRef {
  QueryEntityKind kind{};
  uint32_t index = 0;
};
```

This should be a lightweight handle into compiler-owned storage, not an owned
copy of metadata.

### Universe snapshot

```cpp
struct QueryUniverse {
  const Module* module = nullptr;
  const CompileTimeState* ct_state = nullptr;

  std::vector<QueryEntityRef> entities;
};
```

The first version can materialize `entities` eagerly in deterministic order:

1. `Module.functions` order
2. `Module.globals` order
3. `Module.struct_def_order`
4. `Module.template_defs` in stable key order
5. realized template instances in stable owner/spec-key order

That gives future `for meta` or compile-time loops a stable snapshot.

### Predicate surface

```cpp
struct QueryPredicate {
  bool (*eval)(const QueryUniverse&, QueryEntityRef) = nullptr;
};

QueryPredicate kind_is(QueryEntityKind kind);
QueryPredicate in_namespace(std::string_view canonical_name);
QueryPredicate has_template_origin();
QueryPredicate is_consteval_only();
QueryPredicate is_template_instance();
```

The first implementation does not need a fancy embedded DSL. It only needs a
composable internal algebra that later syntax can lower into.

## Suggested First Query API

Keep the API compiler-internal first.

```cpp
class HirQueryEngine {
 public:
  QueryUniverse snapshot(const Module&, const CompileTimeState&) const;
  std::vector<QueryEntityRef> filter(
      const QueryUniverse&, QueryPredicate) const;

  const Function* get_function(const QueryUniverse&, QueryEntityRef) const;
  const GlobalVar* get_global(const QueryUniverse&, QueryEntityRef) const;
  const HirStructDef* get_struct_def(const QueryUniverse&, QueryEntityRef) const;
  const HirTemplateDef* get_template_def(const QueryUniverse&, QueryEntityRef) const;
  const HirTemplateInstantiation* get_template_instance(
      const QueryUniverse&, QueryEntityRef) const;
};
```

This keeps the first slice honest:

- universe construction is explicit
- result materialization is explicit
- callers can build targeted predicates before any user-facing syntax exists

## Concrete Injection Points

### 1. Add query-owned identity and ordered views to HIR

Primary file:

- [hir_ir.hpp](/workspaces/c4c/src/frontend/hir/hir_ir.hpp)

Potential first additions:

- `QueryEntityKind`
- `QueryEntityRef`
- stable ordered template-definition view helper
- stable ordered template-instance view helper

The immediate goal is not to rewrite existing containers, only to provide
ordered traversal helpers that stop query users from depending on
`unordered_map` iteration.

### 2. Build universe snapshots after HIR normalization

Primary files:

- [hir.cpp](/workspaces/c4c/src/frontend/hir/hir.cpp)
- [hir_build.cpp](/workspaces/c4c/src/frontend/hir/hir_build.cpp)
- [compile_time_engine.hpp](/workspaces/c4c/src/frontend/hir/compile_time_engine.hpp)

The best first lifecycle point is after compile-time normalization has
converged and realized deferred instantiations. At that point:

- HIR functions/globals/structs are available
- template instance parity has already been checked
- query snapshots can treat the normalized module as their default universe

### 3. Preserve query-relevant metadata during lowering

Primary files:

- [hir_lowering_core.cpp](/workspaces/c4c/src/frontend/hir/hir_lowering_core.cpp)
- [hir_functions.cpp](/workspaces/c4c/src/frontend/hir/hir_functions.cpp)
- [hir_types.cpp](/workspaces/c4c/src/frontend/hir/hir_types.cpp)

Existing metadata that should become query-friendly includes:

- canonical namespace identity
- explicit base/trait-like relationships in struct metadata
- template origin / specialization-key identity
- consteval-only and materialization state

If any of these remain stringly-typed or stage-local, this idea should tighten
them before user-facing reflection depends on them.

## Phased Plan

### Phase 1. Internal query substrate

- define `QueryEntityRef` and deterministic ordered traversal helpers
- build a compiler-internal `HirQueryEngine`
- support simple predicates over kind, namespace, template origin, and
  consteval/materialization flags

### Phase 2. Schema-oriented metadata cleanup

- normalize metadata needed for trait / inheritance-style predicates
- ensure template instances and schema-bearing declarations have stable
  identity and ordered enumeration
- stop query code from depending on unordered container order

### Phase 3. Reflection / `@meta` bridge

- lower reflection syntax into query-engine predicates
- add compile-time iteration over snapshot results
- use the query engine to drive generated schema tables or rewrite tables

## Constraints

- keep the first slice compiler-internal; do not design final syntax first
- deterministic ordering is required from day one
- prefer typed ids / refs over string-keyed scans where identity already exists
- do not force one giant global registry if HIR + compile-time state can be
  stitched together cleanly through snapshot construction
- do not block on full standardized reflection support before building the
  internal query model

## Validation

At minimum:

- unit-style coverage for deterministic query ordering
- regression tests proving query snapshots see normalized template instances
- targeted tests for namespace-filtered and kind-filtered enumeration
- debug dump support that prints snapshot contents in stable order

## Non-Goals

- no requirement to ship final user-facing reflection syntax in the first slice
- no requirement to fully replace TableGen in one step
- no attempt to model every AST node as a queryable entity
- no runtime reflection system; this is compile-time compiler-owned metadata
  only

## Success Criteria

This idea succeeds when c4c can truthfully say:

- the compiler has a deterministic compile-time universe of schema entities
- that universe can be filtered without ad hoc container knowledge
- future reflection / `@meta` features can consume query snapshots instead of
  inventing a second metadata database

At that point, "reflection replacing TableGen" stops being a slogan and starts
being a staged engineering path.
