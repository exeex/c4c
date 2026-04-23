# `prepared_fast_path_operands.cpp` extraction

## Purpose and current responsibility

This file is a string-emitting operand helper layer for the x86 prepared-codegen fast paths. It does not own whole instruction lowering; it owns the small decisions needed to turn already-prepared value-placement facts into concrete operand text for a few narrow call/return families.

Its main job is to answer questions like:

- where does a named value currently live: register, stack slot, rematerializable immediate, or ABI stack parameter?
- can that value be moved directly into the register or memory shape required by a fast path?
- after a fast-path call, can the result be tracked as still resident in a register/stack carrier so the next operation can reuse it without re-querying?

The file is evidence of current behavior, not a clean design surface. It mixes operand lookup, ABI register routing, stack-address reconstruction, and small pieces of call-result state tracking.

## Important API and contract surfaces

The public surface is a bag of helpers consumed by prepared x86 lowering. The main contracts are:

```cpp
std::optional<std::string> choose_prepared_float_scratch_register_if_supported(...);
std::optional<std::string> render_prepared_named_float_source_into_register_if_supported(...);
bool append_prepared_float_home_sync_if_supported(...);
```

- Float helpers assume prepared placement data is authoritative.
- They opportunistically reuse an unused scratch XMM register (`xmm15`) and otherwise refuse.
- They emit text only for supported storage classes and supported float widths.

```cpp
std::optional<std::string> render_prepared_named_f128_source_memory_operand_if_supported(...);
std::optional<std::string> render_prepared_named_f128_copy_into_memory_if_supported(...);
```

- `F128` is handled as memory-only x87-style traffic (`fld`/`fstp`, `TBYTE PTR`).
- The helper can reconstruct parameter-stack locations when prepared home data is not enough.

```cpp
bool append_prepared_direct_extern_call_argument_if_supported(...);
bool finalize_prepared_direct_extern_call_result_if_supported(...);
bool finalize_prepared_direct_extern_return_if_supported(...);
```

- Direct-extern path owns argument/result shuffling for a constrained ABI-friendly subset.
- On unsupported ABI handoff decisions it throws `kPreparedCallBundleHandoffRequired`, pushing the caller onto a slower path.

```cpp
bool append_prepared_bounded_multi_defined_call_argument_if_supported(...);
bool finalize_prepared_bounded_multi_defined_call_result_if_supported(...);
std::optional<PreparedBoundedMultiDefinedNamedI32Source>
select_prepared_bounded_multi_defined_named_i32_source_if_supported(...);
```

- The bounded-multi-defined path repeats the same pattern with more stack/addressing support and same-module symbol handling.
- It maintains a tiny “current i32 carrier” model so a fresh call result can be reused without reloading.

## Dependency direction and hidden inputs

This file depends downward on prepared-analysis facts and lower-level x86 rendering helpers:

- `PreparedNameTables`, `PreparedValueLocationFunction`, and `PreparedBirModule` provide authoritative naming/home facts.
- `PreparedModuleLocalSlotLayout`, `PreparedAddressingFunction`, and stack-layout helpers reconstruct frame offsets when direct homes are absent.
- x86 rendering helpers such as `render_prepared_stack_memory_operand`, `render_prepared_stack_address_expr`, register narrowing, and ABI register selectors provide target syntax and policy.

Hidden inputs that materially affect behavior:

- Current carried result state for i32 values, represented by `PreparedDirectExternCurrentI32Carrier` and `PreparedBoundedMultiDefinedCurrentI32Carrier`.
- `instruction_index`, which feeds ABI argument/result selection.
- Function ABI metadata on BIR params, especially for `F128` stack-passed arguments.
- Call-site callbacks for string constants, same-module globals, private-data labels, and asm symbol rendering.

The direction is mostly one-way: callers supply prepared placement/query context, this file emits assembly text fragments and updated carrier state. It should not be deciding broader lowering structure.

## Responsibility buckets

### 1. Small local utilities

- alignment helper for ABI stack offsets
- prepared-parameter stack offset translation (`rbp + 16 + ...`)
- float-size name selection
- “is register already occupied?” probing
- deduplicated name collection for used strings/globals

### 2. Float operand materialization

- load named `F32`/`F64` values into a destination XMM register
- sync a float-producing register back into its declared home
- derive aggregate-slice root memory operands, including ancestor fallback for dotted slot names

### 3. `F128` compatibility handling

- locate a named `F128` in stack/home/function-ABI space
- express it as `TBYTE PTR ...`
- copy via `fld`/`fstp`

This is compatibility support more than a clean fast path.

### 4. Direct-extern call operand routing

- resolve named `I32` sources from current carrier, constants, prepared register homes, stack homes, or rematerializable immediates
- emit pointer arguments for string constants and same-module globals
- finalize call results into either the destination home or a carried register/stack alias
- emit a very small constant/named return fast path

### 5. Bounded-multi-defined call operand routing

- resolve named `I32` sources using module-scoped prepared locations
- support pointer arguments from strings, globals, frame addresses, register homes, or stack homes
- finalize results the same way as direct-extern, but against the module-scoped lookup path

## Fast paths, compatibility paths, and overfit pressure

### Fast paths

- Reuse an existing current i32 result carrier instead of reloading from memory.
- Skip register moves when a value is already in the requested ABI register.
- Emit direct immediates for `I32` arguments/returns instead of loading from homes.
- Reuse a fixed scratch float register when it is not already claimed by prepared homes.

### Compatibility paths

- `F128` traffic is handled through x87 memory operations rather than a unified value model.
- Aggregate-slice address resolution walks ancestor names and falls back through multiple stack-address discovery routes.
- Pointer arguments support string constants and same-module globals via callback-driven label rendering instead of a single symbol abstraction.

### Overfit pressure

- The file duplicates near-identical `I32` source selection and move logic for direct-extern and bounded-multi-defined flows.
- The “current carrier” optimization leaks call-sequence state into operand helpers.
- `F128` and aggregate-slice support are encoded as shape-specific exceptions inside the same file as ordinary scalar moves.
- ABI argument/result routing is embedded as ad hoc helper orchestration rather than one reusable call-lowering service.

These are signs of local fast-path accretion. In a rebuild, they should be separated into reusable operand-query services plus distinct call-lowering policies.

## What a rebuild should and should not own

Should own:

- a compact operand-query layer that answers “where is this prepared value and how can I reference it?”
- one reusable call-ABI operand/result transport service
- explicit compatibility adapters for `F128` and aggregate slices

Should not own:

- duplicated per-path `I32` source-selection logic
- call-sequence state caching mixed directly into operand rendering
- symbol discovery callbacks and stack-layout reconstruction interleaved with string assembly emission
