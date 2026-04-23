# `memory_fold.cpp`

## Purpose And Current Responsibility

This file implements one peephole pass: replace a stack-slot reload followed immediately by a small ALU-or-compare register operation with a direct memory operand form. Its current job is narrowly local:

- recognize `LoadRbp` lines that reload from `offset(%rbp)`
- inspect the next non-empty instruction
- if that next instruction is an allowed reg-reg ALU form using the loaded register as source, rewrite it to read directly from memory
- mark the original load as `Nop`

The pass is text-rewrite based, but it only works because upstream classification has already converted raw assembly lines into `LineInfo` facts.

## Important API And Contract Surfaces

Primary exported surface from this file:

```cpp
bool fold_memory_operands(LineStore* store, LineInfo* infos);
```

Expected ambient model from the owning peephole subsystem:

```cpp
struct LineInfo {
  LineKind kind;
  std::uint16_t trim_start;
  RegId dest_reg;
  bool has_indirect_mem;
  std::int32_t rbp_offset;
  std::uint16_t reg_refs;
};

LineInfo classify_line(std::string_view raw);
std::uint8_t register_family_fast(std::string_view reg);
```

Operational contract:

- `store` and `infos` must be parallel arrays over the same instruction stream.
- `infos` must already reflect current `store` contents on entry.
- when this pass rewrites an instruction, it must immediately reclassify that line so downstream passes see fresh metadata
- when this pass deletes a line logically, it does so by turning the corresponding `LineInfo` into `Nop`; physical line removal is not its concern

## Dependency Direction And Hidden Inputs

Dependency direction is one-way into the peephole support layer:

- depends on `LineStore` for mutable text
- depends on `LineInfo` classification for semantic-ish facts
- depends on `classify_line()` after mutation to rebuild metadata
- depends on `register_family_fast()` to normalize textual register spellings into family ids
- depends on `is_valid_gp_reg()` from pass helpers for GP-register gating

Hidden inputs that materially control behavior:

- `LineKind::LoadRbp` classification must be accurate; this pass does not parse generic memory loads itself
- `rbp_offset` must already be extracted and validated upstream
- `dest_reg` numbering must be stable across width variants (`%al`, `%eax`, `%rax` family collapse)
- `trim_start` controls how the pass slices leading whitespace before textual matching
- the peephole pipeline must tolerate `Nop` tombstones between active instructions

## Responsibility Buckets

### 1. Candidate Load Detection

The pass starts only from stack reloads classified as `LoadRbp` with a concrete `rbp_offset`. It further narrows to `movq` and `movl` textual loads.

### 2. Next-Instruction Pairing

It skips over `Nop` and `Empty` entries to find the next real instruction, but only considers `Other` and `Cmp` as fold targets.

### 3. Target Instruction Parsing

It recognizes a small whitelist of two-operand integer mnemonics and requires size suffixes:

```cpp
// accepted opcode families
add* sub* and* xor* cmp* test* or*
```

The target must parse as register-to-register, with:

- source register family equal to the earlier loaded register
- destination register family different from the loaded register

### 4. Rewrite And Tombstoning

On success, it emits a new instruction using `offset(%rbp)` as the source operand, marks the load as `Nop`, and reclassifies the rewritten consumer line.

## Fast Paths, Compatibility Paths, And Overfit Pressure

Core lowering:

- one-step fold of `mov{q,l} offset(%rbp), %rX` into the immediately following ALU/cmp consumer

Optional fast path:

- skip over intervening `Nop` and `Empty` lines instead of requiring strict adjacency

Legacy compatibility:

- only supports classic AT&T two-operand integer forms with explicit size suffixes
- only folds loads from `%rbp` stack slots, not arbitrary memory operands
- only handles loads whose destination register id is `0`, `1`, or `2`; this is an encoded policy boundary, not an architectural truth

Overfit pressure:

- the opcode whitelist is handwritten and shape-based rather than derived from a shared instruction model
- the `load_reg > 2` cutoff is a strong smell that current behavior is tuned to a narrow register subset
- the pass reparses trimmed text even though classification already exists, so any textual style drift can silently disable folding
- the pass has no explicit hazard model for flags, aliasing, or memory-side effects beyond "next real line looks safe enough"

## Rebuild Ownership Guidance

A rebuild should own:

- the semantic rule for when a stack reload can be folded into a later memory-form consumer
- a reusable instruction-shape model for binary ops and compares
- explicit hazard checks instead of mnemonic-string heuristics

A rebuild should not own:

- ad hoc register-subset gating without a stated architectural reason
- repeated local string parsing helpers for instruction semantics
- pipeline-wide `Nop` conventions or line reclassification mechanics beyond the narrow mutation interface it needs
