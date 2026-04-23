# `prepared_module_emit.cpp`

## Purpose And Current Responsibility

This file is a legacy compatibility shim. It preserves the historical
top-level `c4c::backend::x86::emit_prepared_module(...)` entrypoint while
delegating the real work to the newer `module` sub-namespace implementation.

It does not perform x86 lowering, formatting, scheduling, or emission logic.
Its only responsibility is symbol continuity and namespace bridging.

## Important API And Contract Surface

```cpp
std::string emit_prepared_module(
    const c4c::backend::prepare::PreparedBirModule& module);
```

Observed contract:

- accepts a fully prepared BIR module
- returns emitted module text as `std::string`
- forwards directly to the current owner without transformation

Forward target:

```cpp
namespace c4c::backend::x86::module {
[[nodiscard]] std::string emit_prepared_module_text(
    const c4c::backend::prepare::PreparedBirModule& module);
}
```

This means callers still depending on the old namespace surface get the same
result path as newer code that calls the `module::...` API directly.

## Dependency Direction And Hidden Inputs

Direct dependency direction is one-way:

- this file depends on `module/module_emit.hpp`
- ownership flows inward to `c4c::backend::x86::module`

Hidden inputs are entirely inherited from the downstream emitter:

- whatever invariants `PreparedBirModule` must already satisfy
- whatever target defaults, layout assumptions, or formatting policy the real
  emitter reads internally

This shim adds no policy of its own; it only passes the module through.

## Responsibility Buckets

- Legacy compatibility: preserve the old top-level symbol.
- API forwarding: route calls into the current module emitter surface.
- Namespace adaptation: bridge `c4c::backend::x86` to
  `c4c::backend::x86::module`.

## Fast Paths, Compatibility Paths, And Overfit Pressure

- Core lowering: none.
- Optional fast path: none.
- Legacy compatibility: the whole file is a compatibility path.
- Overfit pressure: low in local logic, but high structurally if rebuild work
  keeps preserving redundant wrapper surfaces after call sites can migrate.

The main rebuild risk is letting this shim become a permanent second owner of
the emit API instead of a temporary migration seam.

## Rebuild Ownership

This file should own only a transitional compatibility entrypoint, if one is
still required by external callers.

It should not own emission logic, prepared-module interpretation, formatting
policy, or any x86 backend behavior. In a rebuild, those belong to the real
module-emission surface, and this wrapper should either stay trivially thin or
disappear once callers are migrated.
