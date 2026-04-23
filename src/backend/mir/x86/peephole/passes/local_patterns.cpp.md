# `local_patterns.cpp`

## Purpose and current responsibility

This file is a small bundle of local x86 peephole rewrites that operate on the
already-tokenized `LineStore` plus per-line `LineInfo` metadata. Its job is not
full dataflow or CFG optimization; it applies narrow line-sequence cleanups that
can be decided from nearby context and preclassified line kinds.

Today it owns two public pass entry points:

```cpp
bool combined_local_pass(LineStore* store, LineInfo* infos);
bool fuse_movq_ext_truncation(LineStore* store, LineInfo* infos);
```

`combined_local_pass` is the catch-all local simplifier. `fuse_movq_ext_truncation`
is a separate one-pattern cleanup for a `movq` followed by redundant `%eax`
self-move.

## Important APIs and contract surfaces

The file depends on the surrounding peephole framework for storage, line
classification, and parse helpers. It mutates both the textual assembly and the
metadata array in place.

```cpp
struct LineInfo {
  LineKind kind;
  std::uint16_t trim_start;
  RegId dest_reg;
  // ... ext_kind, indirect-mem flags, rbp offset, reg refs ...
};

struct LineStore {
  std::vector<std::string> lines;
  std::size_t len() const;
  std::string& get(std::size_t i);
};
```

Local helper surfaces in this file are mutation-oriented, not analysis-oriented:

```cpp
static void mark_nop(LineInfo& info);
static void replace_line(LineStore* store, LineInfo& info,
                         std::size_t index, const std::string& text);
static std::size_t collect_non_nop_indices(const LineInfo* infos,
                                           std::size_t start,
                                           std::size_t len,
                                           std::size_t* out,
                                           std::size_t out_len);
```

Important external helper contracts:

- `parse_reg_to_reg_movq(...)` identifies register-to-register `movq`.
- `extract_jump_target(...)` exposes branch target text.
- `starts_with`, `ends_with`, `contains` support raw string-shape matching.
- `LineInfo.kind`, `dest_reg`, and `trim_start` must already be accurate before
  these passes run.

## Dependency direction and hidden inputs

Dependency direction is downward into shared peephole parsing/classification.
This file does not define the assembly model; it consumes it.

Hidden inputs that materially affect behavior:

- `trim_start` controls `trimmed_line(...)`, so correctness depends on earlier
  whitespace-aware classification.
- `dest_reg == 0` is treated as `%rax`/`%eax` ownership; that numeric encoding
  is implicit here rather than named locally.
- Barrier behavior is duplicated through explicit `LineKind` checks instead of
  calling `LineInfo::is_barrier()`.
- Pattern matching relies on canonical text spellings such as
  `xorl %eax, %eax`, `testq %rax, %rax`, and `movl %eax, %eax`.
- `replace_line(...)` resets `trim_start` but does not re-run full
  reclassification, so rewritten lines must stay compatible with existing
  metadata assumptions.

## Responsibility buckets

### Core local cleanups

- Remove self-moves by turning `LineKind::SelfMove` into `Nop`.
- Remove immediately reversed register copies:
  `movq a, b` followed by `movq b, a`.
- Remove unconditional jumps that target the very next label.
- Fuse `setcc; test{q,l} %rax,%rax; je/jne` into a direct conditional jump.
- Remove `movl %eax, %eax` after a preceding `movq`.

### Optional fast path

- Track a one-bit `rax_is_zero` state and drop repeated `xorl %eax, %eax`.
  This is a cheap local redundancy elimination, not a general zero-value
  analysis.

### Legacy compatibility / mechanical translation residue

- Several helpers are generic string manipulators but currently unused in the
  public pass logic:
  `instruction_writes_to`, `can_redirect_instruction`,
  `replace_dest_register`, `is_redundant_zero_extend`.
- The file header says it is a mechanical translation from a Rust source; the
  mixed helper set and catch-all pass shape reflect that origin.

## Notable fast paths, compatibility paths, and overfit pressure

Fast-path behavior is predominantly string-exact and linear-scan based. That
keeps the pass cheap, but it also makes it fragile:

- `%rax` zero tracking only understands a few line kinds and one exact zeroing
  idiom.
- `setcc` fusion only accepts `je`/`jne` plus exact `testq/testl` forms.
- truncation cleanup only fires on literal `movl %eax, %eax`.

Overfit pressure is high in this file because many optimizations are encoded as
named instruction strings rather than semantic facts. The rebuild should treat
those patterns as evidence of desired simplifications, not as the preferred
future interface. Exact-text matching that exists only to rescue one spelling
belongs in the "overfit to reject" bucket unless it can be generalized into a
small semantic matcher over decoded instruction shape.

## Rebuild ownership

This file should own only narrowly local assembly simplifications that can be
expressed over a stable decoded line/instruction model with explicit mutation
contracts.

This file should not own shared parsing utilities, hidden register numbering
conventions, broad control-flow reasoning, or an ever-growing pile of
spellings-for-one-case pattern matches. In a rebuild, the local rewrite rules
should become small, separately named transformations over a clearer semantic
surface.
