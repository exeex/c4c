# Lazy Template Type Instantiation Plan

## Goal

Move dependent template type realization into the compile-time engine and make
it use-site driven.

The intended rule is simple:

- parser preserves unresolved structure
- AST to HIR records pending work
- compile-time engine is the only stage that turns dependent work into
  concrete types/values

This plan is about:

- template struct/class instantiation from type use sites
- dependent NTTP defaults such as `bool = trait<T>::value`
- dependent member typedefs such as `Owner<T>::type`
- dependent base / alias / member chains
- interactions between the above and `consteval` / template function
  instantiation

This plan is not about:

- full C++ template semantics
- SFINAE, concepts, or full partial ordering
- removing every eager fast path
- fully replacing string-carried template identity


## Relationship To Structured Template Identity

This plan assumes we can continue using the current parser/HIR transport shape
for unresolved template types in the short term, for example:

- `TypeSpec.tpl_struct_origin`
- `TypeSpec.tpl_struct_arg_refs`
- parser-generated specialization / instantiation names

That is acceptable only as a transition mechanism.

But we now know there is a deeper issue:

> string names are currently carrying too much semantic meaning

Today the system can accidentally use strings to represent all of these at
once:

- primary template family identity
- specialization pattern identity
- concrete instantiation identity
- debug / printable names

That is unsafe.
In particular, names like `foo__spec_1` or `foo_T_int` should not become the
semantic key for later template resolution.

So the long-term direction is:

- use-site lazy instantiation remains the control-flow model
- structured template identity becomes the semantic model

Those are related but distinct refactors.

This plan therefore focuses on:

- when work is created
- who owns the fixpoint
- how blocked work decomposes into finer-grained work

And leaves the full "stop using strings as semantic identity" work to a
separate plan:

- [structured_template_identity_plan.md](/workspaces/c4c/ideas/structured_template_identity_plan.md)


## Core Understanding

The important shift is:

> template type work should be created at the use site, not at the template
> definition site

That means we do **not** want:

- "I saw `template <...> struct Foo`, so instantiate things now"
- "parser saw a dependent default, so guess now"

We **do** want:

- "a declaration / cast / base / member lookup needs a concrete type now"
- "record that requirement as compile-time work"
- "let the engine resolve it when enough dependencies are available"


## Why The Old Split Fails

Right now dependent type realization is spread across:

1. parser
2. `ast_to_hir.cpp`
3. `compile_time_engine.cpp`

The parser and `ast_to_hir.cpp` still do too much final work:

- evaluate deferred NTTP defaults
- choose template struct specializations
- instantiate concrete struct defs
- chase `Owner<T>::type`
- resolve dependent bases inline

That causes predictable problems:

- work happens before bindings are concrete
- nested dependencies are solved by recursion and ad hoc fallbacks
- there is no single place that knows whether a failure is terminal or merely
  blocked on more compile-time work


## Work Kinds The Engine Must Own

The compile-time engine should eventually own three kinds of work:

1. `consteval` evaluation
- value-driven work
- already present today

2. template function instantiation
- call-driven work
- already present today

3. template struct/class and dependent type realization
- type-use-driven work
- this is the missing piece

These are not isolated lanes.
They unlock each other.

Examples:

- a `consteval` evaluation may require template function instantiation
- a `consteval` evaluation may require `Trait<T>::value`, which requires
  template struct/class realization
- a template function instantiation may reveal pending local/return/base types
- a template struct/class instantiation may require deferred NTTP evaluation,
  base realization, member typedef lookup, or more compile-time work

So the engine should be treated as one fixpoint over mixed compile-time work,
not a set of unrelated passes.


## The Target Execution Model

### 1. Parser

Parser responsibilities:

- parse syntax
- preserve unresolved type structure
- preserve deferred NTTP default text/tokens
- preserve deferred member-type references

Parser should not be the final authority for dependent answers.


### 2. AST To HIR

AST to HIR responsibilities:

- lower runtime/codegen-facing HIR
- preserve unresolved `TypeSpec` state such as:
  - `tpl_struct_origin`
  - `tpl_struct_arg_refs`
  - `deferred_member_type_name`
