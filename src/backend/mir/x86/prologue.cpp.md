# `prologue.cpp` extraction

## Purpose and current responsibility

This file is the x86 function-entry / function-exit bridge between MIR function shape and concrete SysV-ish frame setup. It does three jobs at once:

- computes frame size and prologue-dependent state before emission
- emits prologue / epilogue assembly, including stack probing and variadic save-area setup
- classifies incoming parameters and decides whether to pre-store them, leave them in ABI locations, or materialize scalar references later

The file is not just "prologue". It is also a parameter-ingress policy layer and a partial ABI lowering adapter.

## Important APIs and contract surfaces

Primary entry points owned by `X86Codegen`:

```cpp
std::int64_t X86Codegen::calculate_stack_space_impl(const IrFunction& func);
void X86Codegen::emit_prologue_impl(const IrFunction& func, std::int64_t frame_size);
void X86Codegen::emit_epilogue_impl(std::int64_t frame_size);
void X86Codegen::emit_store_params_impl(const IrFunction& func);
void X86Codegen::emit_param_ref_impl(const Value& dest, std::size_t param_idx, IrType ty);
void X86Codegen::emit_epilogue_and_ret_impl(std::int64_t frame_size);
```

Support surface hidden in unnamed-namespace helpers:

```cpp
std::string_view scalar_param_ref_type_name(IrType ty);
std::optional<StackSlot> lookup_param_slot(X86Codegen&, std::string_view param_name);
void emit_struct_*_param_store(...);
```

Operational contract:

- `calculate_stack_space_impl` must classify params, seed x86-specific bookkeeping, run regalloc integration, account for variadic register-save area, and include callee-saved spill space.
- `emit_prologue_impl` assumes that frame-size and callee-saved decisions are already fixed.
- `emit_store_params_impl` assumes regalloc results already exist in `reg_assignments`.
- `emit_param_ref_impl` is a fallback scalar materialization path for params not marked as pre-stored.

## Dependency direction and hidden inputs

Visible includes are light, but the hidden dependency surface is broad through `x86_codegen.hpp`:

- ABI classification helpers: `classify_params_full`, `named_params_stack_bytes`
- register policy: `x86_callee_saved_regs`, `x86_prune_caller_saved_regs`, `run_regalloc_and_merge_clobbers`
- frame-layout helpers: `calculate_stack_space_common`, `x86_aligned_frame_size`, `x86_callee_saved_slot_offset`
- variadic ABI helpers: `x86_variadic_*`
- parameter move/load/store rendering helpers: `x86_param_*`
- output side effects through `state.emit(...)` and `state.out.emit_*`

Hidden mutable inputs on `X86Codegen` / `state`:

- `is_variadic`, `no_sse`
- `num_named_int_params`, `num_named_fp_params`, `num_named_stack_bytes`
- `reg_assignments`, `used_callee_saved`, `reg_save_area_offset`
- `current_return_type`, `func_ret_classes`
- `state.param_classes`, `state.param_pre_stored`
- `state.cf_protection_branch`, `state.function_return_thunk`

Important directional fact: this file does not own ABI knowledge itself. It orchestrates many `x86_param_*`, frame, and regalloc queries, then emits assembly. The actual policy is distributed and largely stringly/offset-helper driven.

## Responsibility buckets

### 1. Frame planning and regalloc coupling

- scans IR for indirect calls, `i128` usage, and atomic RMW
- prunes caller-saved register set based on those features
- runs regalloc integration
- computes raw stack usage through a generic allocator callback
- extends frame for variadic register-save area and callee-saved preservation

This is prologue planning, but it is tightly coupled to regalloc and instruction-feature discovery.

### 2. Concrete entry / exit emission

- emits `endbr64` when control-flow protection is enabled
- emits canonical `pushq %rbp` / `movq %rsp, %rbp`
- subtracts frame size directly or through a page-probing loop for large frames
- spills callee-saved registers into frame slots
- saves GP/SSE incoming arg registers for variadic functions
- restores callee-saved registers and emits `ret` or thunk jump

### 3. Parameter ingress classification

- re-runs parameter classification in `emit_store_params_impl`
- caches per-parameter classes in `state.param_classes`
- counts reuse of register-assigned params to decide whether direct pre-store is safe
- handles several aggregate layouts through special helper families

### 4. Deferred scalar param materialization

- if a param was not pre-stored, `emit_param_ref_impl` loads/moves it from ABI location
- supports integer, float, and stack scalar cases
- writes through `store_rax_to(dest)`, so scalar param references are forced through a chosen destination convention

## Notable fast paths, compatibility paths, and overfit pressure

### Fast paths

- direct pre-store of scalar register params into assigned regs when `x86_param_can_prestore_direct_to_reg(...)` allows it
- aggregate copy helpers skip missing source regs and only emit required qword moves
- direct frame subtraction for non-probed frames

### Compatibility / transitional paths

- `lookup_param_slot(...)` currently returns `nullopt` unconditionally after documenting that transitional state is keyed by value id rather than slot name
- many helper functions are marked `[[maybe_unused]]`, which suggests retained compatibility scaffolding or partially wired aggregate paths
- variadic save-area emission is guarded by `no_sse`, carrying ABI compatibility branching into the prologue file
- return emission supports thunk-based return hardening via `function_return_thunk`

### Overfit pressure

- aggregate param handling is expressed as a growing list of layout-shaped cases:
  `StructByValReg`, `StructSseReg`, `StructMixedIntSseReg`, `StructMixedSseIntReg`, `StructStack`, `LargeStructStack`
- scalar param reference logic is duplicated by storage class rather than routed through a smaller normalized ingress model
- the file mixes feature detection (`i128`, indirect calls, atomic RMW), frame policy, ABI ingress, and assembly rendering, which makes "one more case" additions cheap and structural cleanup expensive
- the null `lookup_param_slot` path means some store helpers exist without a live end-to-end contract, increasing pressure for local tactical fixes

## Rebuild guidance

This file should own:

- orchestration of function-entry / function-exit emission once frame and ingress decisions are already expressed in stable data
- minimal bridging from precomputed frame/ABI decisions to assembly output

This file should not own:

- regalloc feature scanning and caller-saved pruning policy
- detailed parameter classification matrices
- aggregate-layout-specific ingress case expansion
- transitional slot lookup policy or value-id/slot-name reconciliation
- mixed concerns of frame planning, ABI semantics, and final text emission in one implementation unit
