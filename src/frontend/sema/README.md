# Semantic Analysis Subsystem

The semantic analysis layer sits between parsing and HIR lowering. Its job is
to take the parser's AST, validate that it is semantically well-formed, compute
canonical type/symbol information, and hand a typed program forward to HIR.

This area is still evolving, so this document intentionally focuses on stable
responsibility boundaries rather than detailed implementation internals.

---

## What Sema Owns

At a high level, `src/frontend/sema` is responsible for:

- validating AST-level programs beyond pure syntax
- computing canonical type and symbol information
- handling semantic helpers used by compile-time reasoning
- producing the typed information needed by HIR lowering

The public entry point is `analyze_program(...)` in `sema.hpp`, which returns:

- validation results
- canonical-symbol / canonical-type results
- an optional HIR module

That makes sema the bridge from:

```txt
AST -> semantic validation + canonical typing -> HIR
```

## Module Organization

```txt
sema/
  sema.hpp              -- public analysis entrypoint and result types
  sema.cpp              -- orchestration of semantic analysis
  validate.*            -- semantic validation and diagnostics
  canonical_symbol.*    -- canonical symbol / type-resolution data
  type_utils.*          -- shared type reasoning helpers
  consteval.*           -- compile-time evaluation helpers used by analysis
```

The exact data flow between these files is still moving, but the split above is
the current stable mental model.

## Current Role In The Pipeline

Today sema is not just a "checker". It also acts as the point where the
frontend stops being mostly syntax-shaped and starts becoming type-shaped.

In practical terms:

- parser builds AST nodes and lightweight type syntax
- sema decides whether the program is semantically valid
- sema resolves canonical type information
- sema prepares or triggers the transition into HIR

If you are debugging "why did this parse, but later fail?" sema is usually the
first place to inspect.

## What To Read First

For a quick orientation:

1. `sema.hpp`
2. `sema.cpp`
3. `validate.hpp` / `validate.cpp`
4. `canonical_symbol.hpp` / `canonical_symbol.cpp`
5. `type_utils.*`
6. `consteval.*`

## Where To Edit

- semantic acceptance/rejection rules: `validate.cpp`
- canonical typing / resolved symbol representation: `canonical_symbol.cpp`
- shared type reasoning helpers: `type_utils.cpp`
- compile-time semantic helpers: `consteval.cpp`
- overall orchestration / returned analysis shape: `sema.cpp`, `sema.hpp`

## Invariants

- Sema sits after parsing and before codegen-facing HIR consumption.
- Validation and canonical typing belong here, not in the lexer/parser.
- This subsystem should document stable responsibilities, not transient pass
  choreography while the implementation is still changing.
