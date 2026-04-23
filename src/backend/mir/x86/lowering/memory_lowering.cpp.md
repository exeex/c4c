# `memory_lowering.cpp` contract

## Legacy Evidence

The extraction below records how the old file mixed operand formatting,
prepared-home archaeology, carrier-state policy, and shape-specific mini
lowering.

## Purpose and current responsibility

This file is the x86 MIR lowering helper layer for turning prepared memory/addressing facts plus a few local codegen state trackers into concrete assembly fragments. It is not one algorithm; it is a grab bag of operand renderers, address resolvers, load/store helpers, and a specialized bounded-call lane renderer.

In practice it sits between:

- prepared analysis data (`PreparedAddressingFunction`, `PreparedValueLocationFunction`, `PreparedNameTables`, `PreparedStackLayout`)
- local frame layout data (`PreparedModuleLocalSlotLayout`)
- x86 string emission conventions (`mov`, `lea`, `movss`, `movsd`, RIP-relative symbol syntax)

The file’s output contract is mostly "return `std::optional<...>` with preformatted asm snippets if this narrow shape is supported; otherwise return `nullopt`."

## Important APIs and contract surfaces

The public surface exported by the sibling header is broad and utility-shaped rather than cohesive:

```cpp
std::optional<std::string_view> prepared_scalar_memory_operand_size_name(
    c4c::backend::bir::TypeKind type);

std::optional<PreparedScalarMemoryAccessRender>
render_prepared_compatible_scalar_memory_operand_for_inst_if_supported(...);

std::optional<PreparedScalarMemoryAccessRender>
render_prepared_pointer_value_memory_operand_if_supported(...);

std::optional<PreparedPointerParamMemoryAccessRender>
render_prepared_pointer_param_scalar_memory_operand_for_inst_if_supported(...);
```

These helpers classify supported memory bases and return either:

- plain memory operands like `DWORD PTR [rsp + N]`
- setup asm plus operand when a scratch register or `lea` is needed
- pointer-param specific metadata (`pointer_name`, ABI register, operand)

The file also exports value-home driven source selection for integer lowering:

```cpp
struct PreparedNamedI32Source {
  std::optional<std::string> register_name;
  std::optional<std::string> stack_operand;
  std::optional<std::int64_t> immediate_i32;
};

std::optional<PreparedNamedI32Source> select_prepared_named_i32_source_if_supported(...);
std::optional<std::string> render_prepared_i32_operand_from_source_if_supported(...);
bool append_prepared_named_i32_move_into_register_if_supported(...);
```

This establishes an internal contract used elsewhere in lowering: an i32 value is represented as exactly one of register, stack operand, or immediate, and the helpers know how to move/sync that source into canonical x86 registers or homes.

There is also one conspicuously larger contract surface:

```cpp
std::optional<PreparedBoundedMultiDefinedCallLaneRender>
render_prepared_bounded_multi_defined_call_lane_body_if_supported(...);
```

That routine consumes a very constrained BIR function shape and emits the full body of a bounded multi-defined call lane, including prologue-like stack reservation, argument materialization, calls, result finalization, local loads/stores, and return lowering.

## Dependency direction and hidden inputs

The obvious dependencies are the prepared-analysis inputs and frame layout, but the real behavior depends on several hidden or implicit inputs:

- Current carrier state:
  `current_i32_name`, `previous_i32_name`, `current_i8_name`, and `current_ptr_name` affect whether the file reuses `eax`, `ecx`, `al`, or `rax` instead of reloading.
- Reserved scratch register policy:
  pointer paths probe a hardcoded list (`r11`, `r10`, `r9`, `r8`); float paths prefer hardcoded `xmm15`.
- Value-home graph shape:
  pointer lowering recursively walks `PointerBasePlusOffset` homes and uses cycle guards. This means alias-family structure in prepared value homes directly changes emitted asm.
- Authoritative memory-use detection:
  some pointer decisions depend on whether a value appears in prepared memory accesses, not only on local instruction shape.
- Symbol-name rendering callbacks:
  global-symbol memory addressing is only available if callers supply renderers and prepared names resolve to link names.
- Stack/frame conventions:
  many renderers assume stack accesses are `rsp`-relative with non-negative computed offsets.

Dependency flow is therefore mostly one-way from prepared analysis into string emission, but the file also mixes in transient register-allocation knowledge that a cleaner lowering layer would likely isolate.

## Responsibility buckets

### 1. Scalar memory operand rendering

