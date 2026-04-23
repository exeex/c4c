# X86 Backend Contract

`src/backend/mir/x86/` is the root contract for the x86-family backend route.
There is no extra `codegen/` ownership layer anymore. The x86 subtree itself
is the implementation surface, and its immediate child directories are the
owned seams.

## Current Shape

```text
src/backend/mir/x86/
  README.md                root architecture contract
  x86.hpp                  thin compatibility holdout
  route_debug.hpp          thin compatibility holdout

  api/                     public x86 backend entrypoints
  core/                    text buffers and shared result carriers
  abi/                     target-triple and naming policy helpers
  module/                  whole-module orchestration and data-section stitching
  lowering/                semantic-family seams
  prepared/                bounded prepared-query and fast-path adapters
  debug/                   observational route summary and trace

  assembler/README.md      deferred assembler subsystem contract
  linker/README.md         deferred linker subsystem contract

  *.cpp.md / *.hpp.md      legacy evidence or design contracts
```

Root ownership rules:

- `src/backend/mir/x86/` is the target-local owner; child directories are
  direct subsystem seams, not nested under another routing layer
- `api/` is the only public x86 emission entry surface for shared backend code
- `x86.hpp` remains the single root compatibility holdout
- `assembler/` and `linker/` stay explicit subsystem contracts even while live
  implementation is deferred
- each live child directory exposes at most one non-helper `.hpp`

## Dependency Direction

- `api/` may call `module/`
- `module/` may depend on `abi/`, `core/`, `lowering/`, `prepared/`, and
  `debug/`
- `prepared/` may ask narrow questions of `lowering/`
- `debug/` may observe route state but must not become a second lowering stack
- root compatibility headers must forward only to the real owner
- VLA and dynamic `alloca` stack motion must come from `prealloc/`
  `dynamic_stack_plan`; x86 may encode it, but not rediscover it

## Public Surfaces

Approved x86 public seams for shared backend consumers:

- `api/api.hpp`
- `x86.hpp`

These seams must stay thin. New functionality should land in the owned
subdirectory first and only be re-exported here when the public surface truly
needs it.

## Markdown Policy

Key in-place markdown companions under this tree are part of the contract
surface and must declare one of two roles:

- `Legacy Evidence`
- `Design Contract`

Design-contract sections should state:

- owned inputs
- owned outputs
- invariants
- direct dependencies
- deferred behavior or upstream gaps

## Current Implementation Status

The live tree is in a contract-first state:

- compile-time seams exist across the intended subdirectories
- module emission currently returns contract-first stub bodies
- behavior recovery remains a later packet family

That is intentional. New packets should first point to a contract section, then
fill behavior behind the seam.
