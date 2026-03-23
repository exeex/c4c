# Lazy Template Type Instantiation Plan

## Goal

Move template type realization away from parser-time guessing and toward a
single lazy instantiation model that runs with complete concrete bindings.

This plan is specifically about:

- dependent NTTP defaults such as `bool = trait<T>::value`
- dependent member typedefs such as `typename Helper<T>::type`
- alias templates and template struct bases that currently require partial
  early resolution
- making template type realization converge through a compile-time fixpoint

This plan is **not** about:

- replacing all parser support for templates
- implementing full C++ template semantics
- adding SFINAE, concepts, or full Clang-like partial ordering
- removing all eager instantiation forever

The narrower goal is:

- parser preserves dependency information
- HIR carries that information forward
- compile-time engine realizes template types lazily from actual use sites


## Problem Statement

Today the frontend splits responsibility for template realization across three
stages:

1. parser
   - parses template structs, alias templates, defaults, and some dependent
     references
   - also eagerly tries to evaluate deferred NTTP defaults
   - sometimes instantiates template structs during `parse_base_type()`

2. AST to HIR
   - resolves some pending template struct refs using concrete bindings
   - instantiates concrete HIR structs on demand
   - has begun preserving deferred refs such as NTTP `$expr:` and member-type
     references

3. compile-time engine
   - runs a fixpoint loop
   - resolves deferred template **calls**
   - evaluates pending consteval expressions

The problem is that type-driven template realization still happens too early
and in too many places.

That causes failures like:

- parser-time NTTP default evaluation without enough information
- alias/member-type chains that collapse to fallback types
- specialization selection that happens before full owner realization
- separate ad hoc logic for calls, bases, typedefs, and aliases


## Core Conclusion

Dependent template type realization should be driven by concrete use sites, not
by parser-time approximation.

In practice that means:

- parser should preserve unresolved template structure and default expressions
- HIR should preserve unresolved template type work items
- compile-time engine should gain a worklist for template **type**
  instantiation, not only template function calls

The key design principle is:

> only instantiate or evaluate when we have the concrete bindings from the
> actual declaration / use site that needs the type


## Why Parser-Time Evaluation Is The Wrong Place

For a dependent default like:

```cpp
template <typename T, bool = arithmetic<T>::value>
struct is_signed_helper;
```

the parser does not inherently know the right answer.

It only sees a token sequence that refers to:

- a template owner `arithmetic<T>`
- a member `value`
- possibly a specialization that depends on `T`

The right result is only knowable when we instantiate something like:

- `is_signed_helper<int>`
- `is_signed_helper<unsigned int>`
- `is_signed_helper<void>`

At that point we finally know:

- the concrete type binding for `T`
- which specialization of `arithmetic<T>` should be selected
- which concrete owner struct exists
- what `value` or `type` means on that concrete owner

So parser-time evaluation should be limited to:

- non-dependent literals
- structurally preserving unresolved expressions
- recording deferred references in a durable form

It should not be the final authority for dependent defaults.


## Current Compile-Time Engine: What It Already Does

The current engine already has one important property:

- it runs a fixpoint loop until compile-time work converges

Today it mainly handles:

- deferred template function instantiation
- pending consteval evaluation

That is already the right *shape* of solution.

What is missing is scope.

The engine does **not yet** own all type-driven template realization such as:

- template struct instantiation discovered from type uses
- member typedef realization like `Owner<T>::type`
- dependent base realization
- dependent NTTP default evaluation used for struct specialization selection

So this plan extends the existing engine instead of replacing it.


## Target Model

The final model should work like this:

1. parser records unresolved template type structure
   - owner template name
   - template arg refs
   - deferred NTTP default expression text/tokens
   - deferred member typedef name such as `type`

2. AST to HIR lowers these to explicit pending type references
   - no forced early evaluation for dependent cases
   - only obviously concrete cases may resolve immediately

3. compile-time engine walks all pending template type refs
   - resolve owner template instantiation
   - fill dependent NTTP defaults using concrete bindings
   - select specialization
   - realize concrete struct / alias / member typedef
   - repeat until no new type instantiations are needed

4. downstream HIR lowering/codegen only sees concrete types or explicit
   unresolved diagnostics


## First Migration Targets

