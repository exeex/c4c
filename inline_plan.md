# HIR Inline Expansion Plan

## Goal

Implement semantic inlining as an explicit HIR-to-HIR transform that runs:

`AST -> HIR -> inline expansion pass -> LLVM emission`

Target semantics for this project:

- `inline` means: expand at call sites when the callee body is available and the call shape is supported
- `noinline` means: never expand
- default means: never expand

This is intentionally different from backend optimization inlining.

## Scope Boundary

### What this plan covers

- a frontend-owned HIR pass that performs body expansion deterministically
- preservation of source language intent for `inline` / `noinline`
- elimination of ordinary call/return ABI boundaries for expanded calls

### What this plan does not cover

- LLVM heuristic inlining
- code-size or hot/cold profitability decisions
- general-purpose whole-program call-graph optimization

## Architectural Decision

The compiler should treat inlining as two separate concepts:

1. Semantic inline expansion

- owned by the frontend/HIR pipeline
- driven only by source-level attributes or keywords
- no cost model

2. Backend optimization inline

- owned by LLVM or later codegen passes
- optional
- disabled by default for this feature

For this project, semantic inline expansion should happen after HIR construction and before [hir_emitter.cpp](/workspaces/c4c/src/codegen/llvm/hir_emitter.cpp).

## Pipeline Order With Templates And Consteval

Semantic inline expansion is not the first caller-driven transformation.

When template instantiation and `consteval` are also present, the intended ordering should be:

1. template instantiation
2. semantic/type resolution of the instantiated callee
3. `consteval` evaluation for immediate calls
4. HIR construction or HIR normalization
5. semantic inline expansion
6. backend lowering
7. optional backend optimization inlining

### Why template instantiation comes first

Template instantiation answers:

- which concrete callee is being called
- what its final parameter and return types are
- what function body will exist for later analysis

Inlining cannot safely clone a template pattern directly. It needs a concrete instantiated function body.

For an inline function template, the intended behavior is:

1. instantiate the template at the call site
2. produce a specialized function entity with a concrete body
3. carry forward its inline policy
4. let the semantic inline pass decide whether to expand that specialized callee

So an `inline` template function should be treated as:

- first: specialized into a concrete function
- later: expanded by the same inline pass used for non-template functions

### Why consteval comes before inline

`consteval` call handling is a semantic evaluation rule, not an optimization.

That means:

- immediate calls should be evaluated first
- successful evaluation should replace the call with a constant result
- only remaining runtime-relevant calls should reach inline expansion

This avoids mixing two very different mechanisms:

- `consteval`: execute at compile time and erase the call
- `inline`: clone the runtime body into the caller

### Practical interpretation

For a plain inline function:

- it reaches the semantic inline pass and may be expanded there

For an inline template function:

- instantiate first
- then inline the specialized function if policy says to do so

For a consteval template function:

- instantiate first
- then evaluate the immediate call
- normally no runtime inline step is needed afterward

For a consteval inline template function:

- instantiate first
- try consteval evaluation first
- only if a runtime-relevant form remains should semantic inline expansion even be considered

## Current State

The current codebase already has the right HIR shape for an inlining pass:

