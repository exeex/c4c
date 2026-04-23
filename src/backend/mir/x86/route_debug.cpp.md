# `route_debug.cpp` extraction

## Purpose and current responsibility

This file no longer implements route-debug logic itself. Its active job is to
preserve a legacy top-level x86 codegen debug surface while forwarding all real
prepared-route reporting to the `debug/` subtree.

It is executable evidence that ownership already migrated away from this file:

- callers still enter through `c4c::backend::x86`
- formatting and traversal live elsewhere
- this file adds no local policy, caching, or filtering

## Important APIs and contract surfaces

The only meaningful surface is the pair of forwarding functions:

```cpp
std::string summarize_prepared_module_routes(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::optional<std::string_view> focus_function,
    std::optional<std::string_view> focus_block);

std::string trace_prepared_module_routes(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::optional<std::string_view> focus_function,
    std::optional<std::string_view> focus_block);
```

Observed contract:

- input is a prepared BIR module plus optional function/block focus selectors
- output is a fully materialized `std::string`
- top-level x86 namespace remains stable for older callers
- semantic behavior is delegated 1:1 to `c4c::backend::x86::debug::*`

Essential implementation shape:

```cpp
return c4c::backend::x86::debug::summarize_prepared_module_routes(
    module, focus_function, focus_block);
```

The trace function follows the same pattern.

## Dependency direction and hidden inputs

Dependency direction is one-way:

- `route_debug.cpp` depends on `route_debug.hpp`
- `route_debug.cpp` depends on `debug/prepared_route_debug.hpp`
- behavior depends on the hidden implementation choices inside `debug/`

Hidden inputs this file does not expose directly:

- whatever route classification and formatting rules `debug/` uses
- any assumptions `debug/` makes about `PreparedBirModule` completeness
- any performance cost from producing the returned strings

This means callers may think they are using a local x86 codegen utility, but
the real owner is already deeper in the dependency graph.

## Responsibility buckets

- Compatibility surface: keep old namespace-level entry points alive.
- Namespace adapter: translate no types, only preserve the legacy symbol path.
- Ownership marker: signal that prepared-route debug authority moved to
  `debug/`.

Not owned here:

- route traversal
- route summarization logic
- trace formatting logic
- focus selection semantics

## Fast paths, compatibility paths, and overfit pressure

- Core behavior: none in this file; it is only a forwarder.
- Compatibility path: strong. The whole file exists to avoid forcing legacy
  callers to include or call `debug/` directly.
- Fast path: trivial call-through with no extra work.
- Overfit pressure: low inside this file, but there is pressure to keep adding
  more namespace-level wrappers instead of deleting or redesigning the legacy
  surface. In a rebuild, this file should not become a dumping ground for
  one-off debug adapters.

## Rebuild ownership line

This file should own only a narrow compatibility seam, if that seam is still
required at all.

It should:

- expose the minimal legacy entry points that cannot yet be removed
- forward into the canonical prepared-route debug owner without adding policy

It should not:

- regain route-debug implementation logic
- define formatting, traversal, or filtering rules
- become a second public debug API beside the canonical `debug/` owner
