# `call_lowering.cpp`

## Purpose and Current Responsibility

This file is the x86 backend's narrow call-lowering implementation seam. It does two different jobs:

- owns the concrete `X86Codegen` call ABI policy and the small emission steps that place call arguments and emit the `call` instruction
- keeps a compatibility holdout for prepared-route call-bundle ABI lookups that still query `lowering/` directly

The second role is explicitly transitional. The source comments mark the prepared selectors as a temporary live surface until a dedicated prepared seam exists.

## Important APIs and Contract Surfaces

The public surface is mostly "methods declared on `X86Codegen`, implemented here", plus a small free-function query API for prepared bundles.

```cpp
CallAbiConfig X86Codegen::call_abi_config_impl() const;
std::size_t X86Codegen::emit_call_compute_stack_space_impl(...) const;
std::int64_t X86Codegen::emit_call_stack_args_impl(...);
void X86Codegen::emit_call_reg_args_impl(...);
void X86Codegen::emit_call_instruction_impl(...);
void X86Codegen::set_call_ret_eightbyte_classes_impl(...);
```

```cpp
std::optional<std::string> select_prepared_call_argument_abi_register_if_supported(...);
std::optional<std::size_t> select_prepared_call_argument_abi_stack_offset_if_supported(...);
std::optional<PreparedCallResultAbiSelection> select_prepared_call_result_abi_if_supported(...);
std::optional<PreparedI32CallResultAbiSelection> select_prepared_i32_call_result_abi_if_supported(...);
```

Operational contract:

- `call_abi_config_impl()` publishes the x86 SysV-style capability flags consumed by generic call classification code elsewhere.
- `emit_call_compute_stack_space_impl()` decides only caller-side stack bytes for stack-passed args plus the alignment pad.
- `emit_call_stack_args_impl()` materializes stack arguments in reverse order and returns the alignment adjustment it applied.
- `emit_call_reg_args_impl()` moves register-classified args into ABI registers and sets `%al` to the floating-argument register count.
- `emit_call_instruction_impl()` chooses direct symbol call vs indirect `call *%rax`.
- `set_call_ret_eightbyte_classes_impl()` only records classification state; result extraction is owned elsewhere.

## Dependency Direction and Hidden Inputs

This file depends inward on shared x86 codegen state rather than owning the whole call pipeline.

- Declared through `call_lowering.hpp`, but the real owner surface is the large shared `x86_codegen.hpp` class.
- Consumes `CallArgClass`, `IrType`, `Operand`, and `EightbyteClass` decisions produced by generic call ABI classification before this file runs.
- Emits text through `this->state.emit(...)` and `state.out.emit_instr_*`, so it depends on assembler-string output conventions and register-cache invalidation rules.
- Uses helpers not defined here: `operand_to_rax`, `x86_arg_reg_name`, `x86_float_arg_reg_name`, `narrow_i32_register`, `x86_allow_struct_split_reg_stack`, and prepared-route bundle lookup utilities.
- The prepared compatibility selectors depend on `PreparedValueLocationFunction`, `find_prepared_move_bundle`, move phases (`BeforeCall`, `AfterCall`), and optional `PreparedValueHome` identity matching.

Hidden inputs worth calling out:

- `%al` float-count semantics for variadic-compatible SysV calls are encoded here, not surfaced as a separate policy object.
- Stack alignment is derived from pushed stack-argument bytes and assumes the call-site alignment contract used by the rest of x86 emission.
- Prepared result selection may prefer a move that matches `result_home->value_id`, but falls back to the first call-result ABI move/binding if identity is absent.

## Responsibility Buckets

### 1. Prepared-route compatibility selectors

- Search prepared move bundles for argument register assignment, stack offset assignment, and result ABI register selection.
- Provide block-aware and single-block overloads.
- Offer i32-specialized narrowing wrappers on top of the generic selectors.

This is not new lowering logic. It is evidence of a transitional dependency from prepared rendering back into this file.

### 2. x86 call ABI policy publication

`call_abi_config_impl()` hardcodes the backend's current call-lowering policy:

- 6 GP arg registers
- 8 FP arg registers
- SysV struct classification enabled
- no `f128` in FP regs or GP pairs
- no variadic-float-in-GP policy
- no large-struct-by-ref override
- split reg/stack aggregate passing controlled by `x86_allow_struct_split_reg_stack()`

This is a compact policy adapter, not the classifier itself.

### 3. Concrete call-site emission

- compute stack space
- push stack arguments
- move register arguments
- emit direct or indirect call
- record return eightbyte classes for later result-store logic

Notably, cleanup and result materialization are already elsewhere (`core/x86_codegen_output.cpp`), so this file owns only the front half of the call sequence.

## Fast Paths, Compatibility Paths, and Overfit Pressure

### Fast paths

- Direct-name calls bypass function-pointer materialization and emit `call <symbol>` immediately.
- Register-argument lowering is a simple linear walk over already-classified args.
- Single-block overloads for prepared selectors default `block_index` to `0` instead of forcing all callers to thread the block coordinate.

### Compatibility paths

- The prepared call-bundle selectors are the main compatibility holdout. Comments state they remain here only until a dedicated prepared seam lands.
- Result ABI lookup falls back from concrete move ownership to looser ABI bindings when no direct move match exists.
- i32 selector helpers narrow full-width ABI registers after generic selection instead of having a separate i32 ABI table.

### Overfit pressure

- The file mixes real call lowering with prepared-route query helpers. That invites rebuild drift where compatibility helpers keep accreting instead of being moved behind a prepared-specific seam.
- `%al` float-count handling is hardcoded in the emission path; future ABI variations would likely pressure this file into target-policy branching.
- Aggregate-return behavior is only partially represented here via `set_call_ret_eightbyte_classes_impl()`, while actual storage lives elsewhere. That split can hide coupling and encourage case-by-case patches.
- The prepared selectors scan both `moves` and `abi_bindings` with nearly duplicated predicates, which is a likely growth point for more compatibility-shaped branches.

## Rebuild Ownership Guidance

This file should own:

- the x86-specific front-half call emission seam
- publication of target call ABI policy needed by generic classification
- a minimal, explicit interface between generic call planning and x86 asm emission

This file should not own:

- long-lived prepared-route compatibility queries
- result-store and cleanup behavior split across unrelated files without an explicit seam contract
- target-policy branching added as special cases for individual call forms or testcase-shaped ABI exceptions
