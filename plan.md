# C++ Consteval Plan

## Goal

Build a real C++ subset constant-evaluation pipeline instead of treating `consteval` as a parser-only annotation.

Long-term target:

- `consteval` functions are modeled as immediate functions
- calls to immediate functions are evaluated during semantic analysis or an explicit constant-evaluation stage
- named constant objects can participate in constant expressions
- constant-evaluation results can be reused by:
  - `constexpr` / `consteval` function calls
  - `if constexpr`
  - global `constexpr` initializers
  - future template/type-trait queries

Immediate target:

- upgrade the current C-style integer constant-expression helper into a reusable constant-evaluation subsystem for the C++ subset

## Current State

### 1. Parser support already exists

The parser already accepts and preserves:

- `constexpr`
- `consteval`
- `if constexpr`

AST nodes already carry:

- `is_constexpr`
- `is_consteval`

Relevant files:

- [src/frontend/parser/ast.hpp](/workspaces/c4c/src/frontend/parser/ast.hpp)
- [src/frontend/parser/declarations.cpp](/workspaces/c4c/src/frontend/parser/declarations.cpp)
- [src/frontend/parser/statements.cpp](/workspaces/c4c/src/frontend/parser/statements.cpp)
- [src/frontend/parser/types.cpp](/workspaces/c4c/src/frontend/parser/types.cpp)

### 2. `src/frontend/sema/consteval.cpp` is currently a C-style constant-expression helper

Today this file does **not** implement C++ `consteval` function semantics.

What it currently does:

- decodes string literal bytes
- evaluates simple integer constant expressions over AST nodes
- supports:
  - integer / char literals
  - unary operators
  - binary operators
  - ternary expressions
  - integer cast folding
- resolves named constants for:
  - enum constants
  - selected named constant globals when the caller provides a binding map

What it does **not** do yet:

- evaluate local named `const` / `constexpr` variables
- interpret `consteval` function bodies
- maintain an evaluation environment for locals/parameters
- diagnose invalid non-constant immediate calls
- prevent taking the address of a `consteval` function

Relevant files:

- [src/frontend/sema/consteval.cpp](/workspaces/c4c/src/frontend/sema/consteval.cpp)
- [src/frontend/sema/validate.cpp](/workspaces/c4c/src/frontend/sema/validate.cpp)
- [src/frontend/hir/ast_to_hir.cpp](/workspaces/c4c/src/frontend/hir/ast_to_hir.cpp)

### 3. C++ subset semantic layer is still mostly a pass-through

Current C++ extension hook is effectively empty:

- [src/frontend/sema/sema.cpp](/workspaces/c4c/src/frontend/sema/sema.cpp)

This means:

- `consteval` is parsed and printed
- but there is not yet a real immediate-function semantic pass

### 4. Current behavior is still closer to “ordinary function with a marker”

Current positive cases such as:

- [consteval_func.cpp](/workspaces/c4c/tests/internal/cpp/postive_case/consteval_func.cpp)
- [consteval_template.cpp](/workspaces/c4c/tests/internal/cpp/postive_case/consteval_template.cpp)

compile and run because the frontend still lowers them like ordinary functions.

That is useful for parser bring-up, but it is not correct final `consteval` behavior.

### 5. Phase 1 now has a working first slice

Implemented so far:

- named constant global folding for `const` / `constexpr` scalar globals during global-init lowering
- global binding propagation so later constant globals can reference earlier ones
- integer cast folding in the AST constant evaluator for cases like:
  - `(unsigned int)-4`
  - `(unsigned short)-4`
  - `(unsigned char)-4`

Current validated examples:

- [constexpr_var.cpp](/workspaces/c4c/tests/internal/cpp/postive_case/constexpr_var.cpp)
  now folds to `@answer = constant i32 42`
- [const_named_fold.cpp](/workspaces/c4c/tests/internal/cpp/postive_case/const_named_fold.cpp)
  now folds `z = x + y` to `9`
- `tests/external/gcc_torture/src/pr39240.c`
  now passes again after restoring correct integer-cast constant folding

Important constraint discovered during implementation:

- named-global pre-folding must not be applied to every global initializer indiscriminately
- it is currently gated to `const` / `constexpr` global initializers
- ordinary C global initializers still rely on the existing HIR/codegen constant path

This keeps:

- C++ named-const folding working
- C integer-cast constant initializers working
- regressions like `pr39240.c` avoided

## Architectural Decision

We should separate three concepts that are currently blurred together:

1. constant expression evaluation

- expression interpreter for AST/HIR values

2. `constexpr`

- declaration/property saying something may participate in constant evaluation

3. `consteval`

- immediate-function policy saying a call must be evaluated in an immediate context

This matters because:

- not every constant expression comes from a `consteval` function
- not every `constexpr` function call must be folded immediately
- `consteval` is a semantic rule on calls, not just a flag on declarations

## Problems To Solve

### A. Named constant propagation is partially complete

Examples that should eventually fold:

```cpp
const int x = 4;
const int y = 5;
const int z = x + y;
```

Status now:

