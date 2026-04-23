# `push_pop.cpp` extraction

## Purpose and current responsibility

This file implements a narrow x86 peephole pass family that deletes obviously redundant `push`/`pop` traffic and rewrites one small stack-spill pattern into a direct register transfer. It operates on already-classified assembly lines, not on MIR or CFG structure, and assumes neighboring instructions are trustworthy enough for short-range local reasoning.

Current responsibility is mixed but still bounded:

- detect short-distance `push r` ... `pop r` pairs and erase them when the register value and stack pointer appear untouched in between
- detect a three-instruction pattern shaped like `push`, redirectable load, register-to-register move, `pop`, then fold it into a direct destination rewrite
- provide local string helpers for stack-touch and destination-rewrite heuristics used only by this pass

The file is an evidence artifact for current behavior, not a clean design surface. Most decisions are driven by line classification plus substring tests on assembly text.

## Important APIs and contract surfaces

Directory-level contract from `peephole.hpp`:

```cpp
bool eliminate_push_pop_pairs(const LineStore* store, LineInfo* infos);
```

Local helpers define the real behavior boundary:

```cpp
bool instruction_modifies_reg_id(const LineInfo& info, RegId reg_id);
bool instruction_modifies_stack(std::string_view line, const LineInfo& info);
bool parse_reg_to_reg_move(std::string_view line, std::string_view push_reg);
bool instruction_writes_to(std::string_view line, std::string_view reg);
bool can_redirect_instruction(std::string_view line);
std::string replace_dest_register(std::string_view line,
                                  std::string_view old_reg,
                                  std::string_view new_reg);
```

Mutation surface:

```cpp
void mark_nop(LineInfo& info);
void replace_line(LineStore* store, LineInfo& info,
                  std::size_t index, const std::string& text);
```

Observed contract assumptions:

- `store->lines` and `infos[]` are parallel arrays and stay index-aligned
- `LineInfo.kind`, `dest_reg`, and `trim_start` are trusted classifier outputs
- marking a line as `Nop` is sufficient logical deletion for later passes
- replacing text also requires resetting `trim_start` because later consumers read the whole rewritten line

## Dependency direction and hidden inputs

Direct dependency direction is downward into the shared peephole substrate:

- depends on `LineStore` for mutable assembly text
- depends on `LineInfo` classification from earlier parsing
- depends on `LineKind`, `RegId`, `REG_NONE`, `starts_with`, and `ends_with`

Hidden inputs that actually control behavior:

- textual instruction spelling, especially `mov*`, `lea`, `%rsp`, `pushf`, `popf`
- short scan windows: pair elimination searches at most the next three non-barrier lines
- classifier precision for `dest_reg`; misclassification changes safety decisions
- exact formatting assumptions such as "opcode then space then operand list"
- `rfind`-based replacement of the last textual register occurrence, which assumes destination is the final matching register token

This means the pass is only partly semantic. The actual outcome depends on both line classification and residual assembly-string shape.

## Responsibility buckets

### 1. Redundant push/pop pair elimination

`eliminate_push_pop_pairs` scans forward from each `Push` up to a small fixed window and removes a matching `Pop` of the same `RegId` when all intervening lines are judged safe.

Safety gate today:

- no intervening instruction that modifies the pushed register
- no intervening instruction that appears to modify stack state
- search stops on another `Push`, `Call`, jump, or `Ret`

This is a local redundancy cleanup pass, not general stack analysis.

### 2. Pattern-shaped push/load/move/pop rewrite

`eliminate_binop_push_pop_pattern` is the more specialized path. It collects the next three non-`Nop` lines, expects them to be:

1. a load-like instruction after the `push`
2. a register-to-register `movq`
3. a matching `pop`

If the load writes the pushed register and is considered redirectable (`mov*` or `lea`), the pass rewrites the load's destination register to the move target and deletes the surrounding stack traffic plus the move.

This is effectively a tiny stack-spill forwarding transform, but encoded as a rigid local pattern.

### 3. Local textual safety and rewrite helpers

The remaining functions classify stack writes, register writes, redirectability, and perform raw text substitution. These helpers are not reusable abstractions; they encode this pass's private heuristics.

## Notable fast paths, compatibility paths, and overfit pressure

### Fast paths

- fixed small lookahead makes the common redundant-pair case cheap
- `collect_non_nop_indices` lets the pattern matcher skip already-deleted lines without rebuilding a compact view
- `mark_nop` avoids structural erasure and keeps indices stable

### Compatibility paths

- `instruction_modifies_stack` falls back to broad `%rsp` substring detection for `LineKind::Other`
- `instruction_modifies_reg_id` encodes conservative behavior by `LineKind` rather than precise operand semantics
- `can_redirect_instruction` accepts broad textual prefixes (`mov`, `lea`) instead of a typed instruction model

These are compatibility heuristics for a text-first peephole framework, not robust ISA modeling.

### Overfit pressure

- `parse_reg_to_reg_move` only checks `starts_with("movq ")` and whether the pushed register appears anywhere in the line; it does not validate operand roles
- `instruction_writes_to` is a raw substring search, so aliasing and incidental register mentions can mislead the transform
- `replace_dest_register` rewrites the last textual occurrence of a register name, assuming that is the destination site
- the binop pattern path is strongly case-shaped: exactly three later non-`Nop` instructions, exact relative order, same-register `push`/`pop`
- pair elimination window is capped at a tiny range, which is performance-friendly but also behavior-shaped rather than capability-shaped

The general rebuild risk is extending these textual heuristics instead of replacing them with clearer operand-aware contracts.

## Rebuild ownership

This file should own only local push/pop redundancy cleanup and narrowly-scoped stack-traffic forwarding rules with explicit operand-aware safety contracts.

This file should not own generic string rewriting utilities, broad stack-semantics detection for arbitrary x86 text, or a growing collection of testcase-shaped local patterns that compensate for missing intermediate representation or instruction modeling.
