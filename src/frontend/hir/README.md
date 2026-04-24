# HIR Subsystem

HIR is the frontend's typed intermediate representation. It sits after parsing
and semantic analysis, and before backend-facing lowering and emission.

This README documents the current header hierarchy and the include boundaries
established by the active plan.

## Public Facade

These headers are the app-facing entry points for HIR:

- `hir.hpp` - public module entrypoint and lightweight summary formatting
- `hir_ir.hpp` - core HIR data model and shared value types
- `compile_time_engine.hpp` - deferred compile-time retry and materialization API
- `inline_expand.hpp` - public inline-expansion transform API
- `hir_printer.hpp` - full HIR dump / inspection formatting API

`hir.hpp` intentionally stays small: it exposes `build_hir(...)` and
`format_summary(...)`, while the fuller dump formatter remains in
`hir_printer.hpp`.

## Private Implementation Indexes

These headers are implementation-only and should not be treated as the public
surface:

- `impl/hir_impl.hpp` - private HIR lowering index and shared lowering helpers
- `impl/lowerer.hpp` - private lowering engine index used by the `hir_*.cpp`
  translation units
- `impl/compile_time/compile_time.hpp` - private compile-time/materialization
  implementation index
- `impl/inspect/inspect.hpp` - private inspection index for debug internals

## Subdomain Indexes

The private lowering index is further split into focused subdomain headers:

- `impl/expr/expr.hpp` - expression lowering internals
- `impl/stmt/stmt.hpp` - statement lowering internals
- `impl/templates/templates.hpp` - template and dependent lowering internals
- `impl/compile_time/compile_time.hpp` - compile-time and materialization
  internals
- `impl/inspect/inspect.hpp` - inspection and debug dump internals

## Current Boundary

The current structure is:

```txt
public facade
  hir.hpp
  hir_ir.hpp
  compile_time_engine.hpp
  inline_expand.hpp
  hir_printer.hpp

private lowering / pipeline internals
  impl/hir_impl.hpp
  impl/lowerer.hpp
  impl/compile_time/compile_time.hpp
  impl/expr/expr.hpp
  impl/stmt/stmt.hpp
  impl/templates/templates.hpp
  impl/inspect/inspect.hpp
```

The public facade is for app-facing callers. Implementation files should stay on
the private side of the boundary and include the narrowest index that matches
their domain.

## Notes

- `compile_time_engine.hpp` is still public because pipeline and dump callers
  use it directly.
- `inline_expand.hpp` remains public because `c4cll` invokes the transform
  before emission.
- `hir_printer.hpp` remains public for full HIR dumps, while
  `impl/inspect/inspect.hpp` is the private implementation-side bridge.
- This README intentionally avoids listing aspirational files that are not part
  of the current tree.
