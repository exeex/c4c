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

In the current pipeline, the broad shape is:

```txt
AST -> sema -> HIR -> LLVM IR
```

## Module Organization

```txt
hir/
  ir.hpp                -- core HIR data structures
  ast_to_hir.*          -- AST/HIR lowering entrypoint
  compile_time_pass.*   -- compile-time reduction / materialization-style passes
  inline_expand.*       -- HIR-level inline expansion helpers
  hir_printer.*         -- debug printing and summaries
```

`ir.hpp` is the main contract file. The other files either build, transform, or
inspect that IR.

## Current Role In The Pipeline

HIR is the place where frontend information becomes codegen-oriented.

That includes, at a high level:

- typed functions, globals, locals, blocks, and expressions
- linkage / storage / visibility style metadata
- a representation suitable for frontend passes before LLVM emission
- enough structure for debug dumping and regression-oriented inspection

The HIR layer is also where some compile-time and materialization decisions are
tracked before final LLVM emission.

## Public Entry Points

The main entry points to know about are:

- `lower_ast_to_hir(...)` in `ast_to_hir.hpp`
- HIR formatting / summaries in `hir_printer.*`
- compile-time reduction / related pass helpers in `compile_time_pass.*`
- inline expansion helpers in `inline_expand.*`

If you are tracing a feature from parsed syntax into emitted LLVM IR, HIR is
usually the most important intermediate checkpoint.

## What To Read First

For a quick orientation:

1. `ir.hpp`
2. `ast_to_hir.hpp` and `ast_to_hir.cpp`
3. `hir_printer.hpp` and `hir_printer.cpp`
4. `compile_time_pass.*`
5. `inline_expand.*`

## Where To Edit

- core IR shapes and IDs: `ir.hpp`
- AST -> HIR lowering rules: `ast_to_hir.cpp`
- HIR debug output / summaries: `hir_printer.cpp`
- compile-time reduction / materialization behavior: `compile_time_pass.cpp`
- inline expansion behavior: `inline_expand.cpp`

## Invariants

- HIR is the typed frontend IR, not just a pretty-printed AST.
- `ir.hpp` is the contract surface; keep documentation here high level while
  the representation is still moving.
- Codegen-facing behavior should consume HIR rather than reaching back into raw
  parser AST structures.
