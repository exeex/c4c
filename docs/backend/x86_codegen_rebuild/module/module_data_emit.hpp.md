# `module/module_data_emit.hpp`

Primary role: declare the symbol, string, global, and data-section emission
services used by module orchestration.

Owned inputs:

- module-level global/string inventories
- target-specific section and symbol-formatting facts
- shared output abstractions for text assembly emission

Owned outputs:

- declarations for `.rodata`, `.data`, `.bss`, private-label, and same-module
  symbol emission helpers
- narrow data-emission result types that the module layer can consume

Allowed indirect queries:

- `core/x86_codegen_output.hpp`
- `abi/x86_target_abi.hpp`
- `core/x86_codegen_types.hpp`

Forbidden knowledge:

- route selection among prepared fast paths
- function/block-level lowering semantics
- debug tracing and route-summary ownership

Role classification:

- `module`

Drafting notes:

- this header exists specifically so stage 3 does not leave data emission
  fused inside `module_emit`
- the services here may format and publish symbols, but they must not decide
  which function-lowering path is chosen
