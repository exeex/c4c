# `calls.cpp` extraction

## Purpose and current responsibility

This file no longer implements x86 call lowering. Its live responsibility is
only to preserve the historical translation-unit name while redirecting callers
to the canonical lowering surface under `lowering/`.

Evidence:

```cpp
#include "lowering/call_lowering.hpp"
```

The comment in the file makes the ownership transfer explicit: canonical call
lowering now lives in `lowering/call_lowering.cpp`.

## Important APIs and contract surfaces

`calls.cpp` does not define any functions, classes, or data. Its only contract
surface is the include dependency on the new lowering seam.

Relevant downstream surface now owned elsewhere:

```cpp
namespace c4c::backend::x86 {
// Canonical x86 call-lowering seam. The owning method declarations remain on
// X86Codegen in the transitional shared surface for now.
}
```

Observed call-lowering ownership in the replacement implementation includes:

```cpp
CallAbiConfig X86Codegen::call_abi_config_impl() const;
std::size_t X86Codegen::emit_call_compute_stack_space_impl(...) const;
std::int64_t X86Codegen::emit_call_stack_args_impl(...);
```

This means the old filename is no longer the behavioral API entry point; the
real contract is the `X86Codegen` call-lowering methods and prepared-call ABI
query helpers in `lowering/call_lowering.cpp`.

## Dependency direction and hidden inputs

Direction is now one-way:

- `calls.cpp` depends on `lowering/call_lowering.hpp`.
- Real lowering logic depends on `x86_codegen.hpp`, ABI-query helpers, backend
  prepare-layer move bundles, register narrowing helpers, and SysV policy
  switches.

Hidden inputs that matter to the real behavior but are not visible from this
stub:

- prepared move-bundle contents before and after the call
- block/instruction indexing used to query those bundles
- target ABI policy encoded in `CallAbiConfig`
- stack-alignment assumptions (`%rsp` 16-byte alignment)
- transitional ownership still hanging off `X86Codegen`

## Responsibility buckets

- Compatibility placeholder: keep the old path buildable during migration.
- Ownership redirect: signal that call lowering moved into `lowering/`.
- Non-owner: does not choose ABI classes, stack layout, argument placement, or
  return-value handling.

## Fast paths, compatibility paths, and overfit pressure

- Core lowering: none in this file.
- Optional fast path: none in this file.
- Legacy compatibility: the whole file is a compatibility path. It exists to
  avoid reviving the old `calls.cpp` implementation surface.
- Overfit pressure: high risk if future fixes start landing here because the
  filename still looks like the natural home for x86 call work. Any new logic
  added here would reintroduce split ownership and weaken the rebuild seam.

## Rebuild ownership

This file should own only a temporary compatibility redirect, or disappear once
all build references stop needing the legacy translation unit name.

It should not own call ABI policy, prepared-call ABI queries, stack-argument
emission, result handling, or any new x86 call special cases.
