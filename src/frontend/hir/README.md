# HIR Subsystem

HIR is the frontend's typed intermediate representation. It is the lowering
target after parsing/semantic analysis and the input consumed by LLVM IR
generation.

This area is still under active iteration, so this README stays intentionally
high level. It is meant to explain what HIR is for, not to freeze every field
or pass detail.

---

## What HIR Is

HIR exists to give the frontend a representation that is:

- more structured and typed than the parser AST
- easier to analyze and transform than raw syntax trees
- closer to code generation needs, without being LLVM IR yet
- able to carry unresolved compile-time work until a dedicated normalization step finishes it

In the current pipeline, the broad shape is:

```txt
AST -> sema -> HIR build -> compile-time normalization -> materialization -> LLVM IR
```

The key idea is that HIR is not just a codegen-oriented typed IR.
It is also the place where compile-time-required computation remains explicit
until the frontend has reduced it as far as required.

## Module Organization

```txt
hir/
  ir.hpp                -- core HIR data structures (the contract surface)
  hir.*                 -- public pipeline entrypoint (thin orchestrator)
  ast_to_hir.*          -- AST -> initial HIR construction
  compile_time_engine.* -- compile-time engine: state, reduction, materialization
  inline_expand.*       -- HIR-level inline expansion helpers
  hir_printer.*         -- debug printing and summaries
```

`ir.hpp` is the main contract file. The other files either build, transform, or
inspect that IR.

## Current Role In The Pipeline

HIR is the place where frontend information becomes both:

- codegen-oriented
- compile-time-normalization-oriented

That includes, at a high level:

- typed functions, globals, locals, blocks, and expressions
- linkage / storage / visibility style metadata
- a representation suitable for frontend passes before LLVM emission
- enough structure for debug dumping and regression-oriented inspection
- explicit representation of compile-time work that still needs reduction

The HIR layer should be thought of as a mixed compile-time/runtime IR.
Some nodes are already runtime-ready. Others still require compile-time
normalization before final code generation should treat them as settled.

## Pipeline

The HIR build pipeline has three explicit stages:

1. **Initial** — `build_initial_hir()` in `ast_to_hir.*` lowers the AST into a
   mixed compile-time/runtime HIR. Template seeds and consteval definitions are
   recorded in `CompileTimeState`.

2. **Normalized** — `run_compile_time_engine()` in `compile_time_engine.*`
   iterates a fixpoint loop: template instantiation, consteval evaluation,
   and reduction verification until convergence.

3. **Materialized** — `materialize_ready_functions()` marks functions for
   emission. Consteval-only functions are excluded from LLVM output.

`build_hir()` in `hir.*` is the public entrypoint that orchestrates all three
stages and returns the final module.

## Public Entry Points

- `build_hir(...)` in `hir.hpp` — the main pipeline entrypoint
- `build_initial_hir(...)` in `ast_to_hir.hpp` — AST to initial HIR
- `run_compile_time_engine(...)` in `compile_time_engine.hpp` — fixpoint reduction
- HIR formatting / summaries in `hir_printer.*`
- inline expansion helpers in `inline_expand.*`

## What To Read First

1. `ir.hpp`
2. `hir.hpp` and `hir.cpp` (pipeline orchestration)
3. `ast_to_hir.*` (AST lowering)
4. `compile_time_engine.*` (compile-time state + reduction)
5. `hir_printer.*`
6. `inline_expand.*`

## Where To Edit

- core IR shapes and IDs: `ir.hpp`
- AST-to-HIR lowering rules: `ast_to_hir.cpp`
- pipeline orchestration: `hir.cpp`
- compile-time engine behavior and state: `compile_time_engine.cpp`
- HIR debug output / summaries: `hir_printer.cpp`
- inline expansion behavior: `inline_expand.cpp`

## Invariants

- HIR is the typed frontend IR, not just a pretty-printed AST.
- HIR is allowed to contain compile-time-required nodes that are not yet fully reduced.
- `ir.hpp` is the contract surface; keep documentation here high level while
  the representation is still moving.
- Codegen-facing behavior should consume HIR rather than reaching back into raw
  parser AST structures.
- Compile-time normalization should reduce HIR until no required compile-time
  work remains, or fail with diagnostics instead of hanging indefinitely.
