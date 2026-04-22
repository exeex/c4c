# `abi/x86_target_abi.hpp`

Primary role: declare the stable target-profile, register, and calling-policy
facts that the replacement subsystem shares.

Owned inputs:

- target triples and x86 target-profile selections
- stable machine-policy concepts such as register classes, stack alignment,
  caller/callee-saved sets, and ABI lane classifications

Owned outputs:

- declarations for register-name formatting, ABI-profile lookup, and reusable
  calling-policy helpers
- narrow ABI-facing types that other headers can depend on without pulling in
  module or prepared concerns

Allowed indirect queries:

- `core/x86_codegen_types.hpp` when shared type aliases are needed at the ABI
  boundary

Forbidden knowledge:

- public API orchestration
- module-data emission ownership
- prepared-route dispatch context breadth
- route-debug summaries or tracing helpers

Role classification:

- `abi`

Drafting notes:

- preserve the stable machine-policy value of `mod.cpp`
- separate machine facts from higher-level orchestration helpers that do not
  belong in the ABI contract
