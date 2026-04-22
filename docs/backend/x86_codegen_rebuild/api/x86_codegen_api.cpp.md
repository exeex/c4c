# `api/x86_codegen_api.cpp`

Primary role: implement the public x86 entrypoints by normalizing caller input
and handing control to the module layer.

Owned inputs:

- BIR modules that need target resolution before preparation
- LIR modules that need the existing direct-LIR entry workflow
- prepared modules that are ready for x86 module emission
- user-facing output-path or assembly options passed through the API surface

Owned outputs:

- the final public `std::string` assembly result or assemble result object
- normalized entry handoff into `module/module_emit.cpp`
- public-boundary errors when callers request unsupported targets or malformed
  entry conditions

Allowed indirect queries:

- semantic-module preparation helpers used today by `emit.cpp`
- `module/module_emit.hpp` as the sole internal owner for top-level x86 module
  orchestration
- `abi/x86_target_abi.hpp` for direct target-profile validation when the API
  must reject a request before deeper work starts

Forbidden knowledge:

- no direct inspection of prepared stack layout, prepared addressing, or block
  routing tables
- no direct emission of `.rodata`, `.data`, or `.bss`
- no local copies of canonical lowering logic for calls, memory, returns, or
  comparisons

Role classification:

- `api`

Dependency direction:

1. accept caller-visible module input
2. normalize target and preparation state
3. hand off to `module/module_emit.cpp`
4. return the assembled textual result without exposing internal routing state
