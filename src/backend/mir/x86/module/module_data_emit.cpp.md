# `module_data_emit.cpp`

## Purpose And Current Responsibility

This file is the x86 module-level data backfill layer that turns prepared BIR module metadata into inline assembly text after function/body emission has already produced some `asm_text`.

Its current job is broader than the name suggests:

- provide a convenience facade (`ModuleDataSupport`) over module data lookups and emission
- decide whether a same-module global can be treated as scalar-readable without a full data-layout model
- render string constants and simple globals into target-flavored assembly
- scan emitted assembly for referenced-but-not-defined symbols, then synthesize missing same-module globals
- inject a tiny set of direct variadic helper implementations when the assembly references LLVM varargs entry points but does not define them
- finalize a rendered module by appending helpers and data in dependency order

This is not just formatting. It also owns policy about what kinds of globals are considered supported, when compatibility shims appear, and how symbol reachability is inferred from raw assembly text.

## Important APIs And Contract Surfaces

Public surface is a thin object wrapper plus free functions over the same behavior:

```cpp
struct ModuleDataSupport {
  const PreparedBirModule* module = nullptr;
  std::string_view target_triple;

  std::string finalize_multi_defined_rendered_module_text(std::string_view rendered_text) const;
  std::string finalize_selected_module_text(
      std::string_view rendered_text,
      const std::unordered_set<std::string_view>& used_string_names,
      std::unordered_set<std::string_view>* used_same_module_global_names) const;
  std::string emit_selected_module_data(
      const std::unordered_set<std::string_view>& used_string_names,
      const std::unordered_set<std::string_view>& used_same_module_global_names) const;
};
```

Key behavioral contracts:

- `find_string_constant` and `find_same_module_global` are linear lookups into `PreparedBirModule`.
- `same_module_global_supports_scalar_load` is a capability predicate, not a full verifier. It only recognizes a narrow scalar subset.
- `emit_same_module_global_data` returns `std::nullopt` for unsupported global initializer shapes rather than partially emitting.
- `emit_missing_same_module_global_data` and `add_referenced_same_module_globals` derive reachability from text search over already-rendered assembly.
- `finalize_*` append generated helpers/data after the existing text and may recursively pull in dependent globals through pointer initializers.

Representative free-function surface:

```cpp
std::optional<std::string> emit_same_module_global_data(
    std::string_view target_triple,
    const c4c::backend::bir::Global& global);

std::string emit_missing_same_module_global_data(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::string_view target_triple,
    std::string_view asm_text);
```

## Dependency Direction And Hidden Inputs

Direct dependencies flow downward into:

- BIR/prepared-module data (`PreparedBirModule`, `Global`, `StringConstant`, `Value`, `TypeKind`)
- ABI naming helpers in `../abi/x86_target_abi.hpp`
- assembly-text symbol scanners in `../core/x86_codegen_output.hpp`

Hidden inputs that materially affect behavior:

- `target_triple` changes symbol spelling, Darwin `.type` suppression, and private-label naming
- raw `asm_text` content controls whether globals/helpers are considered referenced or already defined
- initializer shape constraints are hardcoded locally, not queried from a general data-layout or relocation layer
- recursive dependency closure is inferred from pointer-like named initializers that start with `@`

The file depends on earlier lowering having produced assembly names that match the text scanners and ABI renderers exactly. There is no structured symbol table handoff here.

## Responsibility Buckets

### 1. Lookup And Facade

- `ModuleDataSupport` mostly forwards to namespace-level functions
- `make_module_data_support` packages `module + target_triple` into a small context object

### 2. Scalar-Support Heuristics

The file carries a very narrow model for what same-module globals can support:

```cpp
std::optional<std::size_t> same_module_global_scalar_size(TypeKind type);
bool same_module_global_supports_scalar_load(
    const Global& global, TypeKind type, std::size_t byte_offset);
```

Current support is effectively limited to `I8`, `I32`, and pointer-width loads, plus zero-filled regions and exact element-offset matches inside flat initializer lists.

### 3. Data Rendering

- `emit_string_constant_data` emits `.section .rodata` plus escaped `.asciz`
- `emit_defined_global_prelude` chooses `.bss` vs `.data`, emits `.globl`, optional `.type`, simple `.p2align`, and the symbol label
- `append_initializer_value` lowers a small value subset to `.byte`, `.long`, `.quad`, or symbol references
- `emit_same_module_global_data` assembles the whole global definition or rejects unsupported shapes

### 4. Text-Driven Reachability And Closure

- `emit_missing_same_module_global_data` scans assembly for referenced same-module globals that are not yet defined, then recursively emits their dependencies
- `add_referenced_same_module_globals` computes the same closure but only records names for selected-mode emission

Both routines walk transitive dependencies through:

- `initializer_symbol_name`
- scalar named pointer initializers
- named pointer elements in initializer arrays

### 5. Compatibility Helper Injection

`emit_direct_variadic_runtime_helpers` synthesizes inline implementations for:

- `llvm.va_start.p0`
- `llvm.va_end.p0`
- `llvm.va_copy.p0.p0`

This is a compatibility bridge for emitted assembly that references those runtime entry points directly.

## Fast Paths, Compatibility Paths, And Overfit Pressure

### Core Lowering

- string constant emission
- same-module global emission for a small supported initializer subset
- selected/final module data assembly

### Optional Fast Paths

- zero-initialized global detection allows `.bss` emission and wider scalar-load acceptance without materializing explicit elements
- selected-mode emission avoids dumping all module data by using caller-provided used-name sets

### Legacy Compatibility

- Darwin/non-Darwin assembly directive split is handled inline
- direct emission of `llvm.va_*` helpers patches over missing runtime implementations
- missing-global recovery uses raw text reference scanning instead of a structured relocation/export phase

### Overfit Pressure

- scalar-load support is encoded as a hand-maintained whitelist of BIR types and initializer shapes
- `.p2align` selection only distinguishes 2-byte alignment from everything larger, collapsing richer alignment intent
- named pointer detection depends on the string convention `name.front() == '@'`
- helper injection is keyed off exact symbol spellings in rendered text
- unsupported initializer forms silently fall out through `nullopt`, while one selected-data path escalates to `invalid_argument`

This is pressure toward case-by-case extension: each new global form, relocation shape, or ABI quirk is likely to become another local conditional in this file.

## Rebuild Ownership Boundary

In a rebuild, this unit should own module-data orchestration: selecting required data artifacts, asking lower layers to render them, and appending them to finalized module text.

It should not own ABI-specific directive policy, raw assembly string scanning as the primary dependency mechanism, ad hoc varargs runtime bodies, or the detailed legality matrix for every supported global initializer form. Those should move to clearer lower-level services with structured inputs.
