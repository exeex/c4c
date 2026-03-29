# SFINAE Template-Pattern Parsing and Staged Support

Status: Open
Last Updated: 2026-03-29

## Goal

Make the parser reliably accept the common libstdc++ / stdlib SFINAE surface
forms that appear in template parameter lists and function signatures, even
before full template semantics and substitution-failure behavior are
implemented.

The immediate priority is parser coverage and stable AST ownership:

- parse the syntax completely
- preserve enough structure for later semantic work
- avoid recovery-driven state corruption
- for unsupported semantic behavior, fail explicitly in a staged / localized
  way instead of misparsing downstream declarations

## Why This Is A Separate Idea

The active `std::vector` bring-up is currently using system-header failures to
force parser improvements. That has already exposed a separate, reusable theme:

- SFINAE-shaped template parameters and signature patterns are common in
  libstdc++
- several failures are not "vector-specific"
- these patterns should be handled systematically instead of by ad hoc fixes in
  individual headers

This idea captures that cross-cutting work so it can be implemented once and
  reused across future standard-library bring-up slices.

## Initial Target Patterns

The first support wave should cover these six families:

1. Unnamed typed NTTP in `template<...>`

```cpp
template<typename T, std::enable_if_t<Cond, int> = 0>
template<typename T, __enable_if_t<Cond, bool> = true>
```

2. Named typed NTTP in `template<...>`

```cpp
template<typename T, std::enable_if_t<Cond, int> Dummy = 0>
```

3. Unnamed type parameter defaulted through `enable_if`

```cpp
template<typename T, typename = std::enable_if_t<Cond>>
```

4. Return-type SFINAE

```cpp
template<typename T>
auto f(T) -> std::enable_if_t<Cond, int>;
```

5. Parameter-type SFINAE

```cpp
template<typename T>
void f(T, std::enable_if_t<Cond, int>* = nullptr);
```

6. Partial specialization / alias-specialization gating through `enable_if`

```cpp
template<typename T, typename Enable = void>
struct X;

template<typename T>
struct X<T, std::enable_if_t<Cond>> { };
```

## Parsing Strategy

Short term:

- factor template-parameter parsing so top-level and record-member template
  preludes share the same type-head / NTTP handling
- accept alias / typedef / qualified / `typename ...::type` type-heads inside
  `template<...>`
- parse SFINAE-shaped declarations as ordinary syntax first
- do not require full substitution or overload-pruning semantics to claim
  parser support

Staged semantic fallback:

- if a parsed construct reaches a semantic phase that still lacks required
  support, emit a focused `not implemented` style error at the construct that
  needs semantic interpretation
- do not fall back to recovery that corrupts later parser state

## Non-Goals For The First Slice

Do not initially require:

- real substitution failure behavior
- overload set pruning via SFINAE
- full alias-template instantiation semantics
- complete template partial ordering

Those should become later semantic slices after parser stability is in place.

## Exit Criteria

- each of the six pattern families above has at least one reduced parser test
- parser support for those forms is shared, not duplicated across top-level and
  record-member code paths
- unsupported semantic follow-ons fail explicitly and locally, not via
  downstream parse corruption

## Relationship To Active Work

This idea is expected to feed back into `std::vector` / libstdc++ bring-up, but
it should remain independently activatable so parser-normalization work can be
reviewed separately from container-specific debugging.