- enqueue pending template type work when a use site requires a type

AST to HIR may still do eager work for obviously concrete cases.
But when work is dependent, its job is to record it, not finish it.

Important caveat:

- these string fields are transport/state-preservation fields, not the desired
  long-term semantic identity model
- engine-internal resolution should move toward structured identity over time


### 3. Compile-Time Engine

Compile-time engine responsibilities:

- consume pending template function work
- consume pending consteval work
- consume pending template type work
- repeatedly resolve what is ready
- enqueue finer-grained prerequisite work when an item is blocked
- stop only at fixpoint

This is the key behavioral rule:

> a failed resolution attempt is usually not an error; it is dependency
> discovery


## Template Type Work Should Be Use-Site Driven

Pending template type work should be created when we actually need a concrete
type, for example:

- function return type lowering
- function parameter type lowering
- method return / parameter type lowering
- local declaration type lowering
- cast target lowering
- struct base realization
- member typedef lookup

Not when we merely see:

- a template struct/class definition
- a parser-level dependent token sequence

In short:

- definition site preserves
- use site enqueues
- engine resolves


## Work Item Model

The engine needs first-class type work items.

Conceptually:

```cpp
enum class PendingTemplateTypeKind {
  DeclarationType,
  OwnerStruct,
  MemberTypedef,
  BaseType,
  CastTarget,
};

struct PendingTemplateTypeWorkItem {
  PendingTemplateTypeKind kind;
  TypeSpec pending_type;
  TypeBindings type_bindings;
  NttpBindings nttp_bindings;
  SourceSpan span;
  std::string context_name;
};
```

Important properties:

- it stores the unresolved `TypeSpec`
- it stores the concrete bindings known at the use site
- it carries context for dedup and diagnostics

Current limitation:

- as long as this work item ultimately depends on string-carried owner names,
  it is still vulnerable to confusion between primary templates,
  specialization patterns, and concrete instantiations
- that is why structured identity is a parallel architectural requirement


## Resolution States

Trying to resolve a work item should not return just `success` or `failure`.

We need at least three states:

1. `resolved`
- concrete answer is available

2. `blocked`
- not enough information yet
- more prerequisite work should be enqueued

3. `terminal`
- there is no valid continuation
- only report this as a user-facing unresolved error after fixpoint exhaustion

This distinction is crucial.

Examples of `blocked`:

- owner struct not instantiated yet
- dependent NTTP default not evaluated yet
- member typedef lookup depends on a concrete base that does not exist yet
- consteval value needed by specialization selection is not available yet

Concrete pending example:

- `cpp_positive_sema_template_nttp_default_runtime_cpp`
- shape:
  `template <typename T, bool = is_void<T>::value || is_void<T>::value>`
- current behavior:
  parser-side `eval_deferred_nttp_default()` still tries to decide this too
  early and can select the wrong default specialization
- intended behavior:
  parser preserves the deferred expression/tokens, use-site lowering records
  pending template type/value work, and the compile-time engine evaluates the
  NTTP default only when the concrete bindings for `T` are available

Examples of `terminal`:

- owner template not found
- member typedef does not exist
- no specialization matches
- deferred expression is not evaluable in this model


## Dependency Discovery Rule

When a template type work item cannot be resolved immediately, the engine
should usually decompose it into smaller work.

Examples:

### `Owner<T>::type`

1. try to resolve member typedef
2. discover owner is not concrete
3. enqueue owner-struct work for `Owner<T>`
4. keep the member-typedef work pending
5. retry next iteration


### `Foo<T, trait<T>::value>`

1. try to instantiate `Foo`
2. discover NTTP default/value is not concrete yet
3. enqueue finer-grained work to realize that value
4. keep the owner-struct work pending
5. retry next iteration


### Dependent Base Chain

1. try to realize base type
2. discover base owner/member chain is unresolved
3. enqueue owner/member prerequisite work
4. keep the base work pending
5. retry next iteration


## The Fixpoint Rule

Each engine iteration should:

1. try ready consteval work
2. try ready template function work
3. try ready template type work
4. enqueue any newly discovered prerequisites
5. repeat while progress is still being made

Progress means any of:

