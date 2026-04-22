# `core/x86_codegen_output.cpp`

Primary role: implement shared assembly-text output services and low-level
state plumbing that the larger lowering files consume.

Owned inputs:

- output handles and shared state from `x86_codegen_types.hpp`
- emission requests from `module/module_emit.cpp`, `module/module_data_emit.cpp`,
  and the canonical lowering families

Owned outputs:

- concrete line append, section switch, label publication, and small
  formatting helpers
- output-state behavior that today is spread across
  `shared_call_support.cpp` and adjacent helper code

Allowed indirect queries:

- `abi/x86_target_abi.hpp` for register and section formatting facts
- no direct queries into prepared-route matcher internals

Forbidden knowledge:

- no route dispatch policy
- no prepared module selection or fast-path eligibility decisions
- no ownership of ABI classification, stack layout, or call/return lowering

Role classification:

- `core`

Dependency direction:

1. receive generic emission requests
2. format and append textual output
3. return control to the calling owner without deciding semantic lowering
