# `callee_saves.cpp` extraction

## Purpose and current responsibility

This pass removes callee-saved register save/restore pairs when the saved register is never referenced in the function body. It operates as a narrow textual/metadata peephole over already-classified assembly lines, not as a frame-layout authority.

The implementation assumes a very specific prologue shape:

1. `push %rbp`
2. `movq %rsp, %rbp`
3. `subq $..., %rsp`
4. one or more negative-`%rbp` stores of callee-saved registers

If that shape is not found, the pass exits without changing anything.

## Important APIs and contract surfaces

Primary entrypoint:

```cpp
void eliminate_unused_callee_saves(const LineStore* store, LineInfo* infos);
```

Shared surface it relies on:

```cpp
struct LineInfo {
  LineKind kind;
  std::uint16_t trim_start;
  RegId dest_reg;
  std::int32_t rbp_offset;
  std::uint16_t reg_refs;
};

struct LineStore {
  std::size_t len() const;
  const std::string& get(std::size_t i) const;
};
```

Key helper contracts:

```cpp
bool is_callee_saved_reg(RegId reg);
bool line_references_reg_fast(const LineInfo& info, RegId reg);
bool is_near_epilogue(const LineInfo* infos, std::size_t pos);
```

Local helper behavior:

```cpp
void mark_nop(LineInfo& info);
static std::string trimmed_line(const LineStore* store,
                                const LineInfo& info,
                                std::size_t index);
```

- `mark_nop` only mutates `LineInfo.kind`; it does not rewrite source text.
- `trimmed_line` reconstructs a trimmed textual view by slicing `LineStore` with `trim_start`.

## Dependency direction and hidden inputs

Dependency direction is downward into the generic peephole substrate:

- consumes `LineStore` text plus precomputed `LineInfo` classification
- trusts `dest_reg`, `rbp_offset`, `trim_start`, and `kind` to already be correct
- depends on `is_near_epilogue` to classify restore loads as safe-to-remove epilogue restores
- depends on `line_references_reg_fast` as the only body-use detector

Hidden inputs and assumptions:

- `%rbp` is encoded as register id `5`; the pass hardcodes that value
- function end is detected by scanning for a `.size ` directive
- restore detection only recognizes `LoadRbp` from the same negative `%rbp` slot
- NOP lines are skipped everywhere, so earlier passes can materially affect discovery
- body liveness is approximated by per-line register-reference metadata, not CFG analysis

## Responsibility buckets

### 1. Prologue recognizer

Finds a candidate frame setup beginning at `push %rbp`, skipping existing NOPs between expected lines.

### 2. Save collector

Collects contiguous negative-`%rbp` `StoreRbp` lines that target callee-saved registers immediately after the frame allocation.

### 3. Function-range finder

Sets the scan range from the end of the save block to the next `.size` directive, treating that as the body/epilogue search window.

### 4. Per-register elimination

For each saved register:

- collect matching epilogue restore loads
- stop if any non-epilogue line references the register
- otherwise mark the save and all matching restores as `Nop`

## Notable fast paths, compatibility paths, and overfit pressure

### Optional fast path

- Uses `line_references_reg_fast` and line classification instead of parsing full instructions repeatedly.
- Skips NOP runs aggressively to avoid reprocessing already-deleted lines.

### Legacy compatibility path

- Accepts only one legacy frame pattern rather than a generalized prologue model.
- Uses textual equality for `movq %rsp, %rbp` and prefix matching for `subq $`.
- Leaves frame size untouched even if all callee-save spills disappear.

### Overfit pressure

This file is strongly shaped around one assembly layout:

- fixed `%rbp` frame setup
- spill block immediately after stack allocation
- restores recognized only near epilogue
- `.size` used as function boundary marker

That means the pass can silently miss valid opportunities for:

- functions with different prologue ordering
- interleaved spills/setup code
- alternate restore placement
- control-flow-heavy bodies where simple line scanning is too weak

It also risks false confidence because success is measured by line-pattern matching, not by a semantic model of preserved registers or frame ownership.

## Rebuild ownership

This file should own only the policy for deciding when an otherwise-classified callee-save spill/restore pair is dead.

This file should not own:

- raw text trimming or line-storage mechanics
- hardcoded prologue parsing details beyond a shared frame-model interface
- epilogue-shape heuristics
- function-boundary discovery
- frame compaction or stack-slot reclamation