Before changing architecture further, we should define exactly what gets moved
out of `ast_to_hir.cpp` and into `src/frontend/hir/compile_time_engine.cpp`
first.

The practical rule is:

- move work that is currently *type-driven* and *use-site driven*
- keep work that is just *syntax preservation* or *obviously concrete lowering*

That gives us a narrow first target instead of trying to move all template
logic at once.


### Target A: Concrete Template Struct Realization

This is the biggest current owner of early type work.

Today `resolve_pending_tpl_struct()` in `ast_to_hir.cpp` is doing all of these
jobs at once:

- parse deferred template arg refs
- substitute concrete type/NTTP bindings
- evaluate deferred NTTP defaults
- select template struct specialization
- build the mangled concrete struct name
- instantiate the concrete HIR struct
- lower methods immediately
- rewrite the original `TypeSpec` in place

That is already compile-time-engine-shaped work, just living in the wrong
file.

So the first target should be:

- extract the realization algorithm behind `resolve_pending_tpl_struct()`
- re-home the worklist-driving part in `compile_time_engine.cpp`
- leave `ast_to_hir.cpp` responsible only for creating/preserving the pending
  `TypeSpec`

Concretely this means the engine should become the owner of:

- pending `TypeSpec{ tpl_struct_origin, tpl_struct_arg_refs }`
- struct instantiation dedup
- specialization selection for template structs
- post-instantiation metadata registration


### Target B: Dependent NTTP Default Evaluation For Struct Templates

This is the cleanest semantic target to move early.

Right now dependent NTTP defaults are effectively decided inside the template
struct realization path, not by the fixpoint engine:

- explicit deferred expressions are evaluated while resolving args
- missing NTTP defaults are materialized inside the same helper

That is exactly the behavior we want to defer.

So the first semantic move should be:

- keep parser preservation of `template_param_default_exprs`
- stop treating `ast_to_hir.cpp` as the final authority for evaluating them
- make the compile-time engine evaluate those defaults only when a concrete
  pending type work item is being realized

If we only move one "meaningful" behavior first, this is probably the best
one, because it removes the main reason parser/HIR currently need to guess.


### Target C: Dependent Member Typedef Resolution

`Owner<T>::type`-style lookup is another clear engine candidate.

Today `resolve_struct_member_typedef_hir()` in `ast_to_hir.cpp` performs:

- struct-scope alias lookup
- binding substitution
- inherited lookup through realized bases
- recursive nested member-type resolution
- opportunistic triggering of template struct realization on the owner

This is also use-site-driven type work, so it belongs next to the template
type fixpoint rather than inline in lowering.

The target is:

- `ast_to_hir.cpp` preserves `deferred_member_type_name`
- engine resolves it once the owner type is concrete
- if the member alias expands to another pending template type, requeue that
  work instead of recursively finishing everything inside lowering


### Target D: Dependent Base Types During Struct Realization

Base realization should move together with template struct realization, not as
a separate parser-side feature.

Today concrete struct instantiation also resolves bases inline:

- substitute template bindings into base type
- instantiate pending template bases
- resolve base member typedef chains
- append concrete base tags directly into the HIR struct def

This should move as part of the same engine-owned pipeline:

1. realize concrete owner struct
2. inspect each pending base
3. enqueue dependent base owner/member work
4. finalize base tags only after the base type is concrete

This keeps inheritance on the same path as aliases/member typedefs instead of
giving bases bespoke logic forever.


### What Should Not Move In The First Pass

To keep the first step tractable, we should explicitly *not* move everything.

The following can stay where they are for now:

- parser token/text preservation for deferred defaults
- parser construction of pending `TypeSpec` metadata
- obviously concrete builtin/default type materialization
- template function call seeding already handled by the existing engine
- final method lowering callback once a concrete struct instance is known

In other words, first move "decide what concrete type this should become",
then later decide whether method lowering itself also needs to become more
lazy.


### Recommended Move Order

If we want the smallest viable migration path, the order should be:

1. add engine-side pending template type work items
2. move template struct realization entrypoints there
3. move dependent NTTP default evaluation there
4. move member typedef / dependent base follow-up resolution there
5. only then delete the eager `ast_to_hir.cpp` resolution paths

