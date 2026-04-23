# `variadic.cpp` extraction

## Purpose and current responsibility

This file is the x86 backend's residual variadic-ABI shim. It exposes the member functions that should own `va_start`, `va_copy`, scalar `va_arg`, and struct `va_arg` lowering, but the current implementation does not perform that work. Instead it emits placeholder assembly markers and, in one case, preserves a destination writeback/cache invalidation shape so callers can continue to flow through the backend.

The inline comment in `emit_va_arg_impl` points at the intended semantic model: SysV-style selection between register-save-area access and overflow-area memory access, with an x87-specialized path for `long double`. None of that logic exists here yet.

## Important APIs and contract surfaces

These are `X86Codegen` member functions declared in the directory index header `x86_codegen.hpp` and defined here:

```cpp
void emit_va_arg_impl(const Value& dest, const Value& va_list_ptr, IrType result_ty);
void emit_va_arg_struct_impl(const Value& dest_ptr, const Value& va_list_ptr, std::size_t size);
void emit_va_arg_struct_ex_impl(const Value& dest_ptr, const Value& va_list_ptr,
                                std::size_t size, bool from_register_area);
void emit_va_start_impl(const Value& va_list_ptr);
void emit_va_copy_impl(const Value& dest_ptr, const Value& src_ptr);
void emit_partial_copy(std::int64_t offset, std::size_t remaining);
```

Observed implementation surfaces:

```cpp
this->state.emit("    <va_arg>");
this->store_rax_to(dest);
this->state.reg_cache.invalidate_all();
```

```cpp
this->state.emit("    <va_arg-struct>");
this->state.emit("    <va_arg-struct-ex>");
this->state.emit("    <va_start>");
this->state.emit("    <va_copy>");
this->state.emit("    <partial-copy>");
```

Contract implications:

- Callers are allowed to assume these symbols exist on `X86Codegen`.
- `emit_va_arg_impl` currently preserves a post-load shape of "result is in `rax`, then stored to `dest`, then register cache invalidated", even though the actual load never happens.
- `emit_va_arg_struct_ex_impl` exposes a policy input, `from_register_area`, which suggests the real design needs a shared struct-copy path with an explicit source classification.
- `emit_partial_copy` exists as a local helper surface, implying piecewise copy logic was expected to be shared by struct variadic extraction.

## Dependency direction and hidden inputs

Direct dependencies visible in the file are minimal:

- `x86_codegen.hpp` supplies the `X86Codegen` class, `Value`, and `IrType`.
- All behavior is mediated through hidden `X86Codegen` state rather than local helpers or local data structures.

Hidden inputs carried through member state:

- `this->state.emit(...)` means the real work is assembly-text emission into backend state owned elsewhere.
- `this->store_rax_to(dest)` means the scalar path is coupled to the backend's destination materialization convention and to `rax` as the scalar return/value transport register.
- `this->state.reg_cache.invalidate_all()` means variadic extraction is treated as globally disruptive to register-cache assumptions.

Dependency direction is one-way: this file depends on the monolithic backend codegen shell and contributes no reusable ABI model of its own.

## Responsibility buckets

- Scalar variadic extraction entrypoint: `emit_va_arg_impl`
- Aggregate variadic extraction entrypoints: `emit_va_arg_struct_impl`, `emit_va_arg_struct_ex_impl`
- `va_list` lifecycle hooks: `emit_va_start_impl`, `emit_va_copy_impl`
- Copy helper surface for aggregate extraction: `emit_partial_copy`
- Placeholder compatibility emission: all current bodies

## Fast paths, compatibility paths, and overfit pressure

Core lowering that should exist but is absent:

- ABI-aware decoding of `va_list`
- register-save-area vs overflow-area selection
- type-class-sensitive handling, including the commented x87 `long double` case
- aggregate copy strategy and cursor advancement

Compatibility path currently present:

- marker-only emission strings keep backend plumbing linkable and may help debugging identify unimplemented paths
- scalar `va_arg` also preserves a legacy "write from `rax` then invalidate cache" sequence shape

Fast-path evidence:

- none implemented; the only hint is the unused `from_register_area` split in the extended struct helper

Overfit pressure:

- high risk of rebuilding around placeholder strings instead of the real variadic ABI contract
- high risk of encoding one-off type cases directly into these methods because the file name invites "just add the missing case" patches
- `emit_partial_copy` and `emit_va_arg_struct_ex_impl` could become testcase-shaped escape hatches unless the rebuild moves classification and copy policy into clearer ABI/data-movement seams

## Rebuild ownership

This file should own the thin x86 variadic lowering entrypoints and any local glue needed to connect an extracted variadic-ABI subsystem to `X86Codegen` state.

This file should not own the full variadic ABI model, ad hoc per-type special cases, broad register-classification policy, or generic aggregate-copy machinery that belongs in a clearer shared seam.
