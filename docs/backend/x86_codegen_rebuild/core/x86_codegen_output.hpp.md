# `core/x86_codegen_output.hpp`

Primary role: declare the shared output-buffer and assembly-text helper seam
used by module and lowering owners.

Owned inputs:

- the shared output/state types defined in `x86_codegen_types.hpp`
- line-oriented assembly emission needs that are common across lowering
  families

Owned outputs:

- declarations for append/section/symbol helper surfaces that lowerers and
  module emitters may call
- a narrow output abstraction that replaces scattered string-plumbing helpers
  from `shared_call_support.cpp` and related files

Allowed indirect queries:

- `core/x86_codegen_types.hpp`
- `abi/x86_target_abi.hpp` only where output helpers need target-specific text
  formatting contracts

Forbidden knowledge:

- public API entrypoints
- prepared fast-path routing choices
- call, return, memory, comparison, or frame semantics
- debug tracing and route-summary ownership

Role classification:

- `core`

Drafting notes:

- this header should publish reusable output services, not another bag of
  unrelated helpers
- helper declarations here must remain generic enough for canonical lowering
  and module emission to share them