- map BIR scalar type to x86 size token
- render frame-slot memory operands
- render RIP-relative global-symbol operands
- render stack-address expressions
- combine these into an instruction-oriented "compatible scalar memory operand" helper

### 2. Pointer-address materialization

- find a named pointer home
- recursively chase pointer-base-plus-offset chains
- decide whether current `rax` can be reused for the same pointer family
- otherwise select a scratch GPR and emit `mov`/`add`/`sub` or `lea`
- render `[base +/- delta]` memory operands from that materialized pointer

### 3. Pointer-parameter compatibility path

- detect when a prepared memory access is based on a pointer-valued parameter
- ask the caller for the ABI register that carries that parameter
- render direct `[abi_reg +/- delta]` memory operands without extra setup

### 4. Stack/local slot helpers

- render local slot memory operands using `PreparedModuleLocalSlotLayout`
- render stack addresses for named values or payloads
- fall back across several authoritative/home lookup helpers

### 5. Integer source selection and home synchronization

- represent an i32 source as register/stack/immediate
- inspect current and previous carriers before consulting prepared homes
- look backward through a block for recent `LoadLocalInst` / `StoreLocalInst`
- emit moves into registers, memory homes, and canonical carrier registers
- publish load/result state by updating current/previous carrier assumptions

### 6. Narrow float/qword load helpers

- load qword values either into a register home, stack home, or scratch register
- load `F32`/`F64` via `movss`/`movsd`
- optionally spill the loaded float back to its stack home

### 7. Specialized bounded-call lane lowering

- match a narrow single-block function shape
- lower calls, local stores, local loads, and final return
- rely on many sibling helpers for argument/result handling
- reject unsupported shapes by returning `nullopt`

This last bucket is materially different from the others: it is mini-lowering orchestration, not just operand rendering.

## Notable fast paths, compatibility paths, and overfit pressure

### Fast paths

- Reuse `rax` when the current pointer carrier already belongs to the same pointer family.
- Reuse `eax` / `ecx` when current or previous i32 carriers already hold the desired value.
- Use direct parameter ABI registers for pointer-based memory operands when lookup succeeds.
- Skip home-sync writes when the destination home already matches the current carrier or source memory.

### Compatibility paths

- Support both authoritative stack offsets and weaker value-home/frame-offset fallbacks.
- Accept global symbol accesses only when prepared names resolve and a symbol renderer callback exists.
- Accept rematerializable i32 immediates as value homes.
- Permit recursive pointer-base-plus-offset rematerialization instead of requiring a fully materialized address input.

### Overfit pressure

- Hardcoded scratch register choices (`r11`...`r8`, `xmm15`) are tactical, not abstract resource ownership.
- Many helpers encode knowledge of canonical carriers (`eax`, `ecx`, `al`, `rax`) directly into operand selection.
- `render_prepared_bounded_multi_defined_call_lane_body_if_supported` is a shape-specific orchestration path living beside general-purpose memory helpers.
- Several routines search backward in the current block for matching local load/store patterns, which couples correctness to nearby instruction shapes rather than a clearer dataflow abstraction.
- The dominant failure mode is silent `nullopt` for unsupported structure; callers must know many narrow preconditions that are not centralized in one contract.

## Design Contract

Current rebuild role:

- own x86 memory-operand formatting from already-decided addressing facts
- publish small helpers for stack-operand spelling and scalar size tokens
- stay below module orchestration and above raw string concatenation

Owned inputs:

- BIR scalar type kinds
- stack-byte offsets
- later packets may add prepared addressing facts, but only through explicit
  APIs

Owned outputs:

- x86 size tokens such as `BYTE PTR`, `DWORD PTR`, and `QWORD PTR`
- concrete stack memory operands

Must not own:

- hidden register-allocation policy
- pointer-family archaeology across prepared value homes
- bounded call-lane body synthesis
- backward pattern matching over blocks to recover semantic facts

Deferred gaps:

- richer prepared-address lowering should be reintroduced only behind explicit
  operand-render contracts

## Rebuild ownership boundary

A rebuild should let this area own:

- x86 memory/address operand formatting from already-decided addressing facts
- small, explicit source-selection helpers with documented carrier inputs
- isolated conversion of prepared address forms into operand/render records

A rebuild should not let this area own:

- ad hoc register allocation policy
- block-pattern archaeology for recovering integer sources
- full bounded-call-lane body synthesis
- mixed stack-layout, pointer-family, symbol-resolution, and result-publication policy in one file
