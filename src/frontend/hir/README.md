# HIR Subsystem

HIR is the frontend's typed intermediate representation. It sits after parsing
and semantic analysis, and before backend-facing lowering and emission.

This README documents the current header hierarchy, implementation layout, and
include boundaries for agents working in HIR.

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
- `impl/lowerer.hpp` - private lowering engine index used by root lowering
  translation units and lowering subdomains
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

## Current Tree

The current structure is:

```txt
src/frontend/hir/
  hir.hpp
  hir_ir.hpp
  compile_time_engine.hpp
  inline_expand.hpp
  hir_printer.hpp
  README.md
  hir.cpp
  hir_build.cpp
  hir_functions.cpp
  hir_lowering_core.cpp
  hir_types.cpp
  impl/hir_impl.hpp
  impl/lowerer.hpp
  impl/compile_time/compile_time.hpp
  impl/compile_time/engine.cpp
  impl/compile_time/inline_expand.cpp
  impl/expr/expr.hpp
  impl/expr/*.cpp
  impl/stmt/stmt.hpp
  impl/stmt/*.cpp
  impl/templates/templates.hpp
  impl/templates/*.cpp
  impl/inspect/inspect.hpp
  impl/inspect/printer.cpp
```

The public facade is for app-facing callers. Implementation files should stay on
the private side of the boundary and include the narrowest index that matches
their domain.

## Agent Navigation

- Use `hir.hpp` for AST-to-HIR pipeline entry points and summary formatting.
- Use `hir_ir.hpp` for the shared HIR data model consumed by sema, HIR-local
  transforms, dump tooling, and codegen.
- Use `hir_printer.hpp` for public full-module dump formatting. Its
  implementation lives in `impl/inspect/printer.cpp`.
- Use `impl/hir_impl.hpp` and `impl/lowerer.hpp` for root lowering work.
- Use `impl/expr/expr.hpp`, `impl/stmt/stmt.hpp`,
  `impl/templates/templates.hpp`, and `impl/compile_time/compile_time.hpp` for
  subdomain implementation work.
- Use `impl/inspect/inspect.hpp` only from private inspection/debug
  implementation files.

## Notes

- `compile_time_engine.hpp` is still public because pipeline and dump callers
  use it directly.
- `inline_expand.hpp` remains public because `c4cll` invokes the transform
  before emission.
- `hir_printer.hpp` remains public for full HIR dumps, while
  `impl/inspect/inspect.hpp` is the private implementation-side bridge for
  `impl/inspect/printer.cpp`.
- This README intentionally avoids listing aspirational files that are not part
  of the current tree.
