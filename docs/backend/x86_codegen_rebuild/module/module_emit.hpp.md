# `module/module_emit.hpp`

Primary role: declare the top-level x86 module-orchestration seam that turns a
prepared or normalized module into final assembly text.

Owned inputs:

- prepared modules and module-level routing facts from the public API layer
- shared output/state abstractions from `core`
- target and symbol-policy facts from `abi`

Owned outputs:

- declarations for the canonical module-emission entrypoints
- narrow module-level result/status types that describe whole-module emission
  without exposing prepared fast-path internals

Allowed indirect queries:

- `core/x86_codegen_types.hpp`
- `core/x86_codegen_output.hpp`
- `abi/x86_target_abi.hpp`
- `module/module_data_emit.hpp` for data/symbol emission delegation

Forbidden knowledge:

- no direct ownership of `.rodata` / `.data` / `.bss` rendering details
- no broad prepared-query context declaration in this header
- no frame, call, memory, comparison, or return semantics

Role classification:

- `module`

Drafting notes:

- this header replaces the module-facing subset of `prepared_module_emit.cpp`
  without fusing it to data emission
- the public entry stays module-shaped; specialized fast paths remain behind
  later prepared-layer seams
