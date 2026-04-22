# `inline_asm.cpp`

Primary role: inline-assembly operand substitution, register formatting, and modifier handling.

Key surfaces:

```cpp
std::string X86Codegen::substitute_x86_asm_operands(std::string_view asm_text, const InlineAsmInst& inst);
void X86Codegen::emit_operand_with_modifier(std::string& out, const Operand&, std::optional<char> modifier);
std::optional<char> X86Codegen::default_modifier_for_type(std::optional<IrType> ty);
```

- Resolves dialect alternatives, operand placeholders, block labels, and per-type register modifiers into final assembler text.
- Depends on the same register-name formatting helpers exported from `mod.cpp`.
- Exists as a compatibility layer around external inline-asm syntax rather than a normal BIR lowering family.
- Special-case classification: `legacy compatibility`.
