# `helpers.cpp` extraction

## Purpose and current responsibility

This file is the shared predicate and string-rewrite toolbox for x86 peephole passes under `.../peephole/passes/`. It does not run a pass by itself; it supplies small decisions that let other passes classify instructions, detect register usage, rewrite register families, parse local labels, and spot control-flow or epilogue-adjacent situations.

The implementation is evidence of the current pass contract:

- peephole passes operate on mostly textual assembly lines, not a richer MIR node model
- `LineInfo` carries a precomputed summary that these helpers trust instead of re-deriving
- several optimizations depend on x86-specific naming, instruction prefixes, and ABI assumptions embedded here

## Important APIs and contract surfaces

Public helper surface is declared through the enclosing directory index header, not a local helper header:

```cpp
bool is_valid_gp_reg(RegId reg);
bool is_callee_saved_reg(RegId reg);
bool line_references_reg_fast(const LineInfo& info, RegId reg);
bool has_implicit_reg_usage(std::string_view trimmed);
bool is_shift_or_rotate(std::string_view trimmed);
std::optional<std::pair<RegId, RegId>> parse_reg_to_reg_movq(const LineInfo& info,
                                                             std::string_view trimmed);
std::optional<std::uint32_t> parse_label_number(std::string_view label_with_colon);
std::optional<std::string_view> extract_jump_target(std::string_view s);
bool is_near_epilogue(const LineInfo* infos, std::size_t pos);
bool is_read_modify_write(std::string_view trimmed);
std::string replace_reg_family(std::string_view line, RegId old_id, RegId new_id);
std::string replace_reg_family_in_source(std::string_view line, RegId old_id, RegId new_id);
```

Key contracts:

- `RegId` is assumed to be the shared register-family id from `peephole.hpp`, with GP ids `0..15` and `REG_NONE` as sentinel.
- `line_references_reg_fast` depends on `LineInfo.reg_refs` already being correct; this file does not validate or recompute it.
- `parse_reg_to_reg_movq` only recognizes a narrow `movq src, dst` register-to-register shape and explicitly rejects `rsp`/`rbp` families.
- `replace_reg_family*` perform textual rewriting using the hardcoded x86 GP register name table for q/l/w/b widths.
- `is_near_epilogue` assumes the caller passes a pointer to a line-info array with enough trailing entries; it uses a fixed lookahead window and no explicit length.

## Dependency direction and hidden inputs

Direct include direction is one-way into `../peephole.hpp`. This file consumes shared types and utility functions from the directory surface:

```cpp
using RegId = std::uint8_t;
struct LineInfo {
  LineKind kind;
  RegId dest_reg;
  std::uint16_t reg_refs;
};
```

Hidden inputs and assumptions:

- `starts_with`, `ends_with`, `trim_spaces`, and `trailing_operand` define much of the parsing behavior but live outside this file.
- Register-family semantics are encoded twice: symbolic ids come from `peephole.hpp`, while textual spellings live in the local `REG_NAMES` table.
- Callee-saved detection is SysV-x86-64 specific (`rbx`, `r12`-`r15`) and does not appear parameterized by target ABI.
- Instruction classification is string-prefix driven. Mnemonic spelling, spacing, and operand formatting are part of the effective API.
- `is_near_epilogue` depends on a sentinel/accessible tail in the `LineInfo*` buffer. That safety contract is external and implicit.

## Responsibility buckets

### Register-family utilities

- validate GP-family ids
- identify callee-saved families
- map textual register spellings back to a shared family id
- rewrite all width variants of one family into another

### Instruction-shape predicates

- detect instructions with implicit architectural register traffic
- classify shifts and rotates by mnemonic prefix
- identify read-modify-write style operations
- recognize the specific reg-to-reg `movq` copy shape used by copy-propagation style logic

### Local control-flow and label parsing

- parse `.LBBNNN` / label-number forms
- extract jump targets from unconditional and conditional jumps
- probe nearby lines for an epilogue-like region

## Fast paths, compatibility paths, and overfit pressure

### Fast paths

- `line_references_reg_fast` is a pure bit-test over precomputed metadata.
- `replace_reg_family` uses a fixed 4x16 name table rather than tokenizing the instruction.
- `parse_reg_to_reg_movq` short-circuits on `LineInfo.kind`, `dest_reg`, and a literal `movq ` prefix before doing more parsing.

### Compatibility paths

- `register_family_fast` accepts both bare and `%`-prefixed register names.
- `replace_reg_family_in_source` rewrites only the source side when a comma exists, preserving the destination operand text for passes that must avoid clobbering destination spelling.
- `extract_jump_target` accepts both `jmp target` and generic `j* target` spellings.

### Overfit pressure

- Instruction semantics are approximated by prefix lists: `has_implicit_reg_usage`, `is_shift_or_rotate`, and `is_read_modify_write` all rely on mnemonic fragments rather than parsed opcodes.
- `parse_reg_to_reg_movq` is intentionally narrow and may encourage pass logic to key off one textual idiom instead of a broader move model.
- Delimiter-sensitive replacement is safer than raw substring replacement, but it is still text surgery over assembly syntax, not operand-aware rewriting.
- The callee-saved rule and excluded families (`rsp`, `rbp`) are embedded policy decisions that may get copied into more passes if rebuild boundaries stay unclear.

## Rebuild ownership

In a rebuild, this unit should own reusable x86 peephole helper semantics that multiple passes genuinely share: register-family mapping, small instruction-shape predicates, and narrowly defined parsing/rewrite utilities with explicit contracts.

It should not own pass policy, ABI-wide target configuration, unsafe buffer-walk assumptions, or a growing list of string-matched special cases that exist only to make one optimization pattern work. Those belong either in stronger shared analysis surfaces or in the specific pass that needs the compatibility behavior.
