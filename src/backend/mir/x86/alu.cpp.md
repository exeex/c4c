# `alu.cpp` extraction

## Purpose and current responsibility

This file is no longer a general ALU lowering implementation. It is a tiny transitional compatibility surface that keeps one `i128` copy helper on `X86Codegen` while the canonical float-lowering seam has already moved into `lowering/float_lowering.cpp`.

The file's present job is:

- preserve one externally expected method definition at the old path
- bridge a wide source operand into the `RAX:RDX` pair
- store that pair into the destination value

## Important APIs and contract surfaces

Essential surface:

```cpp
void X86Codegen::emit_copy_i128_impl(const Value& dest, const Operand& src);
```

Operational contract:

- `src` must be lowerable through `operand_to_rax_rdx`
- the copy path uses `RAX:RDX` as the implicit transport register pair
- `dest` must be storable through `store_rax_rdx_to`
- this function is sequencing glue; the real storage/load rules live in helper methods on `X86Codegen`

Implementation shape:

```cpp
this->operand_to_rax_rdx(src);
this->store_rax_rdx_to(dest);
```

## Dependency direction and hidden inputs

Visible dependency direction:

- includes `lowering/float_lowering.hpp`
- relies on `X86Codegen`, `Value`, and `Operand` declarations that come through that header's transitively included shared codegen surface

Hidden inputs:

- register-pair convention: `RAX:RDX` is hardwired rather than passed explicitly
- helper semantics: correctness depends on `operand_to_rax_rdx` and `store_rax_rdx_to`
- backend state on `X86Codegen`: any live register allocation, scratch policy, or emission context is consumed implicitly through `this`

This means the file looks standalone but actually depends on the broader x86 codegen object contract.

## Responsibility buckets

- compatibility holdout: keep the old method definition alive during lowering migration
- wide-copy glue: route one `i128` copy through shared register-pair helpers
- no policy ownership: does not decide instruction selection, register policy, or float-lowering behavior

## Fast paths, compatibility paths, and overfit pressure

- Core lowering: none; this file is not the canonical owner anymore.
- Optional fast path: none visible here.
- Legacy compatibility: the whole file exists for this reason.
- Overfit pressure: high naming drift. The path/name suggests broad ALU ownership, but the implementation is only a narrow `i128` holdout. That makes it easy to keep parking unrelated leftovers here instead of moving them to their real owners.

## Rebuild ownership

This file should own nothing beyond a temporary compatibility shim for the remaining `i128` copy seam.

It should not own general ALU lowering, float lowering, register policy, or new special cases added just because the historical filename is convenient.
