# `module/module_data_emit.cpp`

Primary role: implement the module-level data and symbol rendering services
that are currently fused into `prepared_module_emit.cpp`.

Owned inputs:

- global definitions, string constants, and same-module symbol inventories
- target-specific label and section policy from `abi`
- output-buffer services from `core`

Owned outputs:

- `.rodata`, `.data`, and `.bss` text emission
- private-label and same-module symbol formatting
- reusable data-emission helpers that `module_emit.cpp` can call without
  inheriting all rendering details

Allowed indirect queries:

- `abi/x86_target_abi.hpp` for symbol naming and section policy
- `core/x86_codegen_output.hpp` for textual publication
- no direct queries into prepared fast-path matcher state

Forbidden knowledge:

- function-route selection
- call, return, frame, memory, scalar, float, or comparison semantics
- prepared-query context breadth beyond the minimal module-level names needed
  for symbol publication

Role classification:

- `module`

Drafting notes:

- this file is a hard seam that prevents module dispatch from owning concrete
  data rendering
- if a future fast path needs symbol helpers, it must consume this file's
  narrow services instead of reopening the old `prepared_module_emit.cpp`
  fusion