This order works because it moves the central owner first and lets other
dependent features collapse onto the same path.


## Required Representation Changes

### 1. Preserve Deferred NTTP Defaults As Data

We already have token preservation for NTTP defaults.

We should keep that as the parser-side truth for dependent defaults.

Required property:

- parser stores expression tokens/text
- parser does not have to compute the final value for dependent cases


### 2. Preserve Deferred Member Type References

We need a stable way to represent:

- `Owner<T>::type`
- `Alias<T>::template rebind<U>` if supported later

Minimum viable first step:

- keep `deferred_member_type_name`
- pair it with the owner type ref
- only resolve after the owner is concrete


### 3. Carry Struct Member Typedef Metadata Into HIR

Member aliases inside struct bodies must not remain parser-only state.

HIR-side type realization needs visibility into:

- struct-scope `typedef`
- struct-scope `using Name = Type`

Otherwise the compile-time engine cannot resolve `Owner::type` without calling
back into parser internals.


### 4. Represent Pending Template Type Work Explicitly

The engine currently has machinery for pending consteval and unresolved
template calls.

We need a similar work item for type realization.

Example conceptual shape:

```cpp
struct PendingTemplateTypeRef {
  TypeSpec owner;
  std::string member_typedef;
  TypeBindings type_bindings;
  NttpBindings nttp_bindings;
  SourceSpan span;
};
```

This does not need to be the exact final type.
The important part is that type-driven work becomes first-class engine input.


## Execution Model

## Phase 0: Freeze New Parser-Time Evaluation

Objective:
Stop expanding parser responsibility for dependent template semantics.

Rules:

- parser may preserve deferred refs
- parser may evaluate non-dependent defaults
- parser should not gain new logic that finalizes dependent `trait<T>::value`

Exit condition:

- new template-type fixes default to preserve-first, not evaluate-first


## Phase 1: Define Ownership Boundaries

Objective:
Make the stage responsibilities explicit.

Parser owns:

- syntax
- token preservation
- unresolved type structure

AST to HIR owns:

- lowering unresolved structure into HIR-visible pending refs
- immediate resolution only when bindings are already concrete

Compile-time engine owns:

- fixpoint realization of pending template types
- dependent NTTP default evaluation
- dependent member typedef resolution

Exit condition:

- code comments and helper APIs reflect these boundaries


## Phase 2: Add HIR-Visible Member Typedef Metadata

Objective:
Make struct member typedefs/queryable aliases visible to HIR and the engine.

Required slices:

- record member typedef/using aliases on struct AST nodes
- lower that metadata into HIR-side lookup structures
- support inherited lookup through concrete base tags

Exit condition:

- `Owner::type` can be answered by HIR-owned metadata after concrete owner
  realization


## Phase 3: Introduce Pending Template Type Worklist

Objective:
Generalize the compile-time engine from call-driven work to type-driven work.

Required slices:

- add pending template type work items
- seed them from:
  - pending struct types
  - dependent base types
  - dependent member typedef refs
  - dependent alias template expansions
- deduplicate work by specialization key / concrete owner

Exit condition:

- engine can repeatedly instantiate needed template types until convergence


## Phase 4: Move Dependent NTTP Default Evaluation Into The Engine

Objective:
Make dependent defaults evaluate only when bindings are concrete.

Required slices:

- treat parser-stored NTTP default expressions as deferred inputs
- evaluate them only during concrete template type realization
- perform specialization selection after defaults are materialized in the
  engine, not before

Important rule:

- non-dependent literal defaults can still be materialized early for
  convenience
- dependent defaults must go through the lazy path

Exit condition:

- parser no longer decides the result of `trait<T>::value`-style defaults


## Phase 5: Route Pending Base / Alias / Member-Type Chains Through One Path

Objective:
Avoid separate ad hoc resolution code for:

- pending base types
- alias templates
- member typedef refs

Required slices:

- resolve owner template instantiation
- then resolve alias/member ref on top of the concrete owner
- if that produces another pending type, requeue it

Example chain:

1. instantiate `is_signed<T>`
2. realize base `is_signed_helper<T>::type`
3. realize `is_signed_helper<T, arithmetic<T>::value>`
4. realize base `bool_constant<(T(-1) < T(0))>`
5. inherit `value` and `operator()`

