# `return_lowering.cpp`

## Purpose And Current Responsibility

This file is the final return-value handoff for the generic x86 MIR lowering path. It does two things:

- choose how the current function result gets published into ABI-visible return locations
- sequence the shared epilogue and terminal `ret` or thunk jump

It is not a standalone lowering subsystem. It is a policy-and-ordering node that decides which already-existing value-materialization helpers to call, then closes the frame.

## Main Contract Surface

Essential surface from the implementation:

```cpp
void X86Codegen::emit_return_impl(const std::optional<Operand>& val,
                                  std::int64_t frame_size);

IrType X86Codegen::current_return_type_impl() const;
```

Hidden local helper:

```cpp
static void emit_return_epilogue_and_ret(X86Codegen& codegen,
                                         std::int64_t frame_size);
```

Operational contract:

- `emit_return_impl` assumes `current_return_type` has already been fixed for the active function.
- `val == nullopt` means "return without publishing a value", but the epilogue still must run.
- `frame_size` is treated as authoritative stack layout data for restoring callee-saved registers.
- The helper emits the actual function exit sequence and therefore must run exactly once on every non-error path.

## Dependency Direction And Hidden Inputs

This file is outward-facing only through `X86Codegen`, but it depends on a large amount of precomputed state:

- `current_return_type` drives the dispatch tree.
- `used_callee_saved` decides which registers are restored before exit.
- `state.function_return_thunk` changes the terminal instruction from `ret` to `jmp __x86_return_thunk`.
- `state.f128_direct_slots`, `state.get_slot`, `state.get_f128_source`, `state.resolve_slot_addr`, and `state.get_f128_constant_words` decide which `f128` return path is legal.
- Lowering helpers such as `emit_load_operand_impl`, `operand_to_rax_rdx`, `emit_return_*_to_reg_impl`, `emit_alloca_aligned_addr_impl`, and `emit_load_ptr_from_slot_impl` do the actual data movement.

Dependency direction is one-way: this file does not define return storage policy by itself; it consumes slot/addressing facts and calls deeper helpers that know how to load values into x87, integer return registers, or temporary pointer registers.

## Responsibility Buckets

### 1. Exit sequencing

- restore callee-saved registers from frame slots
- reset `%rsp` from `%rbp`
- pop `%rbp`
- select `ret` versus return thunk jump

### 2. Return-type dispatch

- special-case `F128`
- special-case `I128`
- otherwise load the operand and publish through scalar float or integer helpers

### 3. `F128` source-shape normalization

The `F128` block is the only materially complex branch. It accepts several storage shapes and converges them to x87 `fld` publication:

- direct frame slot
- source pointer resolving to direct / over-aligned / indirect slot address
- constant words handled through a dedicated loader

### 4. Thin introspection surface

- `current_return_type_impl()` is only a trivial read-through accessor

## Fast Paths, Compatibility Paths, And Overfit Pressure

### Fast paths

- direct `F128` slot load avoids extra address materialization
- `I128` goes through a direct `rax:rdx` route
- non-aggregate scalar returns fall through to compact "load then publish" helpers

### Compatibility paths

- `F128` supports several storage/addressing encodings, including over-aligned and indirect cases
- return thunk support indicates compatibility with hardening or target-specific return policy outside this file

### Overfit pressure

This file is a natural place for feature-by-feature return special cases to accumulate because it already branches on type and storage shape. The current `F128` ladder shows that pressure clearly:

- type-specific source-shape probing lives inline here rather than behind a narrower return-publication abstraction
- pointer/address-kind handling leaks into the return dispatcher
- epilogue ownership is coupled to value-publication branches, so every new special case must manually preserve exit ordering

The rebuild should resist adding more "if return type X and source shape Y" logic here. That would continue the current tendency to make return lowering the catch-all compatibility switchboard.

## Rebuild Ownership

This file should own:

- final return publication orchestration
- one authoritative exit-sequence hook
- minimal dispatch between already-factored return strategies

This file should not own:

- slot-shape discovery policy
- type-specific storage archaeology for each exotic return kind
- low-level address materialization details
- ad hoc compatibility branches for individual return cases beyond selecting a prepared strategy
