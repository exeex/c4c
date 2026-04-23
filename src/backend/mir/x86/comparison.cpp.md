# `comparison.cpp` extraction

## Purpose and current responsibility

This file no longer implements x86 comparison lowering. Its current job is to
exist as an explicit compatibility marker at the legacy top-level path so the
former `comparison.cpp` bucket does not silently disappear while ownership has
already moved into `lowering/comparison_lowering.cpp`.

The file is evidence of a transition, not an active lowering unit.

```cpp
#include "lowering/comparison_lowering.hpp"

namespace c4c::backend::x86 {
// Dormant compatibility surface...
}
```

## Important APIs and contract surfaces

The local file exposes no independent API. Its only contract surface is the
dependency on the canonical comparison-lowering seam under `lowering/`.

The owning behavior remains attached to `X86Codegen` methods declared on the
shared x86 codegen surface and implemented in the lowering unit:

```cpp
void X86Codegen::emit_float_cmp_impl(...);
void X86Codegen::emit_f128_cmp_impl(...);
void X86Codegen::emit_int_cmp_impl(...);
void X86Codegen::emit_fused_cmp_branch_impl(...);
void X86Codegen::emit_fused_cmp_branch_blocks_impl(...);
void X86Codegen::emit_cond_branch_blocks_impl(...);
void X86Codegen::emit_select_impl(...);
```

Practical contract: callers should treat this path as non-owning and follow the
shared `X86Codegen` surface into `lowering/comparison_lowering.cpp` for real
compare, branch, and select behavior.

## Dependency direction and hidden inputs

- `comparison.cpp` depends downward on `lowering/comparison_lowering.hpp`.
- The real behavior depends on the transitional shared `X86Codegen` surface,
  not on symbols defined here.
- Hidden inputs live in the owning lowering code: `IrCmpOp`, `IrType`,
  `Operand`, `Value`, `BlockId`, register-cache invalidation rules, operand
  materialization helpers, x87/SSE selection, and assembler emission through
  `state.emit(...)`.

This means the legacy path carries naming continuity only; it does not own the
semantic inputs that actually decide lowering shape.

## Responsibility buckets

- Compatibility marker: preserve a visible top-level file for the old compare
  bucket during the transition.
- Ownership redirect: signal that compare/branch/select lowering moved under
  `lowering/comparison_lowering.cpp`.
- Non-responsibility: no instruction selection, no condition-code mapping, no
  register movement, no branch/select emission.

## Fast paths, compatibility paths, and overfit pressure

- Compatibility path: the entire file is a compatibility path. It keeps the
  old location present while redirecting ownership elsewhere.
- Fast paths: none in this file.
- Overfit pressure: low locally, but high if future work starts reintroducing
  ad hoc compare logic here "because the file already exists." That would
  recreate split ownership between the legacy top-level bucket and the newer
  lowering seam.

## Rebuild ownership

This file should own only the historical note that the legacy compare bucket is
non-owning, or disappear entirely once the rebuild no longer needs that marker.

This file should not own comparison lowering logic, compare-op to condition-code
tables, branch/select emission, float-vs-int compare routing, or any new
special-case backend behavior.
