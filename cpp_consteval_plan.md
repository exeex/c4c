# C++ Consteval Plan

## Goal

Build and stabilize a real C++ subset constant-evaluation pipeline for `consteval`, instead of treating it as a parser-only marker.

## Status Summary

### Completed

- Parser support for `constexpr`, `consteval`, and `if constexpr`
- AST flags for `is_constexpr` and `is_consteval`
- Unified constant-expression entry point in [src/frontend/sema/consteval.cpp](/workspaces/c4c/src/frontend/sema/consteval.cpp)
- Named constant lookup for:
  - enum constants
  - foldable global const bindings
  - foldable local const/constexpr bindings
  - consteval parameters and locals during interpretation
- Consteval function-body interpretation for the current subset:
  - locals
  - block shadowing
  - assignment
  - `if`
  - `for` / `while` / `do while`
  - nested consteval calls
  - template-substituted `sizeof(T)` / `alignof(T)`
  - logical short-circuiting
- Immediate-call enforcement in HIR lowering:
  - consteval calls must fold at compile time
  - non-constant arguments are rejected
  - evaluation failure is a hard error
  - taking the address of a consteval function is rejected
- Regression coverage for the current implementation, including:
  - [tests/internal/cpp/postive_case/consteval_func.cpp](/workspaces/c4c/tests/internal/cpp/postive_case/consteval_func.cpp)
  - [tests/internal/cpp/postive_case/consteval_interp.cpp](/workspaces/c4c/tests/internal/cpp/postive_case/consteval_interp.cpp)
  - [tests/internal/cpp/postive_case/consteval_mutable.cpp](/workspaces/c4c/tests/internal/cpp/postive_case/consteval_mutable.cpp)
  - [tests/internal/cpp/postive_case/consteval_call_resolution.cpp](/workspaces/c4c/tests/internal/cpp/postive_case/consteval_call_resolution.cpp)
  - [tests/internal/cpp/postive_case/consteval_recursive_fib.cpp](/workspaces/c4c/tests/internal/cpp/postive_case/consteval_recursive_fib.cpp)
  - [tests/internal/cpp/postive_case/consteval_template.cpp](/workspaces/c4c/tests/internal/cpp/postive_case/consteval_template.cpp)
  - [tests/internal/cpp/postive_case/consteval_template_sizeof.cpp](/workspaces/c4c/tests/internal/cpp/postive_case/consteval_template_sizeof.cpp)
  - [tests/internal/cpp/postive_case/consteval_block_scope.cpp](/workspaces/c4c/tests/internal/cpp/postive_case/consteval_block_scope.cpp)
  - [tests/internal/cpp/postive_case/consteval_short_circuit.cpp](/workspaces/c4c/tests/internal/cpp/postive_case/consteval_short_circuit.cpp)
  - [tests/internal/cpp/negative_case/bad_consteval_non_constant_arg.cpp](/workspaces/c4c/tests/internal/cpp/negative_case/bad_consteval_non_constant_arg.cpp)
  - [tests/internal/cpp/negative_case/bad_consteval_non_constant_expr_stmt.cpp](/workspaces/c4c/tests/internal/cpp/negative_case/bad_consteval_non_constant_expr_stmt.cpp)
  - [tests/internal/cpp/negative_case/bad_consteval_divide_by_zero.cpp](/workspaces/c4c/tests/internal/cpp/negative_case/bad_consteval_divide_by_zero.cpp)
  - [tests/internal/cpp/negative_case/bad_consteval_addr_of.cpp](/workspaces/c4c/tests/internal/cpp/negative_case/bad_consteval_addr_of.cpp)

### Remaining

- Broaden the value domain beyond integer-like values where the subset needs it
- Reuse the same engine more consistently for all `constexpr` and global-initializer paths
- Tighten diagnostics and unsupported-AST handling so failures are precise and predictable
- Expand coverage for edge cases that are still outside the supported subset

## Current Architecture

### 1. Constant evaluation is now a real subsystem

The old integer-expression helper has been expanded into a reusable evaluator plus a small consteval interpreter.

Key files:

- [src/frontend/sema/consteval.hpp](/workspaces/c4c/src/frontend/sema/consteval.hpp)
- [src/frontend/sema/consteval.cpp](/workspaces/c4c/src/frontend/sema/consteval.cpp)

### 2. Immediate calls are enforced during lowering

HIR lowering collects consteval function definitions, evaluates calls with constant arguments, and emits hard errors instead of runtime fallback.

Key file:

- [src/frontend/hir/ast_to_hir.cpp](/workspaces/c4c/src/frontend/hir/ast_to_hir.cpp)

### 3. Semantic validation already reuses part of the evaluator

The same constant-evaluation entry point is already used for local const folding and switch-case constant checks.

Key file:

- [src/frontend/sema/validate.cpp](/workspaces/c4c/src/frontend/sema/validate.cpp)

## Architectural Direction

We still want to keep these three concepts separate:

1. constant expression evaluation

- expression interpreter for AST/HIR values

2. `constexpr`

- declaration/property saying something may participate in constant evaluation

3. `consteval`

- immediate-function policy saying a call must be evaluated in an immediate context

This remains important because:

- not every constant expression comes from a `consteval` function
- not every `constexpr` function call must be folded immediately
- `consteval` is a semantic rule on calls, not just a flag on declarations

## Recommended Follow-Up

