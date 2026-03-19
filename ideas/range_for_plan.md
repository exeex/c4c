# Range-For Plan

## Goal

Add `for (Type var : range_expr) body` support (C++11 range-based for loop)
as syntax sugar over the existing iterator/container infrastructure.

## Status

Completed for the current STL-enablement milestone.

Implemented and validated in-tree:

- parsing and AST support for range-based `for`
- lowering through member `begin()` / `end()` plus iterator
  `operator!=`, `operator++`, and `operator*`
- explicit typed loop variables such as `for (int x : c)`
- `auto` and `const auto` loop-variable deduction
- `auto&` and `const auto&` loop variables when iterator dereference returns
  `T&`
- integration coverage through the fixed-container smoke test

Not required to close this plan:

- `std::begin` / `std::end` free-function fallback
- ADL-based begin/end lookup
- structured bindings
- init-statements in range-for
- broader general `auto` deduction outside range-for

## Scope

### In scope

- parsing `for (Type var : expr) body` syntax
- desugaring to `begin()`/`end()` member calls + `operator!=`, `operator++`, `operator*`
- explicit loop variable type
- `auto` / `const auto` loop variables
- `auto&` / `const auto&` loop variables
- iterator dereference returning `T&` for reference-preserving loops

### Out of scope (first milestone)

- `std::begin` / `std::end` free-function fallback (ADL)
- structured bindings
- init-statement in range-for (`for (init; Type var : expr)`)
- broader standalone `auto` deduction outside range-for
- explicit non-`auto` reference loop variables if future tests require them

## Design

### Parsing

Detect range-for by looking for a colon after the declaration inside `for(...)`:

```
for ( type-specifier declarator : range-expression ) statement
```

Distinguish from regular for by:
1. Parse the type + declarator
2. If next token is `:` → range-for
3. If next token is `=` or `;` → regular for

Create `NK_RANGE_FOR` AST node with:
- `init` = loop variable declaration (NK_DECL)
- `right` = range expression
- `body` = loop body

### HIR Lowering

Desugar `NK_RANGE_FOR` into equivalent of:

```cpp
{
  auto __begin = range_expr.begin();
  auto __end = range_expr.end();
  for (; __begin != __end; ++__begin) {
    Type& var = *__begin;
    body
  }
}
```

Implementation:
1. Lower range expression, infer its struct type
2. Look up `begin()` and `end()` methods via `struct_methods_`
3. Create hidden locals `__range_begin` / `__range_end` with iterator type
4. Build ForStmt with:
   - no init (begin/end already declared)
   - cond = `__begin != __end` via `try_lower_operator_call`
   - update = `++__begin` via `try_lower_operator_call`
5. In body block: `var = *__begin` via `try_lower_operator_call`, then user body

## Phases

### Phase 0 (this slice)
- `NK_RANGE_FOR` node kind
- Parser detection and AST construction
- HIR desugaring
- Basic test: `range_for_basic.cpp`
- Const test: `range_for_const.cpp`

Status: complete

### Phase 1
- `auto` type deduction for loop variable
- `auto&` and `const auto&` reference loop variables
- Reference return types (`T&`) for `operator*`

Status: complete

### Phase 2 (future)
- `int&` / `const int&` explicit reference loop variables (non-auto), if
  real tests require them
- Structured bindings in range-for

Status: intentionally deferred

## Test Cases

- `range_for_basic.cpp` — sum elements of a container using range-for
- `range_for_const.cpp` — const container access via range-for
- `range_for_auto.cpp` — `for (auto x : c)` with auto deduction
- `range_for_const_auto.cpp` — `for (const auto x : c)` with const auto
- `range_for_auto_ref.cpp` — `for (auto& x : c)` and `for (const auto& x : c)` reference loop variables

## Definition of done

This plan is successful when:

- range-for syntax parses and lowers correctly
- member-based iterator traversal works through ordinary begin/end and operator
  calls
- explicit typed and `auto`-based loop variables required by the current STL
  milestone execute correctly
- container integration tests pass without range-for-specific hacks

This definition is now satisfied for the current member-begin/end range-for
subset. Deferred Phase 2 items are follow-on ergonomics work, not blockers on
plan completion.
