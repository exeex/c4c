# `atomics.cpp` extraction

## Purpose and current responsibility

This file is no longer the implementation home for x86 atomic and intrinsic lowering. Its current job is to keep the legacy translation-unit path alive while pulling in the canonical lowering surface from `lowering/atomics_intrinsics_lowering.hpp`.

The source is effectively a compatibility shell:

```cpp
#include "lowering/atomics_intrinsics_lowering.hpp"

namespace c4c::backend::x86 {
// canonical lowering now lives elsewhere
}
```

The important evidence is not behavior inside this file, but the ownership move it documents: atomics/intrinsics lowering was extracted out of this legacy path.

## Important APIs and contract surfaces

This file exports no functions of its own. The contract surface it keeps reachable is the shared x86 lowering seam declared in the included header.

Essential surfaces:

```cpp
enum class AtomicOrdering : unsigned;
enum class AtomicRmwOp : unsigned;
enum class IntrinsicOp : std::uint16_t;
```

```cpp
void emit_atomic_rmw_impl(..., AtomicRmwOp op, ..., AtomicOrdering ordering);
void emit_atomic_cmpxchg_impl(..., AtomicOrdering success, AtomicOrdering failure, ...);
void emit_atomic_load_impl(..., AtomicOrdering ordering);
void emit_atomic_store_impl(..., AtomicOrdering ordering);
void emit_fence_impl(AtomicOrdering ordering);
void emit_intrinsic_impl(const std::optional<Value>& dest, const IntrinsicOp& intrinsic, ...);
```

Contract implication: callers should treat the header plus the `X86Codegen` member declarations as the real seam. This `.cpp` is only the residual TU anchor.

## Dependency direction and hidden inputs

Dependency direction is one-way:

- `atomics.cpp` depends on `lowering/atomics_intrinsics_lowering.hpp`
- that header depends on `x86_codegen.hpp`
- `x86_codegen.hpp` carries the `X86Codegen` member declarations that use the atomics/intrinsics enums

Hidden input pressure is outside this file:

- `X86Codegen` state and emitter context
- x86 instruction-selection and fence semantics
- BIR/LIR value and operand conventions threaded through `X86Codegen`
- the large intrinsic enum inventory, which is effectively a capability registry

This file does not declare those dependencies directly, but it still participates in the include graph that exposes them.

## Responsibility buckets

- Compatibility bucket: preserve the old `atomics.cpp` path while canonical code lives under `lowering/`
- Namespace bucket: keep the legacy namespace/translation-unit identity valid
- Zero-logic bucket: intentionally contain no lowering rules, dispatch, or instruction emission

## Fast paths, compatibility paths, and overfit pressure

- Core lowering: none here
- Optional fast path: none here
- Legacy compatibility: the entire file is one compatibility path
- Overfit pressure: high risk if future fixes are reintroduced here as one-off shims instead of extending the canonical lowering implementation

The main rebuild hazard is ownership drift. Because this filename still suggests semantic ownership, it invites tactical fixes that bypass the real lowering seam and recreate split-brain behavior.

## Rebuild ownership

This file should own only legacy compatibility while the old path exists, or disappear entirely once teardown is accepted.

It should not own:

- atomic ordering semantics
- intrinsic opcode inventories beyond re-export/include exposure
- `X86Codegen` lowering behavior
- special-case fast paths or testcase-shaped compatibility logic
