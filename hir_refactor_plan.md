# HIR Refactor Plan

## Goal

Refactor HIR so template functions and template function calls survive lowering as first-class compile-time entities, instead of being eagerly instantiated or erased in frontend sema.

This is required for:

- consteval-dependent template arguments
- delayed specialization
- future JIT specialization
- keeping compile-time materialization as a policy decision

## Core Direction

HIR should preserve both:

- template function definitions
- template function application requests

before final specialization/materialization.

The intended mental model is decorator-shaped:

- `template<typename T> T add(T, T);`
- interpret as:
  - `add` = compile-time decorator
  - `add<T>` = specialized callable value produced by decorator application

Informally:

```text
consteval add(typename T) -> (add<T>(T, T) -> T)
```

This is not final surface syntax. It is a semantic model for HIR design.

## Immediate HIR Requirement

Before a full template system exists, HIR should at minimum be able to preserve a stable format for:

1. template function definitions

- function body remains attached to template parameters
- the definition is not immediately lowered into only concrete functions

2. template function calls / applications

- `add<int>(1, 2)` must remain distinguishable from:
  - plain function call `add(1, 2)`
  - already-materialized specialized symbol

3. compile-time-only callable decorators

- decorator application should be representable without immediately forcing codegen

## Why HIR Must Own Instantiation

Template instantiation cannot be rigidly ordered ahead of consteval forever.

Counterexample:

- a template argument may itself depend on a consteval result

So the long-term compile-time reduction pipeline should live in HIR and run to a fixpoint:

1. template instantiate
2. consteval
3. template instantiate
4. consteval
5. repeat until no compile-time-only nodes are reducible

This lets HIR resolve chains such as:

- consteval produces template args
- instantiated templates expose new consteval calls
- those consteval calls unlock more template instantiations

## Role Of Sema

Sema can still do a small amount of early compile-time work, but only conservatively.

Allowed in sema:

- local constant folding
- simple non-template consteval folding
- legality diagnostics
- shape preservation for template/application nodes

Not appropriate as final sema responsibilities:

- full template instantiation
- final template materialization policy
- erasing template application structure needed by HIR/JIT

## Suggested HIR Representation Strategy

### Option A: explicit new HIR nodes

Target end state:

- `TemplateDef`
- `TemplateApply`
- `TemplateMaterialize`

Pros:

- clean semantics
- easiest to reason about for JIT/link-time reuse

Cons:

- bigger refactor

### Option B: transitional decorator-shaped reuse of existing callable nodes

Short-term practical path:

- represent template definition as a compile-time decorator-like callable
- represent `add<T>` as decorator application producing a specialized callable
- reuse existing call/function infrastructure where possible

Pros:

- smaller first step
- may reuse existing node plumbing

Cons:

- semantics can become blurry if transitional encoding is not documented clearly

## Recommended Sequence

### Phase 1: preserve template structure in HIR

- lower template function definitions as HIR entities
- lower template applications/calls as explicit compile-time application forms
- stop eagerly erasing template structure during lowering

### Phase 2: preserve consteval as HIR-reducible nodes

- represent consteval calls/results in a form usable by an iterative reduction pass
- keep failures and diagnostics structured

### Phase 3: add HIR compile-time reduction loop

- worklist or fixpoint iteration
- repeatedly run:
  - template instantiation
  - consteval reduction
- stop at convergence

### Phase 4: define materialization boundary

- decide when a specialized callable becomes a concrete emitted function
- keep this separate from template semantics themselves

### Phase 5: specialization identity

- stable specialization key
- cross-TU determinism
- future caching / JIT reuse

## Exit Criteria

- HIR can represent template functions without immediate concrete instantiation
- HIR can represent template function application distinctly from ordinary calls
- consteval and template instantiation can interact through iterative reduction
- compile-time reduction converges without requiring template erasure in sema
- the design still permits later link-time or JIT specialization
