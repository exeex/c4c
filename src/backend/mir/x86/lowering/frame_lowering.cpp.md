# `frame_lowering.cpp` extraction

## Purpose and current responsibility

This file is the x86 prepared-layout bridge for stack-resident values. Its real job is not instruction emission; it translates several prepared-analysis products into concrete frame offsets, named-home selections, and printable stack-memory operands for later lowering.

Current responsibilities mixed into one unit:

- build a `PreparedModuleLocalSlotLayout` for a BIR function
- resolve authoritative stack offsets for names, slices, frame-slot ids, and pointer-derived accesses
- map prepared value-home metadata into either register or frame selections
- render stack operands once an offset is known
- reconcile several competing "truth" sources for frame location

The file acts as a policy layer over prepared metadata rather than a pure frame-layout primitive.

## Important APIs and contract surfaces

Public surface from the paired header:

```cpp
std::optional<std::size_t> find_prepared_authoritative_named_stack_offset_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    const prepare::PreparedStackLayout* stack_layout,
    const prepare::PreparedAddressingFunction* function_addressing,
    const prepare::PreparedNameTables* prepared_names,
    const prepare::PreparedValueLocationFunction* function_locations,
    FunctionNameId function_name,
    std::string_view value_name,
    std::unordered_set<std::string_view>* visited_names = nullptr);

std::optional<PreparedModuleLocalSlotLayout> build_prepared_module_local_slot_layout(
    const backend::bir::Function& function,
    const prepare::PreparedStackLayout* stack_layout,
    const prepare::PreparedAddressingFunction* function_addressing,
    const prepare::PreparedNameTables* prepared_names,
    const prepare::PreparedValueLocationFunction* function_locations,
    TargetArch prepared_arch);
```

Supporting contract surfaces:

```cpp
std::optional<PreparedNamedHomeSelection> resolve_prepared_named_home_if_supported(...);
std::optional<std::size_t> resolve_prepared_named_payload_frame_offset_if_supported(...);
std::optional<std::string> render_prepared_named_stack_memory_operand_if_supported(...);
```

Behavioral contract:

- returns `std::nullopt` aggressively when architecture, type set, metadata coherence, or addressability assumptions do not hold
- prefers prepared authoritative data over ad hoc local-slot layout when stronger metadata is present
- allows recursive pointer-base resolution, guarded by a visited-name set
- emits only non-negative frame offsets

## Dependency direction and hidden inputs

Direct includes are light, but the effective dependency fan-in is large:

- `frame_lowering.hpp` exports the surface and depends on `PreparedModuleLocalSlotLayout`
- `memory_lowering.hpp` provides operand rendering
- prepared-analysis tables provide the real authority:
  - `PreparedStackLayout`
  - `PreparedAddressingFunction`
  - `PreparedValueLocationFunction`
  - `PreparedNameTables`

Hidden inputs that materially affect results:

- `function_name` must match prepared metadata identity or many lookups silently fail
- string naming conventions drive slice/base recovery via `parse_prepared_slot_slice_name`
- stack-slot objects are filtered by semantic tags like `permanent_home_slot`, `address_exposed`, and `source_kind == "local_slot"`
- `frame_alignment_bytes`, `frame_size_bytes`, frame-slot offsets, and value-home offsets can all override simple local-slot packing
- pointer-based prepared memory accesses are only accepted when `can_use_base_plus_offset` is true

Dependency direction is one-way: this file consumes prepared analysis and BIR function structure, then emits x86-oriented layout facts for downstream lowering.

## Responsibility buckets

### 1. Local frame packing

`build_prepared_module_local_slot_layout` computes local-slot offsets, alignment, and final frame size. It only supports a narrow type set (`I8/I16/I32/I64/F32/F64/F128/Ptr`) and rejects zero-sized or oversized/alignment-heavy slots.

### 2. Prepared frame-slot reconciliation

The same builder folds in prepared frame slots and prepared value-home offsets, then lets `PreparedAddressingFunction` raise alignment and frame-size requirements. This makes the builder partly a merger of local layout and prepared authoritative state.

### 3. Name-to-offset resolution

`find_prepared_fixed_permanent_named_stack_offset_if_supported` and `find_prepared_authoritative_named_stack_offset_if_supported` walk several lookup paths:

- exact permanent-home object match
- slice-name to base-slot reconstruction
- local layout named offsets
- prepared stack-layout name lookup
- prepared value-home lookup
- prepared memory-access lookup with frame-slot or pointer-value base kinds

### 4. Named-home selection

`resolve_prepared_named_home_if_supported` converts prepared value-home metadata into a compact "register or frame offset" answer for later lowering.

### 5. Operand rendering glue

`render_prepared_named_stack_memory_operand_if_supported` is a thin convenience layer that converts resolved offsets into stack operand text through memory-lowering helpers.

## Notable fast paths, compatibility paths, and overfit pressure

Fast paths:

- immediate hit in `local_layout.offsets`
- exact permanent-home object name match
- exact prepared value-home stack offset

Compatibility paths:

- slice-zero fallback for unsuffixed pointer names (`name` -> `name.0`)
- recovering base-slot offsets from slice naming when exact names are absent
- accepting either slot-id-based or raw `offset_bytes` prepared homes
- overload set for layout building that allows progressively less prepared metadata

Overfit pressure:

- heavy reliance on string names and slice syntax means behavior is partly encoded in naming conventions rather than typed semantics
- the resolution order is policy-rich and easy to grow with another special-case lookup instead of simplifying ownership
- `"local_slot"` and similar string filters are compatibility decisions embedded inside a frame helper
- pointer-value recursion plus multiple fallback layers can preserve legacy behavior while obscuring which subsystem actually owns the truth

This file is already carrying compatibility recovery logic for missing or partially aligned prepared metadata. That is useful as evidence, but risky as a rebuild template.

## Rebuild ownership

In a rebuild, this area should own:

- a narrow stack-layout query surface for x86 lowering
- explicit merger rules between local slots and prepared authoritative frame data
- small typed helpers for converting prepared homes into lowering-ready answers

In a rebuild, this area should not own:

- string-shape recovery as the primary semantic path
- broad policy about every prepared metadata fallback in one file
- generic memory-operand rendering glue unless it is the only x86-specific seam
- compatibility layering that hides which upstream analysis is authoritative