- global named constant objects like `x` and `y` can fold when they are scalar `const` / `constexpr` globals lowered in declaration order

Still missing:

- local named constant bindings
- richer non-scalar constant objects
- a single shared environment used consistently across sema, HIR, and immediate calls

### B. Immediate-function calls are not modeled

Examples that should become semantic rules:

- a `consteval` call must be evaluated immediately
- failure to evaluate should be a diagnostic
- taking the address of a `consteval` function should be rejected
- non-immediate contexts should not silently lower to runtime calls

### C. `if constexpr` should consume the same constant-evaluation engine

Right now `if constexpr` and ternary constant folding are partly handled ad hoc.

We want one common source of truth for:

- branch condition folding
- named constant lookup
- builtin constant queries
- immediate call results

### D. Global `constexpr` initialization needs end-to-end folding

The plan should cover:

- semantic evaluation of constant initializer expressions
- conversion into global initializer form
- stable behavior for `constexpr_var.cpp`

Status now:

- `constexpr_var.cpp` is fixed for the current scalar integer subset

Still missing:

- broader initializer forms
- one unified evaluator instead of the current targeted bridge in HIR lowering

## Recommended Design

## 1. Introduce an explicit constant-evaluation domain

Create a small value model, separate from raw AST nodes.

Suggested types:

- `ConstValue`
- `ConstEvalResult`
- `ConstEvalContext`
- `ConstEvalEnv`

Suggested `ConstValue` cases for the current subset:

- integer
- boolean
- character
- null pointer
- string literal reference
- invalid / unknown

Keep the first version intentionally narrow.

Do not try to support:

- heap objects
- dynamic type / RTTI
- virtual dispatch
- arbitrary class object interpretation

That matches the current subset boundaries.

## 2. Unify constant evaluation behind a single API

Instead of multiple loosely related helpers, expose a single semantic API:

```text
evaluate_constant_expr(node, context) -> ConstEvalResult
```

And thinner wrappers on top:

- `evaluate_ice(...)`
- `evaluate_immediate_call(...)`
- `evaluate_global_initializer(...)`

This avoids duplicating logic between:

- `validate.cpp`
- `consteval.cpp`
- `ast_to_hir.cpp`
- future C++ semantic passes

## 3. Add an environment for named constant objects

To support:

```cpp
const int x = 4;
const int y = 5;
const int z = x + y;
```

the evaluator needs bindings for names.

Recommended environment sources:

- enum constants
- global `const` / `constexpr` objects with constant initializers
- local `const` / `constexpr` objects inside an active constant-evaluation frame
- parameters bound during `consteval` / `constexpr` call interpretation

The environment should be lexical and immutable per frame.

## 4. Treat `consteval` calls as semantic evaluation requests

When the analyzer sees a call to a function marked `is_consteval`:

- resolve the target function
- create an evaluation frame
- bind arguments as `ConstValue`
- interpret the body
- produce a constant result or a structured failure

If evaluation fails:

- emit a semantic error
- do not silently fall back to runtime lowering

This is the core change that turns `consteval` from an annotation into real behavior.

## 5. Keep runtime codegen as a later policy decision

For the subset, we may still choose to materialize the body as a normal function temporarily for debugging or transition purposes.

But semantically:

- the call result should come from constant evaluation
- runtime lowering should be optional implementation scaffolding, not the meaning of `consteval`

## Implementation Phases

### Phase 1: strengthen the existing constant-expression helper

Goal:

- make the current helper useful for named constants and C++ subset folding without yet interpreting function bodies

Status:

- partially completed

Tasks:

1. Refactor `src/frontend/sema/consteval.cpp`

- replace the current narrow helper interface with `ConstValue` / `ConstEvalResult`
- preserve existing integer operator support

Status:

- not started in full
- current code still uses `std::optional<long long>`

2. Extend name lookup beyond enums

- teach evaluator to resolve references to constant globals
- support simple local constant bindings

Status:

- global constant bindings: done for the current scalar path
- local constant bindings: not started

3. Replace duplicated integer-folding entry points where practical

- migrate users in [src/frontend/sema/validate.cpp](/workspaces/c4c/src/frontend/sema/validate.cpp)
- migrate users in [src/frontend/hir/ast_to_hir.cpp](/workspaces/c4c/src/frontend/hir/ast_to_hir.cpp)

Status:

- partial use added in [ast_to_hir.cpp](/workspaces/c4c/src/frontend/hir/ast_to_hir.cpp) for gated named-global pre-folding
- `validate.cpp` still uses its own evaluator

4. Fix `constexpr_var.cpp`

- make `base`, `offset`, and `answer` fold through named bindings

Status:

- done

Exit criteria:

- `const int x = 4; const int y = 5; const int z = x + y;` folds correctly
- `tests/internal/cpp/postive_case/constexpr_var.cpp` can move from frontend-only to runtime mode

Current result:

- achieved for the current scalar integer subset
- [const_named_fold.cpp](/workspaces/c4c/tests/internal/cpp/postive_case/const_named_fold.cpp) added as regression coverage
- [constexpr_var.cpp](/workspaces/c4c/tests/internal/cpp/postive_case/constexpr_var.cpp) moved to runtime coverage

