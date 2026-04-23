# `float_ops.cpp` extraction

## Purpose and current responsibility

This file is no longer an implementation unit for x86 floating-point lowering. Its current role is a dormant compatibility shell that:

- includes the canonical float-lowering seam
- preserves the historical filename as an empty translation unit
- documents, in code, that real lowering moved to `lowering/float_lowering.cpp`

The executable evidence here is that `float_ops.cpp` intentionally owns no lowering logic anymore.

## Important APIs and contract surfaces

This file exposes no local functions, types, or registration hooks. Its only meaningful surface is the dependency it forwards to:

```cpp
#include "lowering/float_lowering.hpp"
```

The adjacent header comment says the owning method declarations still live on `X86Codegen` via the shared transitional surface, so this file acts as a compatibility waypoint rather than a contract owner.

## Dependency direction and hidden inputs

Dependency direction is one-way:

- `float_ops.cpp` depends on `lowering/float_lowering.hpp`
- the real behavioral dependency is the implementation behind that seam in `lowering/float_lowering.cpp`

Hidden inputs are organizational rather than algorithmic:

- build graph expectations may still reference `float_ops.cpp`
- developers may infer responsibility from the legacy filename even though ownership moved
- the transitional `X86Codegen` surface remains the effective declaration carrier

## Responsibility buckets

- Compatibility shell: keeps the legacy file path present without duplicating code
- Ownership marker: states that canonical float lowering moved elsewhere
- Non-owner: does not define float ops, selection rules, emission helpers, or ABI behavior

## Fast paths, compatibility paths, and overfit pressure

- Core lowering: not owned here
- Optional fast paths: none in this file
- Legacy compatibility: the whole file is a compatibility path for source/build layout continuity
- Overfit pressure: high risk of mistaken ownership, where future patches get reintroduced here because the filename looks like the natural home for float work

## Rebuild ownership

This file should own, at most, an explicit compatibility marker or nothing at all if the legacy path is retired. It should not own float lowering behavior, x86 float opcode selection, transitional `X86Codegen` method bodies, or new special-case float logic in a rebuild.