### 1. Keep the explicit constant-evaluation domain and extend it carefully

Current value/result types are enough for the current arithmetic subset.

When extending further, keep the domain intentionally narrow:

- integer
- boolean
- character
- null pointer
- invalid / unknown

Do not widen scope prematurely into:

- heap objects
- dynamic type / RTTI
- virtual dispatch
- arbitrary class object interpretation

### 2. Continue unifying constant evaluation behind a single API

The project should keep converging on:

```text
evaluate_constant_expr(node, context) -> ConstEvalResult
```

With thinner wrappers on top:

- `evaluate_ice(...)`
- `evaluate_immediate_call(...)`
- `evaluate_global_initializer(...)`

This keeps logic from drifting between:

- `validate.cpp`
- `consteval.cpp`
- `ast_to_hir.cpp`
- future C++ semantic passes

### 3. Keep expanding named constant support only where the subset needs it

Already supported today:

```cpp
const int x = 4;
const int y = 5;
const int z = x + y;
```

Current environment sources:

- enum constants
- global constant bindings
- local constant bindings
- consteval frame locals and parameters

Still worth preserving:

- lexical lookup rules
- clear frame boundaries
- deterministic failure diagnostics

### 4. Keep treating `consteval` calls as semantic evaluation requests

When the analyzer sees a call to a function marked `is_consteval`, the intended flow remains:

- resolve the target function
- create an evaluation frame
- bind arguments as `ConstValue`
- interpret the body
- produce a constant result or a structured failure

If evaluation fails:

- emit a semantic error
- do not silently fall back to runtime lowering

### 5. Keep runtime codegen as an implementation detail, not the meaning

For the subset, temporary implementation scaffolding is acceptable.

But semantically:

- the call result must come from constant evaluation
- runtime fallback must not define the meaning of `consteval`

## Validation Plan

### Positive cases to keep/add

Core coverage:

- [tests/internal/cpp/postive_case/consteval_func.cpp](/workspaces/c4c/tests/internal/cpp/postive_case/consteval_func.cpp)
- [tests/internal/cpp/postive_case/consteval_template.cpp](/workspaces/c4c/tests/internal/cpp/postive_case/consteval_template.cpp)
- [tests/internal/cpp/postive_case/consteval_interp.cpp](/workspaces/c4c/tests/internal/cpp/postive_case/consteval_interp.cpp)
- [tests/internal/cpp/postive_case/consteval_mutable.cpp](/workspaces/c4c/tests/internal/cpp/postive_case/consteval_mutable.cpp)
- [tests/internal/cpp/postive_case/consteval_call_resolution.cpp](/workspaces/c4c/tests/internal/cpp/postive_case/consteval_call_resolution.cpp)
- [tests/internal/cpp/postive_case/consteval_recursive_fib.cpp](/workspaces/c4c/tests/internal/cpp/postive_case/consteval_recursive_fib.cpp)
- [tests/internal/cpp/postive_case/consteval_template_sizeof.cpp](/workspaces/c4c/tests/internal/cpp/postive_case/consteval_template_sizeof.cpp)
- [tests/internal/cpp/postive_case/consteval_block_scope.cpp](/workspaces/c4c/tests/internal/cpp/postive_case/consteval_block_scope.cpp)
- [tests/internal/cpp/postive_case/consteval_short_circuit.cpp](/workspaces/c4c/tests/internal/cpp/postive_case/consteval_short_circuit.cpp)

### Negative cases to keep/add

- [tests/internal/cpp/negative_case/bad_consteval_non_constant_arg.cpp](/workspaces/c4c/tests/internal/cpp/negative_case/bad_consteval_non_constant_arg.cpp)
- [tests/internal/cpp/negative_case/bad_consteval_runtime_arg.cpp](/workspaces/c4c/tests/internal/cpp/negative_case/bad_consteval_runtime_arg.cpp)
- [tests/internal/cpp/negative_case/bad_consteval_non_constant_expr_stmt.cpp](/workspaces/c4c/tests/internal/cpp/negative_case/bad_consteval_non_constant_expr_stmt.cpp)
- [tests/internal/cpp/negative_case/bad_consteval_divide_by_zero.cpp](/workspaces/c4c/tests/internal/cpp/negative_case/bad_consteval_divide_by_zero.cpp)
- [tests/internal/cpp/negative_case/bad_consteval_addr_of.cpp](/workspaces/c4c/tests/internal/cpp/negative_case/bad_consteval_addr_of.cpp)
- [tests/internal/cpp/negative_case/bad_consteval_on_variable.cpp](/workspaces/c4c/tests/internal/cpp/negative_case/bad_consteval_on_variable.cpp)
- [tests/internal/cpp/negative_case/bad_consteval_local_variable.cpp](/workspaces/c4c/tests/internal/cpp/negative_case/bad_consteval_local_variable.cpp)
- [tests/internal/cpp/negative_case/bad_consteval_constexpr_combo.cpp](/workspaces/c4c/tests/internal/cpp/negative_case/bad_consteval_constexpr_combo.cpp)

## Non-Goals For This Plan

The following remain out of scope for the first full `consteval` implementation:

- full class-object constant evaluation
- inheritance / virtual dispatch
- dynamic cast / RTTI
- variadic templates
- complete standard-library `type_traits`
- full C++ constant-evaluation compliance

The target is a correct and composable subset implementation that matches the project’s current C++ scope.
