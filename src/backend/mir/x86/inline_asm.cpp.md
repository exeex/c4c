# `inline_asm.cpp`

## Purpose And Current Responsibility

This file is the x86 backend's inline-assembly text expander for operand and label placeholders. It does not select constraints, allocate registers, or emit machine instructions directly. Its job is to turn a parsed GCC-style inline-asm template plus already-decided operand facts into final x86 textual fragments.

In practice it owns three tightly related behaviors:

- resolve dialect-alternative syntax embedded in the asm template
- substitute operand and goto-label references into x86 text
- format registers and immediates according to x86 width/modifier rules

## Important API Surface

The public surface exposed through `X86Codegen` is small but high-impact because callers must fully precompute operand facts before entering this file.

```cpp
static std::string substitute_x86_asm_operands(
    const std::string& line,
    const std::vector<std::string>& op_regs,
    const std::vector<std::optional<std::string>>& op_names,
    const std::vector<bool>& op_is_memory,
    const std::vector<std::string>& op_mem_addrs,
    const std::vector<IrType>& op_types,
    const std::vector<std::size_t>& gcc_to_internal,
    const std::vector<std::pair<std::string, BlockId>>& goto_labels,
    const std::vector<std::optional<std::int64_t>>& op_imm_values,
    const std::vector<std::optional<std::string>>& op_imm_symbols);
```

Contract shape:

- `line` is still in GCC inline-asm placeholder syntax.
- operand vectors must already be aligned by internal operand index
- `gcc_to_internal` remaps numeric `%0`-style references
- `goto_labels` provides both named and numeric `%l...` targets
- immediates may arrive as either integer values or symbolic names

Supporting width/format helpers are also externally declared on `X86Codegen`:

```cpp
static void emit_operand_with_modifier(...);
static std::optional<char> default_modifier_for_type(std::optional<IrType> ty);
static std::string format_x86_reg(const std::string& reg, std::optional<char> modifier);
static const char* gcc_cc_to_x86(const std::string& cond);
```

## Dependency Direction And Hidden Inputs

Dependency direction is inward toward generic backend state and x86 naming helpers:

- includes `x86_codegen.hpp`, so this file is an implementation shard of the large `X86Codegen` surface rather than an isolated subsystem
- depends on `IrType` to infer default width modifiers
- depends on `BlockId` layout and local block-label naming convention via `.LBB<raw>`
- depends on external register-name conversion helpers such as `x86_format_reg`, `x86_reg_name_to_16`, `x86_reg_name_to_8l`, `x86_reg_name_to_8h`, and `x86_reg_name_to_64`
- forwards condition-code mapping to `x86_gcc_cc_to_x86`

Hidden inputs and assumptions:

- operand vectors are assumed to be mutually consistent; mismatch handling is mostly "best effort" fallback text emission
- the chosen register names already reflect earlier constraint solving; this file does not validate that a modifier is semantically legal for the chosen register
- AT&T-style immediate spelling is baked in through `$...` and RIP-relative symbol expansion
- dialect alternatives are collapsed by always keeping the first branch inside `{a|b}` groups

## Responsibility Buckets

### 1. Template Normalization

`resolve_dialect_alternatives` strips GCC dialect-choice groups and retains the first alternative. This is a compatibility normalization pass before any `%` parsing happens.

### 2. Placeholder Parsing And Rewriting

`substitute_x86_asm_operands` walks the normalized string and handles:

- escaped `%%`
- named operands like `%[name]`
- numeric operands like `%0`
- label references `%l[name]` and `%l123`
- modifier-prefixed forms such as `%k0`, `%b[name]`, `%P0`, `%n0`
- fallback passthrough when a placeholder cannot be resolved

### 3. Operand Text Selection

`emit_operand_common` and `emit_operand_with_modifier` choose among:

- symbolic immediate
- numeric immediate
- memory address text
- register text
- address-form register wrapping like `(%rax)`

This is where modifier-specific behavior is concentrated: raw/immediate-preserving modes (`c`, `P`), negated-immediate mode (`n`), and address mode (`a`).

### 4. Register Width Adaptation

`default_modifier_for_type` and `format_x86_reg` convert a chosen architectural register into the width-specific spelling expected by the asm template.

Important special case:

- `xmm*` and x87 `st` forms bypass general GPR width rewriting and are returned unchanged

## Fast Paths, Compatibility Paths, And Overfit Pressure

### Fast Paths

- early return when a template line has no dialect-alternative braces
- `emit_operand_common` handles many cases without invoking register-width formatting
- unchanged passthrough for unresolved placeholders avoids hard failure

### Compatibility Paths

- first-branch-only handling for `{att|intel}`-style syntax is a compatibility shortcut, not full dialect support
- numeric goto-label remapping depends on GCC operand numbering rules and the local `op_regs.size()` offset convention
- `%a` emits symbols as `symbol(%rip)`, baking in current x86-64 addressing expectations
- missing named operands or out-of-range indices degrade to textual passthrough instead of producing a backend error

### Overfit Pressure

This file is vulnerable to accreting ad hoc modifier cases because template rewriting is driven by syntax shape rather than richer typed operand objects. Specific pressure points:

- more GCC inline-asm modifier variants could be added as one-off parser branches
- label-number handling already contains shape-sensitive arithmetic tied to current operand-vector layout
- unresolved placeholder passthrough can hide unsupported semantics and encourage "just preserve the text" behavior
- AT&T spelling assumptions make it tempting to patch individual cases instead of separating dialect selection from operand rendering

## Essential Behavior Surfaces

```cpp
// width choice when the template omitted an explicit modifier
switch (ty.value_or(IrType::Void)) {
  case IrType::I8: return 'b';
  case IrType::I16:
  case IrType::U16: return 'w';
  case IrType::I32:
  case IrType::U32:
  case IrType::F32: return 'k';
  default: return std::nullopt;
}
```

```cpp
// vector/x87 names bypass GPR width remapping
if (reg.rfind("xmm", 0) == 0 || reg == "st" || reg.rfind("st(", 0) == 0) {
  return reg;
}
```

These snippets show the file's real role: render already-selected operands into the exact textual variant expected by legacy inline-asm syntax.

## Rebuild Ownership

This file should own:

- x86-specific inline-asm text rendering
- placeholder-to-text substitution rules
- narrow register-name formatting helpers for inline-asm output

This file should not own in a rebuild:

- constraint solving or operand selection policy
- asm dialect selection policy beyond consuming an already-chosen mode
- block-label numbering conventions as hidden local string rules
- broad register taxonomy knowledge duplicated from general x86 register helpers
- silent compatibility fallbacks for unsupported placeholder semantics