Remaining Phase 1 work:

- unify duplicated constant-expression code paths
- add local binding support
- stop relying on HIR-lowering-specific glue for named global constants

### Phase 2: add immediate-function interpretation

Goal:

- evaluate simple `consteval` functions instead of lowering them as ordinary runtime calls

Tasks:

1. Build a minimal function-body interpreter

Support only the subset we already care about:

- parameter references
- local constant declarations
- arithmetic expressions
- `return`
- simple `if` / `if constexpr`
- calls to other constant-evaluable functions

2. Add function-evaluation frames

- map parameters to values
- maintain local constant bindings
- track return state

3. Define failure modes

Examples:

- use of unsupported expression kind
- read of non-constant object
- missing return value
- call to non-constant function from immediate context

4. Hook call resolution

- when lowering or validating a call to a `consteval` function, evaluate it first
- replace the call with a constant result when successful

Exit criteria:

- `consteval_func.cpp` remains passing without relying on runtime semantics
- simple chained immediate calls can be folded

### Phase 3: enforce C++ subset `consteval` rules

Goal:

- stop accepting incorrect runtime behavior for immediate functions

Tasks:

1. Diagnose non-immediate calls

- a `consteval` call that cannot be evaluated as a constant expression should be rejected

2. Diagnose taking function address

- reject `&f` when `f` is `consteval`

3. Forbid runtime fallback

- once semantic evaluation is required, codegen should not emit ordinary runtime call sites for failed immediate invocations

4. Tighten declaration rules

- reject invalid combinations as needed by the supported subset

Exit criteria:

- current success cases still pass
- invalid `consteval` use becomes a semantic error instead of silently lowering

### Phase 4: integrate with `if constexpr`, builtins, and templates

Goal:

- make the same evaluator power the rest of the C++ subset work

Tasks:

1. Route `if constexpr` condition folding through the unified evaluator

2. Make builtin constant queries evaluator-aware

Examples:

- type-trait style builtins
- `sizeof`
- already-foldable parser/sema builtins

3. Allow template-substituted constant evaluation

- after template args are substituted, consteval/constexpr bodies should evaluate over the specialized environment

Exit criteria:

- `if constexpr` branch selection does not rely on ad hoc logic
- template + consteval positive cases fold through the same engine

## File-Level Work Plan

### `src/frontend/sema/consteval.cpp` and `.hpp`

Primary changes:

- define constant-evaluation value/result/context types
- implement expression interpreter
- implement function-body evaluation entry point

### `src/frontend/sema/validate.cpp`

Primary changes:

- stop using bespoke integer-only evaluation where richer constant-evaluation results are needed
- use the unified evaluator for constant-expression checks

### `src/frontend/sema/sema.cpp`

Primary changes:

- add a real C++ subset semantic pass
- trigger immediate-function diagnostics and replacement/folding

### `src/frontend/hir/ast_to_hir.cpp`

Primary changes:

- consume already-folded constants where available
- avoid encoding runtime calls for semantically immediate invocations

### `src/frontend/sema/canonical_symbol.cpp`

Primary changes:

- stop overloading template pretty-printing with `consteval` wording
- keep `consteval` semantics distinct from template-decoration semantics

## Validation Plan

### Positive cases to keep/add

Existing useful cases:

- [consteval_func.cpp](/workspaces/c4c/tests/internal/cpp/postive_case/consteval_func.cpp)
- [consteval_template.cpp](/workspaces/c4c/tests/internal/cpp/postive_case/consteval_template.cpp)
- [constexpr_var.cpp](/workspaces/c4c/tests/internal/cpp/postive_case/constexpr_var.cpp)
- [constexpr_if.cpp](/workspaces/c4c/tests/internal/cpp/postive_case/constexpr_if.cpp)

Recommended new positive cases:

- named constant propagation:
  - `const int x = 4; const int y = 5; const int z = x + y;`
- chained immediate calls
- `consteval` using local constant bindings
- `if constexpr` driven by immediate-call result

### Negative cases to add later

- `consteval` call with non-constant argument
- taking address of `consteval` function
- immediate body reading non-constant local
- immediate call to unsupported runtime-only function

## Suggested Execution Order

1. Refactor the existing constant-expression helper into a reusable evaluator.
2. Expand the current global named-constant support into a shared lexical environment model.
3. Add immediate-function interpretation for the current arithmetic subset.
4. Enforce `consteval` call rules and remove silent runtime fallback.
5. Reuse the same evaluator for `if constexpr`, builtin traits, and template-specialized evaluation.

## Non-Goals For This Plan

The following are intentionally out of scope for the first full `consteval` implementation:

- full class-object constant evaluation
- inheritance / virtual dispatch
- dynamic cast / RTTI
- variadic templates
- complete standard-library `type_traits`
- full C++ constant-evaluation compliance

The target is a correct and composable **subset** implementation that matches the project’s current C++ scope.
