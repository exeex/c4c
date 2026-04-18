# `__strlit` Compile-Time Concat And Legacy Decay

## Goal

Introduce a front-end string-literal type model that keeps string literals as a
compiler-owned `__strlit` long enough to support compile-time string
operations such as `__strlit + __strlit`, while still preserving existing C/C++
behavior by lowering back to legacy array/pointer string-literal semantics when
the enriched rules do not apply.

The intended end state is:

- source string literals start semantic analysis as `__strlit`
- adjacent literal concatenation remains the existing lexer/parser rule
- `__strlit + __strlit` becomes a compile-time concatenation operation
- `__strlit` can participate in compiler-owned constant-string use cases such as
  asm template construction
- expressions that do not match `strlit`-specific rules can still fall back to
  ordinary C/C++ string-literal lowering and array-to-pointer decay

## Why This Idea Exists

Current C/C++ string literals decay too quickly into legacy array/pointer
semantics for the kind of compile-time string programming we want.

That blocks or complicates patterns such as:

- assembling target-specific asm template strings from compile-time pieces
- writing `consteval` / `constexpr` helpers that return string fragments
- allowing `"vload_" + to_asm_prefix<T>() + "{%0}"`-style source to mean
  compile-time string assembly instead of invalid pointer arithmetic

At the same time, c4c still wants broad C/C++ compatibility. Existing code that
relies on legacy string-literal decay such as `"hello" + 1` should not be
silently broken if the enriched string rules do not match.

This idea exists to define a two-layer model:

- enriched `__strlit` semantics first
- legacy C/C++ string-literal lowering second

## Core Semantic Model

### 1. Initial literal type

Every string literal enters semantic analysis as a compiler-owned `__strlit`
value that still preserves length and null-termination metadata.

This is a front-end semantic type, not `std::string`, and not yet the legacy
`const char[N]` array view.

### 2. Existing adjacent literal concat stays unchanged

The language should continue to treat:

```cpp
"hello" "world"
```

as the existing adjacent-literal concatenation rule before normal semantic
typing. This idea does not replace that rule with `operator+`.

### 3. `__strlit + __strlit`

When both operands are `__strlit`, `+` should produce a compile-time
concatenated `__strlit`.

That gives a direct path for:

```cpp
"vload_" + get_prefix<T>() + "{%0}"
```

provided `get_prefix<T>()` also produces a compile-time string result.

### 4. Mixed string-object cases

When one side is a library string object such as `std::string`, the language
should prefer the string-object route rather than forcing legacy pointer
lowering first.

Examples:

- `__strlit + std::string -> std::string`
- `std::string + __strlit -> std::string`

### 5. Legacy decay fallback

If an expression does not match a valid `__strlit` rule, the compiler may lower
`__strlit` into the legacy C/C++ form and then continue normal typing.

That fallback should preserve behavior such as:

- `"hello" + 1` meaning pointer arithmetic after legacy lowering
- passing a string literal to APIs that expect ordinary character-array /
  pointer semantics

This fallback is the key compatibility bridge. `__strlit` is not intended to
delete legacy string-literal behavior wholesale.

## Proposed Type Behavior

### `__strlit` surface properties

- preserves compile-time length
- preserves null termination
- is eligible for compile-time concatenation
- can be materialized into legacy string-literal storage when fallback or code
  generation requires it

### Questions to resolve explicitly

- whether `decltype("hello")` should expose `__strlit` or preserve legacy
  `const char[6]` user-facing behavior
- whether template deduction sees `__strlit` directly or sees the lowered
  legacy type outside compiler-internal contexts
- whether `sizeof("hello")` should continue to report legacy array size through
  immediate lowering or via equivalent `__strlit` metadata
- whether `__strlit` should support indexing directly before fallback

## Constant-Evaluation Use Cases

The first motivating use case is compile-time asm string assembly through a
`consteval` helper that returns `__strlit`.

Example target shape:

```cpp
template <typename T>
consteval __strlit get_prefix() {
    if constexpr (std::is_same_v<T, float>) {
        return "f32";
    }
    // ...
}

template<typename T>
void vadd_op(T* a, T* b, T* c, int n) {
    asm("vload_" + get_prefix<T>() + "{%0}" : ptr(a));
}
```

For that to work cleanly:

- the asm template operand must accept a constant string expression, not only a
  raw token-level string literal
- `get_prefix<T>()` must be able to produce a compile-time string result of
  type `__strlit`
- constant folding must be able to reduce chained `__strlit` concatenation before
  asm semantic checks

This idea is not asm-only, but asm is the clearest near-term proving surface.

## Proposed Execution Order

### 1. Front-end semantic representation

- introduce compiler-owned `strlit<N>` handling in the type system
- keep it explicit and internal at first; do not immediately broaden every
  user-visible type query

### 2. Compile-time concat rule

- implement `strlit + strlit -> strlit`
- ensure the result remains a constant string value

### 3. Legacy fallback

- define the exact point where unresolved `strlit` expressions lower into the
  ordinary string-literal array/pointer model
- preserve existing valid legacy behavior such as pointer arithmetic and API
  passing

### 4. Constant-string consumers

- let asm template operands accept folded constant string expressions
- identify whether other compiler-owned consumers such as attributes, builtins,
  or diagnostics should accept the same constant-string form

### 5. Follow-on library / language surface

- decide whether explicit user-facing helpers such as `consteval` string
  builders or fixed-string library integration should be layered on top later

## Constraints

- do not break existing adjacent string-literal concatenation
- do not reinterpret `__strlit + int` as string concatenation
- do not silently remove legacy pointer arithmetic behavior when the enriched
  string rules fail to match
- do not special-case asm only; the compile-time string model should be general
- do not turn this into implicit `std::string` construction everywhere

## Non-Goals

- replacing the standard-library string model
- making string literals runtime heap objects
- introducing broad dynamic string semantics into the core language
- solving full formatting / interpolation in the same idea
- designing every possible compile-time string API before the base type and
  fallback model exist

## Success Criteria

- the compiler can represent a source string literal as an enriched compile-time
  `__strlit` form before legacy decay
- `__strlit + __strlit` works as compile-time concatenation
- existing legacy string-literal behavior can still be reached when the new
  rules do not apply
- at least one compiler-owned constant-string consumer such as asm template
  parsing accepts a folded constant string expression
- the design remains compatible with ordinary C/C++ code that expects pointer
  semantics after fallback

## Good First Patch

Define the internal `__strlit` semantic representation and a minimal
`__strlit + __strlit` constant-folding path, then prove it on a narrow compiler
surface such as an asm template operand that accepts a folded constant string.