- a pending item becomes resolved
- a blocked item produces new finer-grained work
- a previously pending consteval / function / type dependency becomes concrete

Only when no work resolves and no new work is discovered do we stop.


## The First Real Migration Boundary

The first real migration is not "fully lazy templates".

It is:

- use sites enqueue template type work
- compile-time engine drives attempts to resolve it
- `ast_to_hir.cpp` stops being the control loop

At first the engine may still call back into existing helpers in
`ast_to_hir.cpp`.
That is acceptable as a transition step.

The key thing to move first is control ownership, not every helper body.

But we should be explicit about one thing:

- moving control ownership into the engine does **not** by itself solve
  semantic identity bugs caused by stringly-typed template ownership

So a minimal "engine owns the loop" implementation may still need temporary
compatibility logic until structured identity exists.


## What Needs To Move Out Of `ast_to_hir.cpp`

Eventually the following logic should no longer be engine-external:

- pending template struct realization
- deferred NTTP default evaluation for struct/class templates
- specialization selection for template struct/class instantiation
- dependent member typedef lookup
- dependent base-type follow-up resolution

Today these mostly live inside:

- `resolve_pending_tpl_struct()`
- `resolve_struct_member_typedef_hir()`
- parser-side `eval_deferred_nttp_default()`

Those helpers currently combine:

- dependency discovery
- final realization
- recursive follow-up work

That is exactly what should become explicit engine worklist logic.


## Recommended Refactor Order

### Step 1. Preserve And Enqueue

- keep parser preservation as-is
- enqueue pending template type work at use sites
- keep eager fallback behavior temporarily

This gives visibility without risking a full semantic move.


### Step 2. Move Engine Control Ownership

- compile-time engine actively consumes pending template type work
- engine tracks resolved vs blocked items
- engine becomes the fixpoint driver for type work

This is the first important architectural shift.


### Step 3. Split `resolve_pending_tpl_struct()`

Break the current monolith into engine-callable sub-steps:

1. substitute deferred template arg refs
2. materialize explicit/default args
3. evaluate dependent NTTP defaults
4. select primary/specialization pattern
5. build concrete mangled name
6. instantiate concrete struct/class metadata
7. realize follow-up base/member work

At this stage code motion is fine.
Semantic change should stay minimal.


### Step 4. Teach The Engine About `blocked`

Replace simple bool success/failure with structured results:

- resolved
- blocked
- terminal

And allow blocked attempts to enqueue finer-grained work items.


### Step 5. Remove Recursive Ad Hoc Resolution

Once blocked/requeue behavior works, shrink eager recursive logic in:

- `resolve_pending_tpl_struct()`
- `resolve_struct_member_typedef_hir()`
- expression-lowering opportunistic resolution

The goal is:

- engine owns the loop
- helpers do local resolution work only


## What The First Good Diff Looks Like

A good first implementation diff should:

- add pending template type work items
- seed them from use sites
- let the engine attempt to consume them
- keep current eager fallback logic behind callbacks or compatibility paths
- expose stats / diagnostics so we can see what remains pending

A good second diff should:

- split struct realization into smaller engine-callable sub-helpers
- start distinguishing blocked from terminal failures


## Diagnostics Policy

Do not emit hard unresolved diagnostics at first blocked attempt.

Instead:

- if resolution is blocked, record/queue dependencies and retry later
- if fixpoint ends with no progress, then report what remains unresolved

Diagnostics should say which category failed:

- owner struct/class never materialized
- member typedef missing
- dependent NTTP default never became concrete
- specialization selection failed
- prerequisite consteval work never reduced


## Success Criteria

We are on the right track when:

- use-site dependent types are enqueued rather than guessed early
- the engine, not `ast_to_hir.cpp`, drives retries
- blocked attempts generate finer-grained work
- primary template identity is not silently replaced by specialization /
  instantiation print names during later resolution
- nested cases such as "consteval inside template type" or
  "template type inside consteval" converge through the same fixpoint
- unresolved diagnostics happen only after fixpoint exhaustion


## Short Version

The new model is:

- definition site preserves
- use site enqueues
- engine resolves
- blocked work expands into finer-grained work
- only fixpoint exhaustion turns pending work into an error
