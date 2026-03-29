# Type-Trait Builtin Gap Audit

Status: Open
Last Updated: 2026-03-29

## Goal

Inventory the real compiler builtin type traits currently exercised by the
`std::vector` / libstdc++ bring-up path, separate them from libstdc++
helper wrappers, and turn that inventory into a narrow implementation order.

## Why This Should Be A Separate Idea

The active `std::vector` parser bring-up already moved past the latest
type-trait parsing blocker. The remaining direct repro failures now start in
`/usr/include/c++/14/concepts` and `bits/stl_iterator.h`, so broadening that
plan into builtin-semantics work would mix two different initiatives.

This idea exists to answer a more precise question:

- which `__is_*` / `__has_*` names on the real libstdc++ path are actual
  compiler builtins we need to model,
- which ones are only library helpers and should not be counted as builtin
  implementation work,
- and what is the smallest high-value builtin subset to implement first.

## Current Evidence

Using the current repro path:

```bash
./build/c4cll --pp-only tests/cpp/std/std_vector_simple.cpp > /tmp/std_vector_simple.pp
```

The preprocessed output currently contains 33 unique function-like
`__is_*` / `__has_*` names.

That raw set is not the same thing as "33 builtins to implement". It mixes:

- real compiler trait builtins used directly by libstdc++
- library helper wrappers and concepts whose names happen to start with
  `__is_` / `__has_`

A curated first-pass candidate builtin set for this path is:

- `__has_trivial_destructor`
- `__has_virtual_destructor`
- `__is_abstract`
- `__is_assignable`
- `__is_base_of`
- `__is_class`
- `__is_constructible`
- `__is_empty`
- `__is_enum`
- `__is_final`
- `__is_literal_type`
- `__is_nothrow_assignable`
- `__is_nothrow_constructible`
- `__is_pod`
- `__is_polymorphic`
- `__is_same`
- `__is_standard_layout`
- `__is_trivial`
- `__is_trivially_assignable`
- `__is_trivially_constructible`
- `__is_trivially_copyable`
- `__is_union`

That is 22 likely builtin traits on the current `std::vector` path.

Names that currently look helper-like and should be audited separately before
counting them as builtin work:

- `__is_complete_or_unbounded`
- `__is_implicitly_constructible`
- `__is_explicitly_constructible`
- `__is_implicitly_default_constructible`
- `__is_explicitly_default_constructible`
- `__is_alloc_arg`
- `__is_derived_from_view_interface_fn`
- `__has_single_bit`
- `__is_permutation`

## Scope

Do:

- derive a trustworthy builtin inventory from the real repro path
- map each builtin name to current parser-only support vs semantic support
- choose a small first implementation slice with high libstdc++ leverage
- add focused reduced tests for each semantic builtin slice that is claimed

Do not:

- silently absorb this work into the parked `std::vector` parser bring-up idea
- count every `__is_*` helper in libstdc++ as a compiler builtin
- promise "all GCC / Clang traits" up front

## Exit Criteria

- one audited list distinguishes builtin traits from helper wrappers
- one ordered shortlist identifies the first builtin traits worth implementing
- the first selected slice has reduced tests and a clear success metric