- stable `LocalId`, `BlockId`, and `ExprId` identities exist in [ir.hpp](/workspaces/c4c/src/frontend/hir/ir.hpp#L102)
- the HIR module already provides `alloc_local_id()`, `alloc_block_id()`, and `alloc_expr_id()` in [ir.hpp](/workspaces/c4c/src/frontend/hir/ir.hpp#L631)
- expressions, statements, and control-flow edges refer to each other by IDs rather than raw pointers

Current inline-related status:

- `inline` keyword is preserved into HIR linkage metadata in [ast_to_hir.cpp](/workspaces/c4c/src/frontend/hir/ast_to_hir.cpp#L989)
- `FnAttr.no_inline` and `FnAttr.always_inline` exist in HIR in [ir.hpp](/workspaces/c4c/src/frontend/hir/ir.hpp#L76)
- there is currently no HIR inline-expansion pass
- there is currently no LLVM emission path that enforces `alwaysinline` / `noinline`

## Design Principles

### 1. Inline by cloning IDs, not by renaming strings

Collision avoidance should be based on fresh IDs:

- old callee `LocalId` -> fresh caller `LocalId`
- old callee `BlockId` -> fresh caller `BlockId`
- old callee `ExprId` -> fresh caller `ExprId`

Debug names may be rewritten for readability, but correctness must not depend on names.

### 2. Expand CFG, not text

Inlining must splice cloned HIR blocks/statements/expressions into the caller CFG.

This is not an LLVM-text substitution step.

### 3. Evaluate arguments exactly once

Before expansion:

- each actual argument is evaluated once
- result is stored in a temp/local/expr form suitable for reuse
- parameter references inside the cloned callee body resolve to those captured values

### 4. Return becomes local control flow

Each `return` in the cloned callee body should be rewritten into:

- optional assignment to a synthetic return-result temp
- branch to the inline continuation block

### 5. Keep first implementation narrow

The first working slice should support only direct calls to known function definitions with simple, non-recursive bodies.

## Proposed Files

Suggested additions:

- `src/frontend/hir/inline_expand.hpp`
- `src/frontend/hir/inline_expand.cpp`

Suggested entry point:

- `phase2::hir::run_inline_expansion(Module&)`

Suggested integration point:

- run after HIR is fully built
- run before LLVM emission starts

## Pass Contract

### Input

- fully built `phase2::hir::Module`
- `fn_index` resolved
- function bodies, block graph, and expr pool stable

### Output

- caller functions rewritten so supported `inline` call sites are expanded in place
- unsupported `inline` call sites either:
  - remain as calls with a clear diagnostic strategy, or
  - are rejected during the phase if strict semantic enforcement is desired

### Invariants

- no ID collisions
- `expr_pool` remains internally consistent
- every referenced `ExprId` exists
- every referenced `BlockId` exists in the owning function
- control-flow successors remain valid after splice

## Phase Plan

## Phase 0: Attribute And Policy Plumbing

Goal:

- make `inline`, `noinline`, and default policy explicit and observable in HIR

Tasks:

- ensure parser/HIR lowering distinguishes:
  - source `inline`
  - source `__attribute__((noinline))`
  - source `__attribute__((always_inline))` if you want that alias
- define one canonical policy in HIR:
  - `InlinePolicy::Never`
  - `InlinePolicy::Always`
  - `InlinePolicy::DefaultNever`
- decide precedence when attributes conflict

Recommended rule:

- `noinline` wins over everything
- `inline` means semantic expansion request
- default stays non-inline

Deliverables:

- HIR carries enough metadata to decide expansion without reparsing tokens
- debug dump/printer shows inline policy

Tests:

- parser/HIR unit tests for:
  - plain function
  - `inline` function
  - `__attribute__((noinline))`
  - conflicting combinations

## Phase 1: Call-Site Discovery And Eligibility

Goal:

- identify which call sites are expandable

Tasks:

- scan each function body for `CallExpr`
- resolve callee for direct calls only
- determine whether the callee:
  - has a body
  - is marked inline-always under project semantics
  - is not marked noinline
- reject or defer unsupported cases

Initial supported cases:

- direct call by function name
- callee definition available in same HIR module
- non-variadic functions
- non-recursive calls
- ordinary return value or `void`

Initial deferred cases:

- indirect calls through function pointers
- recursive/self-recursive inline
- mutually recursive inline SCCs
- varargs
- computed goto interactions
- inline asm-heavy bodies if they complicate cloning

Deliverables:

- helper APIs like:
  - `const Function* resolve_direct_callee(const Function&, ExprId)`
  - `bool can_inline_callsite(const Module&, const Function&, const CallExpr&, Reason*)`

Tests:

- positive direct-call detection
- negative cases for indirect/variadic/missing-body calls

## Phase 2: ID Remapping Infrastructure

Goal:

- build the reusable cloning machinery

Tasks:

- define an `InlineCloneContext` containing:
  - `local_map`
  - `block_map`
  - `expr_map`
  - optional call-site serial/debug prefix
- add clone helpers for:
  - `Expr`
  - `Stmt`
  - `Block`
  - `DeclRef`
  - branch/loop/switch target references
- ensure cloned nodes allocate fresh IDs from `Module`

Suggested structure:

```cpp
struct InlineCloneContext {
  Module* module = nullptr;
  std::unordered_map<uint32_t, LocalId> local_map;
  std::unordered_map<uint32_t, BlockId> block_map;
  std::unordered_map<uint32_t, ExprId> expr_map;
  std::string debug_prefix;
};
```

Deliverables:

- generic remap/clone helpers usable independently of the final splice logic

Tests:

- cloning a block tree produces all-new IDs
- cloned expr references point only to cloned expr IDs
- cloned branch targets point only to cloned block IDs

## Phase 3: Argument Capture And Parameter Binding

Goal:

- preserve call semantics while preparing for body splice

Tasks:

- for each inline call site, evaluate every actual argument exactly once
- store argument values in synthetic caller-side temporaries or locals
- map each callee parameter to the captured value representation

Recommended initial strategy:

- synthesize one `LocalDecl` per parameter capture
- emit initialization in the inline prelude at the original call location
- rewrite `DeclRef.param_index` uses inside the cloned body into those synthetic locals

Why this is a good first step:

- easy to reason about
- avoids duplicated side effects
- avoids needing a special “parameter alias” expr kind immediately

Deliverables:

- parameter references inside cloned body no longer depend on original callee parameter numbering

Tests:

- side-effecting arguments evaluated once
- multiple parameter references read same captured value
- argument order preserved

## Phase 4: Minimal CFG Splice For Single-Return Bodies

Goal:

- get the first end-to-end inline expansion working

Tasks:

- split the caller block around the original call site into:
  - pre-call block
  - inlined callee region
  - continuation block
- clone callee blocks into caller
- redirect entry to cloned callee entry
- replace single `return expr` with:
  - store into synthetic result local if non-void
  - branch to continuation block
- replace the original call expression use with a read from the synthetic result local

Initial restriction:

- support only callees with exactly one return site
- support only call sites used as standalone expr stmt or simple initializer if that reduces first-slice complexity

Deliverables:

- first working inliner on a narrow subset

Tests:

- `int add1(int x) { return x + 1; }`
- void callee
- inline call nested in a simple expression context if supported

## Phase 5: General Return Rewriting And Multi-Block Bodies

Goal:

- support real-world structured functions

Tasks:

- rewrite all callee `ReturnStmt` nodes to target one shared continuation block
- support multiple return sites
- support conditionals and loops in inlined bodies
- remap break/continue/switch targets within cloned region

Key requirement:

- cloned internal control flow must stay internal
- only rewritten returns may exit to the caller continuation

Deliverables:

- full structured-control-flow inline support for ordinary HIR bodies

Tests:

- `if/else` with distinct returns
- loop with early return
- switch with return in cases

## Phase 6: Call Replacement In Expression Contexts

Goal:

- inline calls that appear inside larger expressions

Tasks:

- define how an inlined call produces a value in expression position
- normalize call sites into statement form when necessary
- introduce temporary materialization for expression-result consumption

Recommended implementation path:

- add a normalization step before inlining that hoists complex call-containing expressions into:
  - temp decl
  - expr stmt
  - later use of temp via `DeclRef`

This can dramatically simplify the inliner.

Deliverables:

- inlining works not only for standalone call statements but also for:
  - assignments
  - local initializers
  - binary-expression operands after normalization

Tests:

- `int y = inl(x) + 3;`
- `return inl(a) * inl(b);`

## Phase 7: Diagnostics And Unsupported Cases

Goal:

- make semantic behavior explicit when a requested inline cannot be performed

Decision to make:

- strict mode:
  - source `inline` requires successful expansion
  - unsupported cases are errors
- permissive mode:
  - source `inline` is a request
  - unsupported cases fall back to normal call, possibly with warning

Recommended for this project:

- start permissive during bring-up
- move to strict once supported surface is broad enough

Tasks:

- produce clear reasons for refusal:
  - recursive inline unsupported
  - indirect callee
  - variadic callee
  - unsupported control-flow form
- ensure `noinline` always suppresses expansion

Deliverables:

- stable diagnostics and testable refusal behavior

Tests:

- negative tests per unsupported category

## Phase 8: Backend Coordination

Goal:

- keep LLVM from silently redefining project semantics

Tasks:

- default backend pipeline should not introduce independent heuristic inlining for this feature
- optionally emit LLVM `noinline` for non-expanded calls if you want extra backend protection
- optionally emit LLVM `alwaysinline` only when it matches an intentional policy and a later LLVM pipeline is expected to honor it

Recommended default:

- semantic expansion happens in HIR
- backend should not be relied on for correctness

Deliverables:

- documented contract between HIR inline pass and LLVM emission

## Phase 9: Expansion Coverage

Goal:

- broaden support after the core pass is stable

Possible follow-up support:

- inline through function aliases if represented canonically
- selected function-pointer calls with proven direct target
- variadic subset
- better debug-name decoration for inlined locals
- recursion guards and max-inline-depth limits

Non-goals unless required:

- heuristic size thresholds
- PGO-guided inlining

## Recommended Implementation Order

Suggested execution order:

1. Phase 0
2. Phase 1
3. Phase 2
4. Phase 3
5. Phase 4
6. Phase 5
7. Phase 6
8. Phase 7
9. Phase 8

This reaches a useful end-to-end system before broadening expression coverage.

## Testing Strategy

### Unit-level checks

- clone/remap helper correctness
- no reused IDs after expansion
- valid block successor references
- valid expr references in `expr_pool`

### Integration tests

Add internal positive cases for:

- simple inline return
- inline with two parameters
- inline with local temporaries
- inline with branching
- inline in initializer/expression position

Add internal negative or deferred cases for:

- noinline function stays a call
- plain unannotated function stays a call
- recursive inline
- indirect call
- variadic inline

### Output validation

For expanded calls, emitted LLVM IR should show:

- no call instruction at the targeted site
- caller-local allocas/temps only as needed
- no ABI lowering artifacts attributable to a separate callee invocation

## Risks

### 1. Expression-context complexity

Inlining directly inside nested expressions can become much harder than statement-position inlining.

Mitigation:

- normalize call expressions before full expansion

### 2. CFG corruption

Incorrect block remapping can silently create invalid fallthrough or broken successor edges.

Mitigation:

- add post-pass verifier for block and expr references

### 3. Recursive expansion blow-up

Even without heuristics, semantic inline can recurse infinitely or explode code size.

Mitigation:

- forbid recursive expansion initially
- add inline-depth and visited-callstack guards

### 4. Parameter aliasing and side effects

Naive substitution can duplicate evaluation or change sequencing.

Mitigation:

- capture arguments once before cloning

## Definition Of Done

The semantic inliner is complete when:

- HIR has an explicit inline policy
- supported `inline` calls are expanded before LLVM emission
- `noinline` calls are never expanded
- default calls remain ordinary calls
- argument evaluation order is preserved
- return-value and control-flow rewriting are correct
- emitted LLVM IR no longer depends on backend inlining for source-level inline semantics