Exit condition:

- chained dependent template types resolve by repeated engine work, not nested
  parser tricks


## Phase 6: Shrink Early Resolution In Parser And AST-to-HIR

Objective:
After the engine path is solid, delete or narrow eager logic.

Candidate reductions:

- parser-side dependent NTTP default evaluation
- parser-side eager dependent member-type materialization
- AST-to-HIR eager fallback code that guesses concrete owners too early

Not everything must be removed.
Some eager paths are still useful for obviously concrete cases.

The rule is:

- eager is fine for already-concrete work
- dependent work should go through the shared lazy engine path


## Phase 7: Diagnostics And Convergence Rules

Objective:
Make unresolved type-driven template work fail cleanly.

Required slices:

- report unresolved pending template types after fixpoint exhaustion
- distinguish:
  - unresolved because owner never materialized
  - unresolved because member typedef missing
  - unresolved because dependent default could not be evaluated
  - unresolved because specialization selection found no match

- keep the same convergence semantics as template call/consteval reduction:
  - if a pass creates new concrete work, iterate again
  - stop when no new work appears


## Dataflow Sketch

### Before

```text
parser
  -> parses template type
  -> may eagerly evaluate dependent default
  -> may instantiate partial owner

ast_to_hir
  -> resolves some pending refs

compile_time_engine
  -> handles calls/consteval only
```

### After

```text
parser
  -> records unresolved type/default/member refs

ast_to_hir
  -> lowers them as pending template type work

compile_time_engine
  -> instantiate owner templates lazily
  -> evaluate dependent defaults with concrete bindings
  -> resolve member typedefs / aliases / bases
  -> repeat to convergence
```


## Benefits

If we do this well, we get:

- one authoritative place for dependent template type realization
- fewer parser-only hacks
- better correctness for trait-style code such as EASTL and libc++ patterns
- cleaner support for inherited `::value`, `::type`, and `operator()`
- easier debugging because work happens in one explicit fixpoint loop


## Risks

### 1. Too Much Work Moves At Once

If we move all template type resolution at once, we may destabilize a lot of
currently passing cases.

Mitigation:

- move one dependent category at a time
- keep eager concrete fast paths
- add targeted regression tests for each migrated pattern


### 2. Worklist Explosion

A naive type-instantiation loop can duplicate work or recurse forever.

Mitigation:

- specialization-key dedup
- explicit pending/realized sets
- convergence accounting


### 3. Parser/HIR Ownership Gets Blurry Again

It is easy to reintroduce ad hoc eager fixes in the parser.

Mitigation:

- enforce the stage boundary in helper APIs and comments
- treat parser-time dependent evaluation as exceptional and temporary


## Suggested First Slices

The first slices should be narrow and test-driven.

### Slice A

Add HIR-visible struct member typedef metadata and a lookup helper for
concrete owners.

Success means:

- `ConcreteOwner::type` can be resolved in HIR without parser-local maps


### Slice B

Add pending template type work items for `deferred_member_type_name`.

Success means:

- `Owner<T>::type` stays unresolved through parsing/lowering and gets realized
  only after `Owner<T>` is concrete


### Slice C

Move dependent NTTP default evaluation for template structs into the engine for
one narrow pattern:

- `trait<T>::value`

Success means:

- `template<typename T, bool = trait<T>::value>` is resolved by engine-driven
  type instantiation, not parser-time evaluation


### Slice D

Route inherited trait chains through the same lazy path.

Primary regression targets:

- `is_signed_helper<T>::type`
- inherited static `::value`
- inherited `operator()`


## Completion Criteria

This plan is complete when all of the following are true:

- parser no longer acts as the final evaluator for dependent NTTP defaults
- compile-time engine owns lazy realization of pending template types
- member typedef chains such as `Owner<T>::type` resolve after owner
  materialization
- inherited `::value` / `operator()` trait chains resolve through the same
  engine-driven flow
- targeted EASTL/libc++-style trait regressions pass without parser-specific
  hacks


## Immediate Next Step

The next coding task after agreeing on this plan should be:

1. add or finish HIR-owned metadata for struct member typedef/using aliases
2. expose pending member-type refs as engine work items
3. migrate one dependent default pattern from parser evaluation to engine
   evaluation
