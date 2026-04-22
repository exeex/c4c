# `module/module_emit.cpp`

Primary role: orchestrate whole-module x86 emission, including top-level
prepared route selection, while delegating data rendering and semantic
lowering to narrower owners.

Owned inputs:

- prepared module metadata and module-level function inventories
- target validation results from the API or ABI layer
- shared output and symbol-publication services from `core`
- data-emission services from `module_data_emit.cpp`

Owned outputs:

- ordered whole-module assembly text
- entry-function selection and module-level routing decisions
- delegation points into canonical lowering families or bounded prepared
  adapters

Allowed indirect queries:

- `module/module_data_emit.hpp` for string/global/data section work
- `abi/x86_target_abi.hpp` for target validation and module-wide ABI facts
- `core/x86_codegen_output.hpp` for text assembly output
- future prepared adapters through narrow prepared-layer headers only
- future canonical lowering headers for function-family lowering entrypoints

Forbidden knowledge:

- no concrete `.rodata` / `.data` / `.bss` formatting bodies
- no ownership of frame-home lookup, call-lane ordering, memory operand
  formation, or comparison shaping
- no debug-trace rendering logic beyond calling the debug surface

Role classification:

- `module`

Dependency direction:

1. validate the module-level route and choose the entry flow
2. delegate data/symbol emission to `module_data_emit.cpp`
3. delegate function emission to canonical lowering or bounded prepared
   adapters
4. return the final module text without becoming a hidden lowering owner
