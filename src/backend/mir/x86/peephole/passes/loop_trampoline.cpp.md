# `loop_trampoline.cpp`

## Purpose And Current Responsibility

This file implements one peephole pass, `eliminate_loop_trampolines`, over the x86 peephole line model. Its job is to detect a branch that targets a tiny label block whose real effect is:

- a sequence of register-to-register moves and/or stack reloads
- followed by an unconditional jump to a second label

When safe, it removes the trampoline by:

- redirecting the original branch to the final target
- folding predecessor-side copies/spills into the destination register family
- marking now-dead copies, stack traffic, and trampoline lines as `Nop`

The file owns both recognition and mutation. It is not just a matcher; it also performs local dataflow checks, predecessor scanning, and textual rewrite of assembly lines.

## Primary Surface

```cpp
bool eliminate_loop_trampolines(LineStore* store, LineInfo* infos);
```

Contract surface:

- input is the mutable peephole program representation from `peephole.hpp`
- `store` holds original text lines; `infos` holds preclassified metadata
- output is `true` iff the pass changed text or `LineInfo.kind`
- mutation happens in place through `replace_line`, `replace_jump_target`, and `mark_nop`

This pass assumes `infos` already contains reliable classification such as `LineKind`, `dest_reg`, `rbp_offset`, register references, and barrier detection.

## Important APIs And Hidden Inputs

This file is downstream of the generic peephole classifier and utility layer in `peephole.hpp` / sibling helpers. The pass depends heavily on these external contracts:

```cpp
enum class LineKind { LoadRbp, StoreRbp, Label, Jmp, CondJmp, Call, Ret, Directive, Other, ... };

struct LineInfo {
  LineKind kind;
  RegId dest_reg;
  std::int32_t rbp_offset;
  std::uint16_t reg_refs;
  bool is_barrier() const;
};

std::optional<std::string_view> extract_jump_target(std::string_view s);
std::uint8_t register_family_fast(std::string_view reg);
bool line_references_reg_fast(const LineInfo& info, RegId reg);
bool is_read_modify_write(std::string_view trimmed);
std::string replace_reg_family(std::string_view line, RegId old_id, RegId new_id);
```

Hidden inputs that drive behavior:

- exact textual spelling of x86 instructions after trimming
- fixed register-family numbering where `0` means the `rax/eax` family
- `LoadRbp` and `StoreRbp` classification for stack-slot traffic
- barrier semantics embedded in `LineKind` and `LineInfo::is_barrier()`
- assumption that dead lines can be represented by switching `kind` to `Nop` without removing the source text

Dependency direction is one-way: this pass consumes normalized line metadata and low-level text helpers, but exports only the pass entry point.

## Responsibility Buckets

### 1. Trampoline block recognition

The pass finds a branch target label, proves it has exactly one incoming branch and no meaningful fallthrough predecessor, then parses the target block until the terminating unconditional jump.

Accepted block members are narrowly restricted to:

- reg-to-reg moves: `movq` or `movslq`
- stack reloads from `rbp` slots, either direct to final destination or via `rax` then a copy
- final `jmp`

Anything outside those shapes aborts the candidate.

### 2. Safety proof against predecessor code

For each trampoline move or stack reload, the pass scans backward from the branch site looking for:

- the earlier copy into the source register family
- optional intervening modifications that can be rewritten
- disqualifying reads/writes, barriers, `setcc`, mixed source/destination use, or unsupported sign-extension shapes

It also scans forward with limited jump following to verify that fallthrough code does not observe either register before both are killed.

### 3. Textual rewrite and dead-line marking

When coalescing succeeds, the pass:

- rewrites predecessor instructions by replacing one register family with another
- marks superseded copies and stack stores as `Nop`
- marks trampoline interior move/load lines as `Nop`
- optionally redirects the original branch to the trampoline’s final target and marks the trampoline jump as `Nop`

The rewrite unit is still raw text. There is no IR-level operation graph here.

## Fast Paths, Compatibility Paths, Overfit Pressure

### Fast paths

- empty trampoline block: if the target label immediately jumps elsewhere, just retarget the incoming branch and nop the trampoline jump
- direct stack reload into the final destination register
- full coalescing path when every trampoline move/load can be proven safe

### Compatibility paths

- supports `movq` and `movslq` as the accepted copy vocabulary
- supports stack reloads that go through `rax/eax` before copying into the real destination
- tolerates partial cleanup: if some internal moves coalesce but not all, it can still remove the coalesced trampoline interior without retargeting the original branch

### Overfit pressure

This file is highly shape-driven and under strong overfit pressure:

- register names are hardcoded in `kReg64Names` / `kReg32Names`
- stack handling is effectively specialized around `rbp` slots and the `rax/eax` family
- only a tiny whitelist of move and reload spellings is accepted
- safety logic follows at most two forward jumps and depends on line classification quality
- rewrites assume textual register-family substitution is semantics-preserving for the accepted lines

The likely failure mode is not unsound global optimization; it is silent refusal to optimize anything outside the narrow recognized assembly shapes.

## Notable Internal Seams

Key helper clusters inside this file:

- line normalization wrappers: `trimmed_line`, `replace_line`, `mark_nop`, `next_real_line`
- control-flow shape checks: `has_fallthrough_predecessor`, `count_branches_to_label`, `find_label_index`
- trampoline parsing: `parse_trampoline_reg_move`, `collect_trampoline_block`
- predecessor/fallthrough safety: `verify_fallthrough_safety`, `find_copy_and_modifications`, `find_stack_spill_copy_and_modifications`
- final mutation: `replace_jump_target`

These helpers are private but collectively define the real contract of the pass.

## Rebuild Ownership

In a rebuild, this logic should own:

- recognizing loop-trampoline opportunities as a distinct optimization concept
- proving local safety for branch retargeting and copy coalescing
- emitting structured rewrite decisions back to the peephole pipeline

It should not own:

- raw string parsing for every accepted x86 spelling
- register-name tables and ad hoc operand-shape decoding
- direct text mutation policy mixed together with control-flow/dataflow proof
- special treatment of `rax`-mediated stack reloads as an inlined one-off compatibility mechanism
