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
  ir.hpp                -- core HIR data structures
  hir.*                 -- HIR entrypoint: AST seed collection + HIR construction
  compile_time_engine.* -- current compile-time engine / normalization logic
  inline_expand.*       -- HIR-level inline expansion helpers
  hir_printer.*         -- debug printing and summaries
```

`ir.hpp` is the main contract file. The other files either build, transform, or
inspect that IR.

Compatibility headers with the old names (`ast_to_hir.hpp`,
`compile_time_pass.hpp`) still exist during migration, but the canonical names
are now `hir.*` and `compile_time_engine.*`.

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

## Public Entry Points

The main entry points to know about are:

- `build_hir(...)` in `hir.hpp`
- HIR formatting / summaries in `hir_printer.*`
- compile-time normalization / engine helpers in `compile_time_engine.*`
- inline expansion helpers in `inline_expand.*`

If you are tracing a feature from parsed syntax into emitted LLVM IR, HIR is
usually the most important intermediate checkpoint.

## What To Read First

For a quick orientation:

1. `ir.hpp`
2. `compile_time_engine.*`
3. `hir.hpp` and `hir.cpp`
4. `hir_printer.hpp` and `hir_printer.cpp`
5. `inline_expand.*`

## Where To Edit

- core IR shapes and IDs: `ir.hpp`
- AST seed collection + HIR construction rules: `hir.cpp`
- compile-time normalization behavior: `compile_time_engine.cpp`
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
